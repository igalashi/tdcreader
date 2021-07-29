/*
 *
 */

#include <iostream>
#include <thread>
#include <atomic>

#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "zmq.hpp"
#include "koltcp.h"


#include "daqtask.cxx"



const int bufsize = 1024 * sizeof(unsigned int);

unsigned int id = 0;
std::atomic<int> g_avant_depth(0);


const char* g_snd_endpoint = "tcp://localhost:5558";
//const char* g_snd_endpoint = "ipc://./hello";
//const char* g_snd_endpoint = "inproc://hello";

void buf_free(void *buf, void *hint)
{
	char *bufbuf = reinterpret_cast<char *>(buf);
	delete bufbuf;
	g_avant_depth--;
}



class DTavant : public DAQTask
{
public:
	DTavant(int i) : DAQTask(i) {};
	DTavant(int, char *, int, bool);
protected:
	virtual int st_init(void *) override;
	virtual int st_idle(void *) override;
	virtual int st_running(void *) override;
private:
	char *m_host;
	int m_port;
	kol::TcpClient *tcp;
	bool m_is_dummy;
};

DTavant::DTavant(int i, char *host, int port, bool is_dummy)
	: DAQTask(i), m_host(host), m_port(port), m_is_dummy(is_dummy)
{
}

int DTavant::st_init(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "avant(" << m_id << ") init" << std::endl;
	}

	if (! m_is_dummy) {
		try {
			tcp = new kol::TcpClient(m_host, m_port);
		} catch(kol::SocketException &e) {
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cout << "Socket open error. (" << m_host << ", " << m_port
				<< ") "  << e.what() << std::endl;
			return 1;
		}
	} else {
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "Dummy mode" << std::endl;
	}

	return 0;
}

int DTavant::st_idle(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "avant(" << m_id << ") idle" << std::endl;
	}
	usleep(100000);

	return 0;
}

int DTavant::st_running(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "avant(" << m_id << ") running" << std::endl;
	}

	zmq::socket_t sender(
		*(reinterpret_cast<zmq::context_t *>(context)),
		ZMQ_PUSH);
	//sender.setsockopt(ZMQ_SNDBUF, bufsize +  2);
	//sender.setsockopt(ZMQ_SNDHWM, 512*1024);
	std::cout << "avant: ZMQ_SNDBUF : " << sender.getsockopt<int>(ZMQ_SNDBUF) << std::endl;
	std::cout << "avant: ZMQ_SNDHWM : " << sender.getsockopt<int>(ZMQ_SNDHWM) << std::endl;

	sender.connect(g_snd_endpoint);


	int segnum = 0;
	while (1) {

		if (c_state != SM_RUNNING) break;

		char *buf;
		try {
			buf = new char[bufsize + sizeof(unsigned int)];
		} catch (std::exception &e){
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cerr << "avant; Memory allocation fail. " << e.what() << std:: endl;
			return -1;
		}
		g_avant_depth++;

		unsigned int *head = reinterpret_cast<unsigned int *>(buf);
		*head = 0xff000000 | id;
		char *data = reinterpret_cast<char *>(head + 1);
	
		int nread = 0;
		if (! m_is_dummy) {
			try {
				tcp->read(data, bufsize);
			} catch (kol::SocketException &e) {
				std::lock_guard<std::mutex> lock(*c_dtmtx);
				std::cerr << "#E tcp read err. " << e.what() << std::endl;
				break;
			}
			nread = tcp->gcount();
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
			std::lock_guard<std::mutex> lock(*c_dtmtx);
			std::cerr << "#E send zmq err. " << e.what() << std::endl;
			return -1;
		}
	
		#if 1
		if ((segnum % 1000) == 0) {
			std::cout << "\rR:" << g_avant_depth << "   " << std::flush;
		}
		#endif

		segnum++;
	}



	return 0;
}
