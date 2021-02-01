/*
 *
 */
#include <iostream>

void be_printhelp(char *argv[])
{
	std::cout << argv[0] << " -s <number of spill>"
		<< " : " << "The number of spills in a file" << std::endl;
	std::cout << argv[0] << " -p <port number> : communication port with fe" <<std::endl;
	std::cout << "        If the number is lower than 1024, UDS is used." << std::endl;
	std::cout << "        In the other case TCP/IP is used." << std::endl;
	
	return;
}
