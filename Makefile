CXX=gcc
BINARY=network
SOURCE_DIR=src
BUILD_DIR=bld
SHELL=/usr/bin/env bash

C_FILES=$(SOURCE_DIR)/main.c \
	$(SOURCE_DIR)/tuntap.c \
	$(SOURCE_DIR)/utils.c \
	$(SOURCE_DIR)/ethernet.c \
	$(SOURCE_DIR)/arp.c


# Output Colours. Must reset to plain or the rest of the terminal
# will remain that colour
OUT_GREEN="\033[0;32m"
OUT_RED="\033[0;31m"
OUT_PLAIN="\033[0m"

PASS=echo -e $(OUT_GREEN) "test passed" $(OUT_PLAIN)
FAIL=echo -e $(OUT_RED) "test failed" $(OUT_PLAIN)

__init := $(shell mkdir -p $(BUILD_DIR))

build:
	$(CXX) $(C_FILES) -o $(BUILD_DIR)/$(BINARY)

# Can use if defined TRACE to include extra tracing in debug builds
debug:
	$(CXX) $(C_FILES) -o $(BUILD_DIR)/$(BINARY) -g -DTRACE

clean:
	rm -f $(BUILD_DIR)/*
