CXX=g++
CC=$(CXX)
CXXFLAGS=$(INC) --std=c++11 -g -O1

all: sc

sc: main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

test: sc
	./sc english --test

clean:
	rm -rf $(APPS)

