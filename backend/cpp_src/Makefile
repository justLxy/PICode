# Makefile for PiCode Encoder and Decoder CLI tools

# Compiler and flags
CXX = g++
CC = gcc
CFLAGS = -O2 -Dcimg_display=0 -I/opt/homebrew/include -Wno-deprecated-register
CXXFLAGS = -std=c++11 -O2 -Dcimg_display=0 -I/opt/homebrew/include -Wno-deprecated-register
LDFLAGS = -L/opt/homebrew/lib -lpng -ljpeg -lpthread

# Directories
ENCODER_DIR = encoder
DECODER_DIR = decoder
BUILD_DIR = build

# Encoder files
ENCODER_C_SRCS = $(wildcard $(ENCODER_DIR)/*.c)
ENCODER_CPP_SRCS = $(filter-out $(ENCODER_DIR)/encoder_cli.cpp, $(wildcard $(ENCODER_DIR)/*.cpp))
ENCODER_CLI_SRC = $(ENCODER_DIR)/encoder_cli.cpp
ENCODER_OBJS = $(patsubst $(ENCODER_DIR)/%.c, $(BUILD_DIR)/encoder/%.o, $(ENCODER_C_SRCS)) \
               $(patsubst $(ENCODER_DIR)/%.cpp, $(BUILD_DIR)/encoder/%.o, $(ENCODER_CPP_SRCS)) \
               $(patsubst $(ENCODER_DIR)/%.cpp, $(BUILD_DIR)/encoder/%.o, $(ENCODER_CLI_SRC))
ENCODER_TARGET = ../encoder

# Decoder files
DECODER_SRCS = $(filter-out $(DECODER_DIR)/decoder_cli.cpp, $(wildcard $(DECODER_DIR)/*.cpp))
DECODER_CLI_SRC = $(DECODER_DIR)/decoder_cli.cpp
DECODER_OBJS = $(patsubst $(DECODER_DIR)/%.cpp, $(BUILD_DIR)/decoder/%.o, $(DECODER_SRCS)) \
               $(patsubst $(DECODER_DIR)/%.cpp, $(BUILD_DIR)/decoder/%.o, $(DECODER_CLI_SRC))
DECODER_TARGET = ../decoder

.PHONY: all clean encoder decoder

all: encoder decoder

encoder: $(ENCODER_TARGET)

decoder: $(DECODER_TARGET)

# Create build directories
$(shell mkdir -p $(BUILD_DIR)/encoder)
$(shell mkdir -p $(BUILD_DIR)/decoder)

# Rule to build encoder executable
$(ENCODER_TARGET): $(ENCODER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to build decoder executable
$(DECODER_TARGET): $(DECODER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile encoder C source files
$(BUILD_DIR)/encoder/%.o: $(ENCODER_DIR)/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

# Rule to compile encoder C++ source files
$(BUILD_DIR)/encoder/%.o: $(ENCODER_DIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

# Rule to compile decoder C++ source files
$(BUILD_DIR)/decoder/%.o: $(DECODER_DIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(ENCODER_TARGET) $(DECODER_TARGET) 