/*
 *
 *
 */

#include <iostream>
#include <cstdio>

char * zportname(int port)
{
	static char pname[64];

	if (port > 1024) {
		sprintf(pname, "tcp://*:%d", port);
	} else {
		sprintf(pname, "ipc:///tmp/zport%04d", port);
	}

	std::cout << "Connection port: " << pname << std::endl;

	return pname;
}
