# Compiler
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

# Paths
SRC_DIR = src
OBJ_DIR = obj
BIN = main

# Sources and objects
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Default target
all: $(BIN)

# Debug build
DEBUG_FLAGS = -g -O0

debug: clean
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(BIN)

# Linking
$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compiling
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure obj directory exists
$(OBJ_DIR):
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

# Clean build artifacts
clean:
	-@if exist $(BIN).exe del /Q $(BIN).exe
	-@if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)

.PHONY: all clean
