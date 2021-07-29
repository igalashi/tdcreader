#include <iostream>
#include <thread>
#include <deque>
#include <atomic>
#include <sstream>

#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "zmq.hpp"
#include "koltcp.h"

#if 1
#include <fstream>
const char *filename(int id)
{
        return "/dev/null";
}
#else
#include "filename.cxx"
#endif

#include "mstopwatch.cxx"

enum RunState {
	IDLE,
	RUNNING,
	END,
};


class DAQtask
{
public:
	DAQtask(int);
	virtual ~DAQtask();
	void run(int);
	int get_state(){return m_state;};
protected:
private:
	int m_id = 0;
	int m_state = IDLE;
};

DAQtask::DAQtask(int id) : m_id(id)
{
	return;
}

DAQtask::~DAQtask()
{
	std::cout << "DAQtask " << m_id << " destructed." << std::endl;
	return;
}


const int bufsize = 1024 * sizeof(unsigned int);
static int g_state = IDLE;
static bool g_f_dummy = false;

unsigned int id = 0;
std::atomic<int> g_avant_depth(0);

//zmq::context_t context(1);
//const char* g_snd_endpoint = "tcp://localhost:5558";
//const char* g_rec_endpoint = "tcp://*:5558";
//const char* g_snd_endpoint = "ipc://./hello";
//const char* g_rec_endpoint = "ipc://./hello";
const char* g_snd_endpoint = "inproc://hello";
const char* g_rec_endpoint = "inproc://hello";

void buf_free(void *buf, void *hint)
{
	char *bufbuf = reinterpret_cast<char *>(buf);
	delete bufbuf;
	g_avant_depth--;
	//std::cout << " " << g_avant_depth;
}

//int avant(kol::TcpClient &tcp, int backend_id)
int avant_run(zmq::socket_t &sender, kol::TcpClient &tcp)
{
	//zmq::context_t context(1);

	//zmq::socket_t sender(context, ZMQ_PUSH);
	//sender.setsockopt(ZMQ_SNDBUF, bufsize +  2);
	//sender.setsockopt(ZMQ_SNDHWM, 512*1024);
	std::cout << "avant: ZMQ_SNDBUF : " << sender.getsockopt<int>(ZMQ_SNDBUF) << std::endl;
	std::cout << "avant: ZMQ_SNDHWM : " << sender.getsockopt<int>(ZMQ_SNDHWM) << std::endl;

	sender.connect(g_snd_endpoint);

	int segnum = 0;
	while (1) {

		if (g_state != RUNNING) break;

		char *buf;
		//usleep(100000);
		//const int bufsize = 16 * 1024 * sizeof(unsigned int);
		try {
			buf = new char[bufsize + sizeof(unsigned int)];
		} catch (std::exception &e){
			std::cerr << "avant; Memory allocation fail. " << e.what() << std:: endl;
			return -1;
		}
		g_avant_depth++;

		unsigned int *head = reinterpret_cast<unsigned int *>(buf);
		*head = 0xff000000 | id;
		char *data = reinterpret_cast<char *>(head + 1);
	

		int nread = 0;
		if (! g_f_dummy) {
			try {
				tcp.read(data, bufsize);
			} catch (kol::SocketException &e) {
				std::cerr << "#E tcp read err. " << e.what() << std::endl;
				break;
			}
			nread = tcp.gcount();
		} else {
			for (int i = 0 ; i  < bufsize ; i++) data[i] = i & 0xff;
			nread = bufsize + sizeof(unsigned int);
		}


		zmq::message_t message(
			reinterpret_cast<void *>(buf),
			nread + sizeof(unsigned int),
			buf_free,
			NULL);
		try {
			sender.send(message);
		} catch (zmq::error_t &e) {
			std::cerr << "#E send zmq err. " << e.what() << std::endl;
			//break;
		}
		//std::cout << "." << std::flush;
	
		#if 1
		if ((segnum % 1000) == 0) {
			std::cout << "\rR:" << g_avant_depth << "   " << std::flush;
		}
		#endif

		segnum++;
	}

	return 0;
}


