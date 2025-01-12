CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O3 -march=native -fsanitize=address

TARGET = bench

SRC = bench.cpp
HEADERS = btree.hpp bplustree.hpp

OBJ = $(SRC:.cpp=.o)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
