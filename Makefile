CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O0 -g -march=native -fsanitize=address -Wno-unused-parameter -Wno-unused-variable

TARGET = bench

SRC = bench.cpp
HEADERS = btree.hpp bplustree.hpp


test: test.o
	$(CXX) $(CXXFLAGS) -o $@ $<

bench: bench.o
	$(CXX) $(CXXFLAGS) -o $@ $<

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f test test.o bench bench.o


.PHONY: all clean
