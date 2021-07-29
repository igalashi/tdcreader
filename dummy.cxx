/*
 *
 *
 */

#include <iostream>
#include <string>
#include <sstream>

#include <unistd.h>
#include "koltcp.h"

const int buf_size = 16 * 1024 * 1024;

int main(int argc, char *argv[])
{
	int port = 8024;

	for (int i = 0 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if((sargv == "-p") && (argc > i)) {
			std::string param(argv[i + 1]);
			std::istringstream iss(param);
			iss >> port;
		}
	}
	std::cout << "Port: " << port << std::endl;


	try {
		kol::SocketLibrary socklib;

		char *buf = new char[buf_size];
		for (int i = 0 ; i < buf_size ; i++) {
			buf[i] = i & 0xff;
		}
		buf[3] = 0xba;
		buf[2] = 0xbe;
		buf[1] = 0xca;
		buf[0] = 0xfe;
		buf[buf_size - 4] = 0x55;
		buf[buf_size - 3] = 0x55;
		buf[buf_size - 2] = 0xff;
		buf[buf_size - 1] = 0xff;


		kol::TcpServer server(port);
		while(1) {
			kol::TcpSocket sock = server.accept();

			try {
				while (true) {
					sock.write(buf, buf_size);
					std::cout << "." << std::flush;
					usleep(100 * 1000);
				}
			} catch (kol::SocketException &e) {
				sock.close();
			}
		}
	} catch(kol::SocketException &e) {
		std::cout << "Error " << e.what() << std::endl;
	}

	return 0;
}
