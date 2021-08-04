#
#
#
CC = gcc
CFLAGS = -Wall -g -O
CLIBS = -lzmq

CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -g -O
CXXLIBS = -lzmq

INCLUDES = -Ikol
#LIBS = kol/kolsocket.o kol/koltcp.o
LIBS = -Lkol -lkol

PROGS = fe be dummy dummycheck thtdcreader benb daqtask dtmain
all: $(PROGS)

fe: fe.cxx zportname.cxx kol/libkol.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

be: be.cxx filename.cxx zportname.cxx  mstopwatch.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

fctreader: fctreader.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		-Ikol \
		fctreader.cxx \
		kol/kolsocket.o kol/koltcp.o

thtdcreader: thtdcreader.cxx filename.cxx mstopwatch.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

benb: benb.cxx filename.cxx zportname.cxx  mstopwatch.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

daqtask: daqtask.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -D TEST_MAIN -o $@ $< $(CXXLIBS) $(LIBS)

dtmain: dtmain.cxx daqtask.cxx dtavant.cxx dtrear.cxx dtfilename.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

dummy: dummy.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

dummycheck: dummycheck.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)


kol/kollib.a:
	(cd kol; $(MAKE))

clean:
	rm -f $(PROGS)

cleandata:
	rm -f tdc*.dat
