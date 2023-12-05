# Simple makefile that builds various tools created that utilize libsca.

MF_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# Library
LIBSCA_DIR=$(MF_DIR)/../lib
LIBSCA_MAKEFILE=$(LIBSCA_DIR)/makefile
LIBSCA_SRC=$(LIBSCA_DIR)/*.c
LIBSCA_INCLUDE=$(LIBSCA_DIR)/libsca.h
LIBSCA_LIB_STATIC=$(LIBSCA_DIR)/libsca.a
LIBSCA_MAKE_ARGS=--no-print-directory

# Source files
TOOLS_SRCS=$(wildcard *.c)
TOOLS_BINS=$(patsubst %.c,%,$(TOOLS_SRCS))

# Output files
CACHE_BIN=cache
TIMING_BIN=timing
FLUSHRELOAD_BIN=flush-reload
SPECTREV1_BIN=spectre-v1

# Flags
CFLAGS=-Wall -g
LDFLAGS=$(LIBSCA_LIB_STATIC)

default: all

# Makes all targets.
all: libsca $(TOOLS_BINS)

# Builds the side-channel attack library.
libsca: $(LIBSCA_SRC)
	@$(MAKE) $(LIBSCA_MAKE_ARGS) -C $(LIBSCA_DIR) -f $(LIBSCA_MAKEFILE)

# Builds a single C source file into a tool.
%: %.c
	$(CC) $(CFLAGS) -I$(LIBSCA_DIR) $< -o $@ $(LDFLAGS)

# Removes binaries and other scraps.
clean:
	rm -f $(TOOLS_BINS)

