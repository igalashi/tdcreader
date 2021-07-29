/*
 *
 */

#include <iostream>
#include <thread>

#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "zmq.hpp"
#include "koltcp.h"


#include "daqtask.cxx"
#include "mstopwatch.cxx"
#include "filename.cxx"

const char* g_rec_endpoint = "tcp://*:5558";
//const char* g_rec_endpoint = "ipc://./hello";
//const char* g_rec_endpoint = "inproc://hello";


class DTrear : public DAQTask
{
public:
	DTrear(int i) : DAQTask(i) {};
protected:
	//virtual void state_machine(void *) override;
	virtual int st_init(void *) override;
	virtual int st_idle(void *) override;
	virtual int st_running(void *) override;
private:
};

#if 0
void DTrear::state_machine(void *context)
{
	c_dtmtx->lock();
	std::cout << "#rear sm start# " << m_id << " : " << c_state << std::endl;
	c_dtmtx->unlock();

	while(true) {
		switch (c_state) {
			case SM_INIT :
				if (!m_is_done) {
					st_init(context);
				} else {
					usleep(1000);
				}
				m_is_done = true;
				break;

			case SM_IDLE :
				usleep(1);
				st_idle(context);
				break;

			case SM_RUNNING :
				st_running(context);
				break;
		}
		if (c_state == SM_END) break;
	}

	std::cout << "Task:" << m_id << " end." << std::endl;

	return;
}
#endif

int DTrear::st_init(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "rear(" << m_id << ") init" << std::endl;
	}


	return 0;
}

int DTrear::st_idle(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "rear(" << m_id << ") idle" << std::endl;
	}
	usleep(100000);

	return 0;
}


struct ebbuf {
	unsigned int id;
	std::deque<int> event_number;
	int discard;
	int prev_en;
};

static int nspill = 1;

int DTrear::st_running(void *context)
{
	{
		std::lock_guard<std::mutex> lock(*c_dtmtx);
		std::cout << "rear(" << m_id << ") running" << std::endl;
	}

	zmq::socket_t receiver(
		*(reinterpret_cast<zmq::context_t *>(context)),
		ZMQ_PULL);

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


		//if (c_state != SM_RUNNING) break;
		if ((c_state != SM_RUNNING) && (g_avant_depth <= 0)) break;

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
