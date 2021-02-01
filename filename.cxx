/*
 *
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

char *filename(int id)
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
	oss << "tdc"
		<< std::setfill('0') << std::setw(4) << id << std::setfill(' ') 
		<< "_"
		<< (lt->tm_year + 1900) << ossmonth.str() << ossday.str()
		<< osshour.str() << ossmin.str() << osssec.str();

	int fileno = 0;
	std::string fnamestr = oss.str();
	while (true) {
		//struct stat finfo;
		//if (stat(oss.str().c_str(), &finfo)) break;
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
