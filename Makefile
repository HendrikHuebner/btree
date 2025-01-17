CXX = g++
TESTFLAGS = -std=c++20 -Wall -Wextra -pedantic -O0 -g -march=native
BENCHFLAGS = -std=c++20 -Wall -Wextra -pedantic -O3 -march=native

TARGET = bench

SRC = bench.cpp
HEADERS = btree.hpp bplustree.hpp


test: test.o
	$(CXX) $(TESTFLAGS) -o $@ $<

bench: bench.o
	$(CXX) $(BENCHFLAGS) -o $@ $<

test.o: test.cpp $(HEADERS)
	$(CXX) $(TESTFLAGS) -c $< -o $@

bench.o: bench.cpp $(HEADERS)
	$(CXX) $(BENCHFLAGS) -c $< -o $@

clean:
	rm -f test test.o bench bench.o


.PHONY: all clean
