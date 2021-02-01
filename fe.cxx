/*
 *
 *
 */

#include <iostream>
#include <string>
#include <sstream>

#include <cstdio>
#include <cstring>

//#include <sys/types.h>
//#include <unistd.h>

#include "zmq.hpp"
//#include "zhelpers.hpp"

#include "koltcp.h"

#include "zportname.cxx"

//const int bufsize = 16 * 1024 * sizeof(unsigned int);
const int bufsize = 1024 * sizeof(unsigned int);


unsigned int id = 0;
int g_depth = 0;

void buf_free(void *buf, void *hint)
{
	char *bufbuf = reinterpret_cast<char *>(buf);
	delete bufbuf;
	g_depth--;
	//std::cout << " " << g_depth;
}

int reader(kol::TcpClient &tcp, int backend_id)
{
	zmq::context_t context(1);

	zmq::socket_t sender(context, ZMQ_PUSH);
	//sender.setsockopt(ZMQ_SNDBUF, bufsize +  2);
	sender.setsockopt(ZMQ_SNDHWM, 512*1024);
	std::cout << "ZMQ_SNDBUF : " << sender.getsockopt<int>(ZMQ_SNDBUF) << std::endl;
	std::cout << "ZMQ_SNDHWM : " << sender.getsockopt<int>(ZMQ_SNDHWM) << std::endl;
	//sender.connect("tcp://localhost:5558");
	//sender.connect("ipc://./hello");
	sender.connect(zportname(backend_id));

	int segnum = 0;
	while (1) {
		char *buf;
		//usleep(100000);
		//const int bufsize = 16 * 1024 * sizeof(unsigned int);
		try {
			buf = new char[bufsize + sizeof(unsigned int)];
		} catch (std::exception &e){
			std::cerr << "reader; Memory allocation fail. " << e.what() << std:: endl;
			return -1;
		}
		g_depth++;

		unsigned int *head = reinterpret_cast<unsigned int *>(buf);
		*head = 0xff000000 | id;
		char *data = reinterpret_cast<char *>(head + 1);
	
		try {
			tcp.read(data, bufsize);
		} catch (kol::SocketException &e) {
			std::cerr << "#E tcp read err. " << e.what() << std::endl;
			break;
		}
		int nread = tcp.gcount();
		//for (int i = 0 ; i  < bufsize ; i++) data[i] = i & 0xff;
		//int nread = bufsize + sizeof(unsigned int);

		zmq::message_t message(
			reinterpret_cast<void *>(buf),
			nread + sizeof(unsigned int),
			buf_free,
			NULL);
		sender.send(message);
	
		#if 1
		if ((segnum % 1000) == 0) {
			std::cout << "\r" << g_depth << "   " << std::flush;
		}
		#endif

		segnum++;
	}

	return 0;
}
#include "fe_printhelp.cxx"

int main (int argc, char *argv[])
{

	int port = 24;
	char host[128];
	strcpy(host, "192.168.10.56");
	std::string shost(host);
	std::string sid = shost.substr(shost.rfind(".") + 1, shost.length());
	std::istringstream(sid) >> id;

	int backend_id = 0;
	

	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if ((sargv == "-h") && (argc > i)) {
			strncpy(host, argv[i + 1], 128);
		}
		if ((sargv == "-p")  && (argc > i)) {
			port = strtol(argv[i + 1], NULL, 0);
		}
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
	}

	std::cout << "host: " << host << "  port: " << port << "  id: " << id;
	std::cout << " backend: " << backend_id << std::endl;


	kol::SocketLibrary socklib;
	//kol::TcpClient tcp(host, port);

	try {
		kol::TcpClient tcp(host, port);
		reader(tcp, backend_id);
		tcp.close();
	} catch(kol::SocketException &e) {
		std::cout << "Socket open error. (" << host << ", " << port
			<< ") "  << e.what() << std::endl;
		return 1;
	}

	//tcp.close();

	return 0;
}
