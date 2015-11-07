CXX=g++
CC=$(CXX)
CXXFLAGS=$(INC) --std=c++11 -g -O1

all: sc

sc: main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(APPS)

