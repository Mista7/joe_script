CXX = g++
LLVM_CONFIG = /opt/homebrew/opt/llvm/bin/llvm-config

LLVM_INCLUDE = -isystem $(shell $(LLVM_CONFIG) --includedir)
LLVM_CXXFLAGS = $(filter-out -I%,$(shell $(LLVM_CONFIG) --cxxflags))

CXXFLAGS = -std=c++17 -Wall -Wextra -g $(LLVM_CXXFLAGS) $(LLVM_INCLUDE)
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --system-libs --libs core native)

TARGET = compiler
# Added codegen.cc here
SRCS = main.cc lexer.cc parser.cc semantics.cc symbol-table.cc codegen.cc
# Dynamically generate a list of .o files from the SRCS list
OBJS = $(SRCS:.cc=.o)

# 1. Link the final executable using the pre-compiled object files
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

# 2. Compile only the modified .cc files into .o object files
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS) output.o a.out
