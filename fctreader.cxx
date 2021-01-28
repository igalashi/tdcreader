#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

#include "koltcp.h"


char *filename()
{
	static char fname[128];
	
	time_t now = time(NULL);
	struct tm *lt = localtime(&now);

	std::ostringstream ossmonth;
	if ((lt->tm_mon + 1) < 10) {
		ossmonth << "0" << lt->tm_mon + 1;
	} else {
		ossmonth << lt->tm_mon + 1;
	}
	std::ostringstream ossday;
	if ((lt->tm_mday) < 10) {
		ossmonth << "0" << lt->tm_mday;
	} else {
		ossmonth << lt->tm_mday;
	}
	std::ostringstream osshour;
	if ((lt->tm_hour) < 10) {
		osshour << "0" << lt->tm_hour;
	} else {
		osshour << lt->tm_hour;
	}
	std::ostringstream ossmin;
	if ((lt->tm_min) < 10) {
		ossmin << "0" << lt->tm_min;
	} else {
		ossmin << lt->tm_min;
	}
	std::ostringstream osssec;
	if ((lt->tm_sec) < 10) {
		ossmin << "0" << lt->tm_sec;
	} else {
		ossmin << lt->tm_sec;
	}

	std::ostringstream oss;
	oss << "fct"
		<< (lt->tm_year + 1900) << ossmonth.str() << ossday.str()
		<< osshour.str() << ossmin.str() << osssec.str();

	int fileno = 0;
	std::string fnamestr = oss.str();
	while (true) {
		std::ifstream infile((fnamestr + ".dat").c_str());
		if (! infile.good()) {
			break;
		} else {
			std::ostringstream ioss;
			ioss << ++fileno;
			fnamestr = oss.str() + "_" + ioss.str();
		}
	}

	strncpy(fname, (fnamestr + ".dat").c_str(), 128);
	
	return fname;
}

const int buf_size = 1024 * sizeof(unsigned int) * 128;
int main(int argc, char* argv[])
{
	int port = 24;
	char host[128];
	strcpy(host, "192.168.10.56");
	int nspill = 1;

	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if ((sargv == "-h") && (argc > i)) {
			strncpy(host, argv[i + 1], 128);
		}
		if ((sargv == "-p")  && (argc > i)) {
			port = strtol(argv[i + 1], NULL, 0);
		}
		if ((sargv == "-s") && (argc > i)) {
			nspill = strtol(argv[i + 1], NULL, 0);
		}
	}

	std::cout << "host: " << host << " port: " << port << " ";
	std::cout << std::endl;

	kol::SocketLibrary socklib;
	kol::TcpClient tcp(host, port);

	#if 0
	int opt;
	int size = 6291456;
	if ((opt = tcp.setsockopt(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)))
		< 0) {
		perror("setsockpot SO_RCVBUF");
	}
	#endif

	char wfname[128];
	strncpy(wfname, filename(), 128);
	std::ofstream ofs;
	ofs.open(wfname, std::ios::out);
	std::cout << wfname << std::endl;

	int spillcount = 1;
	static char buf[buf_size + 8];
	unsigned int *ubuf;
	try {
		while(tcp.read(buf, buf_size)) {
			ubuf = reinterpret_cast<unsigned int *>(buf);
			for (unsigned int i = 0 ; i < (tcp.gcount() / sizeof(unsigned int)) ; i++) {
				///if ((*ubuf & 0xffff0000) == 0x4e000000) {
				if (*ubuf  == 0xffff5555) {
					time_t now = time(NULL);
					#ifdef TO_STDOUT
					fwrite(reinterpret_cast<char *>(ubuf), sizeof(unsigned int), 1, stdout);
					fwrite(reinterpret_cast<char *>(&now), sizeof(time_t), 1,stdout);
					#else
					ofs.write(reinterpret_cast<char *>(ubuf), sizeof(unsigned int));
					ofs.write(reinterpret_cast<char *>(&now), sizeof(time_t));
					#endif
						
					if ((spillcount % nspill) == 0) {
						char wfname[128];
						ofs.close();
						strncpy(wfname, filename(), 128);
						ofs.open(wfname, std::ios::out);
						std::cout << wfname << std::endl;
					}
					spillcount++;
	
				} else {
					#ifdef TO_STDOUT
					fwrite(ubuf, sizeof(unsigned int), 1, stdout);
					#else
					ofs.write(reinterpret_cast<char *>(ubuf), sizeof(unsigned int));
					#endif
				}
				ubuf++;
			}
		}
	} catch(std::exception e) {
		std::cout << "tcp error. " << e.what() << std::endl;
	}
	tcp.close();
	ofs.close();

	return 0;
}
