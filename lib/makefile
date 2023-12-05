# Makefile to compile the side-channel attack library.

# Flags
CFLAGS=-Wall -g -fPIC

# Source filese
LIBSCA_SRC=$(wildcard ./*.c)
LIBSCA_OBJ=$(patsubst %.c, %.o, $(LIBSCA_SRC))

# Outputted libraries
LIBSCA_BIN_SO=./libsca.so
LIBSCA_BIN_AR=./libsca.a

default: all

all: libsca.a libsca.so

# Compiles a single C source file.
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# Compiles C code (uses the above rule).
sources: $(LIBSCA_OBJ)

# Generates a static library.
libsca.a: sources
	ar rcs $@ $(LIBSCA_OBJ)

# Generates a shared library.
libsca.so: sources
	$(CC) -shared $(CFLAGS) -o $@ $(LIBSCA_OBJ)

# Cleans up junk.
clean:
	rm -f $(wildcard ./*.o)
	rm -f $(LIBSCA_BIN_AR)
	rm -f $(LIBSCA_BIN_SO)

