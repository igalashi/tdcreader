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
	int spillcount = 1;
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

		for (unsigned int i = 0 ; i < (data_size / sizeof(unsigned int)) ; i++) {
			if (data[i] == 0xffff5555) {
				time_t now = time(NULL);
				ofs.write(reinterpret_cast<char *>(&(data[i])), sizeof(unsigned int));
				std::cout << "#D sizeof time_t " << sizeof(time_t) << std::endl;
				ofs.write(reinterpret_cast<char *>(&now), sizeof(time_t));
				if ((spillcount % nspill) == 0) {
					char wfname[128];
					ofs.close();
					strncpy(wfname, filename(), 128);
					ofs.open(wfname, std::ios::out);
					std::cout << wfname << std::endl;
				}
				spillcount++;

			} else {
				ofs.write(reinterpret_cast<char *>(&(data[i])), sizeof(unsigned int));
			}
		}
		//ofs.write(cdata, datasize);


		
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