void avant(zmq::socket_t &sender, kol::TcpClient &tcp)
{
	//static int l_state = IDLE;

	while(true) {
		if (g_state == IDLE) {
			//l_state = g_state;
			usleep(1);
		} else
		if (g_state == RUNNING) {
			//l_state = g_state;
			//std::cout << "R: " << id << " " ;
			usleep(1);
			avant_run(sender, tcp);
		} else
		if (g_state == END) {
			//l_state = g_state;
			break;
		}
	}
	std::cout << "avant end : " << g_avant_depth << std::endl;

	return;
}


struct ebbuf {
	unsigned int id;
	std::deque<int> event_number;
	int discard;
	int prev_en;
};

static int nspill = 1;

int rear_run(zmq::socket_t &receiver)
{
	//zmq::context_t context(1);
	//zmq::socket_t receiver(context, ZMQ_PULL);

	//receiver.setsockopt(ZMQ_RCVBUF, 16 * 1024);
	//receiver.setsockopt(ZMQ_RCVHWM, 1000);
	std::cout << "rear: ZMQ_RCVBUF : " << receiver.getsockopt<int>(ZMQ_RCVBUF) << std::endl;
	std::cout << "rear: ZMQ_RCVHWM : " << receiver.getsockopt<int>(ZMQ_RCVHWM) << std::endl;

	receiver.bind(g_rec_endpoint);

	zmq::message_t message;


	int port = 111;
	char wfname[128];
	strncpy(wfname, filename(port), 128);
	std::ofstream ofs;
	ofs.open(wfname, std::ios::out);
	std::cout << wfname << std::endl;

	mStopWatch sw;
	sw.start();
	int wcount = 0;

	std::vector<struct ebbuf> buf;
	int nread_flagment = 0;
	int spillcount = 0;
	while (true) {


		//if (g_state != RUNNING) break;
		if ((g_state != RUNNING) && (g_avant_depth <= 0)) break;

		bool rc;
		try {
			rc = receiver.recv(&message, ZMQ_NOBLOCK);
		} catch (zmq::error_t &e) {
			std::cerr << "#E rear_run zmq recv err. " << e.what() << std::endl;
			//break;
			continue;
		}
		//std::cout << rc  << std::flush;
		if (rc) {
		} else {
			usleep(100);
			continue;
		}

		unsigned int *head = reinterpret_cast<unsigned int *>(message.data());
		unsigned int *data = head + 1;
		//char *cdata =  reinterpret_cast<char *>(head + 1);
		unsigned int id =  (*head) & 0x000000ff;
		unsigned int data_size = message.size() - sizeof(unsigned int);

		bool is_new = true;
		for (unsigned int i = 0 ; i < buf.size() ; i++) {
			if (id == buf[i].id) {
				is_new = false;
			}
		}
		if (is_new) {
			struct ebbuf node;
			node.id = id;
			//node.event_number.push_back(eb->event_number);
			node.discard = 0;
			buf.push_back(node);
		}

		/*
		std::cout << "#" << std::hex << id << " " << *head;
		for (int i = 0 ; i < 8 ; i++) {
			std::cout << " " << data[i];
		}
		std::cout << std::endl;
		*/

		while (data_size > 0) {
			bool is_spill_end = false;
			for (unsigned int i = 0 ; i < (data_size /sizeof(unsigned int)) ; i++) {
				if (data[i] == 0xffff5555) {
					time_t now = time(NULL);
					ofs.write(reinterpret_cast<char *>(data), sizeof(unsigned int) * (i + 1));
					ofs.write(reinterpret_cast<char *>(&now), sizeof(time_t));
					data = &(data[i + 1]);
					data_size = data_size - ((i + 1) * sizeof(unsigned int));
					spillcount++;
					wcount += sizeof(unsigned int) * (i + 1) + sizeof(time_t);

					if ((spillcount % nspill) == 0) {
						char wfname[128];
						ofs.close();
						
						int elapse = sw.elapse();
						sw.start();
						double wspeed = static_cast<double>(wcount)
							/ 1024 / 1024
							* 1000 / static_cast<double>(elapse);
						std::cout << wfname << " "
							<< wcount << " B " << elapse << " ms "
							<< wspeed << " MiB/s"  << std::endl;

						wcount = 0;
						strncpy(wfname, filename(port), 128);
						ofs.open(wfname, std::ios::out);
					}
					is_spill_end = true;
				}
			}
			if (! is_spill_end) {
				ofs.write(reinterpret_cast<char *>(data), data_size);
				wcount += data_size;
				break;
			}
		}


		#if 0
		if ((nread_flagment % 1000) == 0) {
			std::cout << "\rID : ";
			for (unsigned int i = 0 ; i < buf.size() ;i++) {
				std::cout << "  " << i
					<< ": " << buf[i].id;
			}
			std::cout << "      " << std::flush;

		} else {
			//std::cout << "." << std::flush;
		}
		#endif

		nread_flagment++;
	}

	ofs.close();

	return 0;
}


