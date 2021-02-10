CXX = clang++
CXXFLAGS = -std=c++11 -g -gdwarf-2 -rdynamic -pthread
BIN_DIR = bin/

all: $(BIN_DIR)basic $(BIN_DIR)reuse $(BIN_DIR)struct $(BIN_DIR)multithreaded $(BIN_DIR)region $(BIN_DIR)big

$(BIN_DIR)basic:
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)basic basic.cpp

$(BIN_DIR)reuse:
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)reuse reuse.cpp

$(BIN_DIR)struct:
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)struct struct.cpp

$(BIN_DIR)multithreaded:
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)multithreaded multithreaded.cpp

$(BIN_DIR)region:
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)region region.cpp

$(BIN_DIR)big:
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)big big.cpp

clean:
	rm -f $(BIN_DIR)basic $(BIN_DIR)reuse $(BIN_DIR)struct $(BIN_DIR)multithreaded $(BIN_DIR)region $(BIN_DIR)big
