#
#
#
CC = gcc
CFLAGS = -Wall -g -O
CLIBS = -lzmq

CXX = g++
CXXFLAGS = -Wall -g -O
CXXLIBS = -lzmq

INCLUDES = -Ikol
LIBS = kol/kolsocket.o kol/koltcp.o

PROGS = fe be dummy dummycheck
all: $(PROGS)

fe: fe.cxx zportname.cxx kol/kollib.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

be: be.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

eb: eb.cxx filename.cxx zportname.cxx
	$(CXX) $(CXXFLAGS) -o $@ $< $(CXXLIBS)

fctreader: fctreader.cxx
	$(CXX) $(CXXFLAGS) -o $@ \
		-Ikol \
		fctreader.cxx \
		kol/kolsocket.o kol/koltcp.o

dummy: dummy.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

dummycheck: dummycheck.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(CXXLIBS) $(LIBS)

kol/kollib.a:
	(cd kol; $(MAKE))

clean:
	rm -f $(PROGS)