void rear(zmq::socket_t &receiver)
{
	//static int l_state = IDLE;
	while(true) {
		if (g_state == IDLE) {
			//l_state = g_state;
			usleep(1);
		} else
		if (g_state == RUNNING) {
			//l_state = g_state;
			//std::cout << " x ";
			usleep(1);
			rear_run(receiver);
		} else
		if (g_state == END) {
			//l_state = g_state;
			break;
		}
	}

	std::cout << "rear end : " << g_avant_depth << std::endl;

	return;
}

int main(int argc, char *argv[])
{

	int port = 24;
	char host[128];
	strcpy(host, "192.168.10.56");
	std::string shost(host);
	std::string sid = shost.substr(shost.rfind(".") + 1, shost.length());
	std::istringstream(sid) >> id;

	//int backend_id = 0;
	

	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if ((sargv == "-h") && (argc > i)) {
			strncpy(host, argv[i + 1], 128);
		}
		if ((sargv == "-p")  && (argc > i)) {
			port = strtol(argv[i + 1], NULL, 0);
		}

		#if 0
		if ((sargv == "-i") && (argc > i)) {
			id = strtol(argv[i + 1], NULL, 0);
		}
		if ((sargv == "-b") && (argc > i)) {
			backend_id = strtol(argv[i + 1], NULL, 0);
		}

		if (sargv == "--help") {
			fe_printhelp(argv);
			return 0;
		}
		#endif

		if (sargv == "--dummy") {
			g_f_dummy = true;
		}
	}

	std::cout << "host: " << host << "  port: " << port << "  id: " << id;
	//std::cout << " backend: " << backend_id << std::endl;
	std::cout << std::endl;


	kol::SocketLibrary socklib;
	//kol::TcpClient tcp(host, port);
	kol::TcpClient *tcp = 0;

	if (! g_f_dummy) {
		try {
			tcp = new kol::TcpClient(host, port);
			//kol::TcpClient tcp(host, port);
			//reader(tcp, backend_id);
			//tcp.close();
		} catch(kol::SocketException &e) {
			std::cout << "Socket open error. (" << host << ", " << port
				<< ") "  << e.what() << std::endl;
			return 1;
		}
	} else {
		std::cout << "Dummy mode" << std::endl;
	}


	zmq::context_t context(1);
	zmq::socket_t sender(context, ZMQ_PUSH);
	zmq::socket_t receiver(context, ZMQ_PULL);

	std::thread th_avant(avant, std::ref(sender), std::ref(*tcp));
	std::thread th_rear(rear, std::ref(receiver));

	sleep(1);
	g_state = RUNNING;
	sleep(2);
	g_state = IDLE;
	sleep(1);
	g_state = END;
	sleep(1);

	if (! g_f_dummy) {
	tcp->close();
	delete tcp;
	}

	//th_avant.detach();
	//th_rear.detach();
	th_avant.join();
	th_rear.join();
	std::cout << "main end" << std::endl;
	
	return 0;
}


#ifdef TEST_MAIN
void p1(int val)
{
	for (int i = 0 ; i < val ; i++) {
		std::cout << ".";
		usleep(1);
	}
	std::cout << std::endl;
}

int main() {
	std::thread t1(p1, 100);

	int i = 10;
	std::thread t2([&i](int x) {
		int counter = 0;
		while(counter++ < x) {
			i += counter;
			std::cout << " " << i;
			usleep(1);
		}
	}, 10); // 関数オブジェクトの引数に10を渡す

	t1.detach();
	t2.join(); //スレッドt2の処理が終了するまで待機
	std::cerr << "#" << i << std::endl;

	return 0;
}
#endif
