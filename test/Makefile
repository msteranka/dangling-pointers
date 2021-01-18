CXX = clang++
CXXFLAGS = -std=c++11 -g -gdwarf-2 -rdynamic -pthread

all: basic reuse struct multithreaded

basic:
	$(CXX) $(CXXFLAGS) -o basic basic.cpp

reuse:
	$(CXX) $(CXXFLAGS) -o reuse reuse.cpp

struct:
	$(CXX) $(CXXFLAGS) -o struct struct.cpp

multithreaded:
	$(CXX) $(CXXFLAGS) -o multithreaded multithreaded.cpp

clean:
	rm -f basic reuse struct multithreaded
