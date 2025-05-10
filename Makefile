# Compiler settings
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN = main

# Source layout
LEXER_SRC     = $(wildcard $(SRC_DIR)/lexer/*.cpp)
PARSER_SRC    = $(wildcard $(SRC_DIR)/parser/*.cpp)
RUNTIME_SRC   = $(wildcard $(SRC_DIR)/runtime/*.cpp)
UTIL_SRC      = $(wildcard $(SRC_DIR)/util/*.cpp)
MAIN_SRC 	  = $(SRC_DIR)/main.cpp

SRCS = $(LEXER_SRC) $(PARSER_SRC) $(RUNTIME_SRC) $(UTIL_SRC) $(MAIN_SRC)
OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(subst $(SRC_DIR)/,,$(SRCS)))

# Default target
all: $(BIN)

# Debug build
DEBUG_FLAGS = -g -O0

debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean $(BIN)

# Link
$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compile to obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir "$(dir $@)" 2>NUL || exit 0
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule (Windows-compatible)
clean:
	@if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)
	@if exist $(BIN).exe del /Q $(BIN).exe
	@if exist $(BIN) del /Q $(BIN)
