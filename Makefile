CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -g3 -O0 -fsanitize=address

TARGET = main

SRC = main.cpp
HEADERS = btree.hpp bplustree.hpp

OBJ = $(SRC:.cpp=.o)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
