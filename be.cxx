/*
 *
 *
 */

#include <zmq.hpp>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <vector>
#include <deque>

#if 1
#include "filename.cxx"
#else
const char *filename()
{
	return "/dev/null";
}
#endif

#include "zportname.cxx"


struct ebbuf {
	unsigned int id;
	std::deque<int> event_number;
	int discard;
	int prev_en;
};


static int nspill = 1;


int reader(int port)
{
	zmq::context_t context(1);
	zmq::socket_t receiver(context, ZMQ_PULL);
	//receiver.bind("tcp://*:5558");
	//receiver.bind("ipc://./hello");
	receiver.bind(zportname(port));

	zmq::message_t message;

	char wfname[128];
	strncpy(wfname, filename(), 128);
	std::ofstream ofs;
	ofs.open(wfname, std::ios::out);
	std::cout << wfname << std::endl;

	std::vector<struct ebbuf> buf;
	int nread_flagment = 0;
	int spillcount = 0;
	while (true) {

		receiver.recv(&message);

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
					if ((spillcount % nspill) == 0) {
						char wfname[128];
						ofs.close();
						strncpy(wfname, filename(), 128);
						ofs.open(wfname, std::ios::out);
						std::cout << wfname << std::endl;
					}
					is_spill_end = true;
				}
			}
			if (! is_spill_end) {
				ofs.write(reinterpret_cast<char *>(data), data_size);
				break;
			}
		}


		#if 0
		for (unsigned int i = 0 ; i < (data_size / sizeof(unsigned int)) ; i++) {
			if (data[i] == 0xffff5555) {
				time_t now = time(NULL);
				ofs.write(reinterpret_cast<char *>(&(data[i])), sizeof(unsigned int));
				ofs.write(reinterpret_cast<char *>(&now), sizeof(time_t));
				spillcount++;
				if ((spillcount % nspill) == 0) {
					char wfname[128];
					ofs.close();
					strncpy(wfname, filename(), 128);
					ofs.open(wfname, std::ios::out);
					std::cout << wfname << std::endl;
				}
			} else {
				ofs.write(reinterpret_cast<char *>(&(data[i])), sizeof(unsigned int));
			}
		}
		#endif

		
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

		nread_flagment++;
	}

	ofs.close();

	return 0;
}

int main (int argc, char *argv[])
{
	//int id = 0;
	//int port = 5558;
	int port = 0;

	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if ((sargv == "-s") && (argc > i)) {
			nspill = strtol(argv[i + 1], NULL, 0);
		}
		if ((sargv == "-p") && (argc > i)) {
			port = strtol(argv[i + 1], NULL, 0);
		}
	}

	reader(port);

	return 0;
}
