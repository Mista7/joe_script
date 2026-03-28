# 1. Variables: Define your compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# 2. Targets: What are we building?
TARGET = lexer

# 3. The Build Rule
$(TARGET): lexer.cc
	$(CXX) $(CXXFLAGS) lexer.cc -o $(TARGET)

# 4. Cleanup: Run 'make clean' to delete the executable
clean:
	rm -f $(TARGET)
