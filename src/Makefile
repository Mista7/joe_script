CXX = g++
LLVM_CONFIG = /opt/homebrew/opt/llvm/bin/llvm-config

LLVM_INCLUDE = -isystem $(shell $(LLVM_CONFIG) --includedir)
LLVM_CXXFLAGS = $(filter-out -I%,$(shell $(LLVM_CONFIG) --cxxflags))

CXXFLAGS = -std=c++17 -Wall -Wextra -g $(LLVM_CXXFLAGS) $(LLVM_INCLUDE)
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --libs core)

TARGET = compiler
SRCS = main.cc lexer.cc parser.cc semantics.cc symbol-table.cc

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)
