/*
 *
 */
#include <iostream>

void fe_printhelp(char *argv[])
{
	std::cout << argv[0] << " -h <host-name or host IP>"
		<< " : " << "The device host-name of host IP number" << std::endl;
	std::cout << argv[0] << " -p <port number>"
		<< " : " << "The device port number" << std::endl;
	std::cout << argv[0] << " -i <id number>"
		<< " : " << "The source id number" << std::endl;
	std::cout << "        The last two digits is used if the host IP is specified by the number.";
	std::cout << std::endl;
	std::cout << argv[0] << " -b <port number> : communication port with be" << std::endl;
	std::cout << "        If the number is lower than 1024, UDS is used." << std::endl;
	std::cout << "        In the other case TCP/IP is used." << std::endl;
	
	return;
}
