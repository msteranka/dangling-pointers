CXX = clang++
CXXFLAGS = -std=c++11 -g -gdwarf-2 -rdynamic

all: basic reuse struct

basic:
	$(CXX) $(CXXFLAGS) -o basic basic.cpp

reuse:
	$(CXX) $(CXXFLAGS) -o reuse reuse.cpp

struct:
	$(CXX) $(CXXFLAGS) -o struct struct.cpp

clean:
	rm -f basic reuse struct
