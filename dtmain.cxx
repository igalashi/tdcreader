/*
 *
 */

#include <iostream>
#include <cstring>

#include "daqtask.cxx"
#include "dtavant.cxx"
#include "dtrear.cxx"



int main(int argc, char* argv[])
{

	int port = 24;
	static char host[128];
	strcpy(host, "192,168,10.56");
	bool is_dummy = false;

	for (int i = 1 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if ((sargv == "-h") && (argc > i)) {
			strncpy(host, argv[i + 1], 128);
		}
		if ((sargv == "-p")  && (argc > i)) {
			port = strtol(argv[i + 1], NULL, 0);
		}
		if (sargv == "--dummy") {
			is_dummy = true;
		}
		#if 0
		if (sargv == "--help") {
			dt_printhelp(argv);
			return 0;
		}
		#endif
	}



	zmq::context_t context(1);

	DTavant *avant = new DTavant(1, host, port, is_dummy);
	DTrear  *rear  = new DTrear(2);

	std::vector<DAQTask*> tasks;
	tasks.push_back(avant);
	tasks.push_back(rear);

	for (auto &i : tasks)  i->run(&context);

	for (auto &i : tasks)  i->clear_is_done();
	avant->set_state(SM_INIT);
	for (auto &i : tasks) while (i->is_done() != true) usleep(10);


	avant->set_state(SM_IDLE);
	sleep(1);
	avant->set_state(SM_RUNNING);
	sleep(1);
	avant->set_state(SM_IDLE);
	sleep(1);
	avant->set_state(SM_END);


	for (auto &i : tasks) i->get_thread()->join();
	for (auto &i : tasks) delete i;

	std::cout << "main end" << std::endl;

	return 0;
}
