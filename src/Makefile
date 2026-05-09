# 1. Variables: Define your compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

TARGET = compiler

SRCS = main.cc lexer.cc

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
