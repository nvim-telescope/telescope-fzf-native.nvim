CC=gcc
# CFLAGS=-Ofast -Wall -Werror -fpic -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wconversion
CFLAGS=-Ofast -Wall -Werror -fpic

all: build/libfzf.so

build/libfzf.so: src/fzf.c src/fzf.h
	mkdir -pv build
	$(CC) $(CFLAGS) -shared src/fzf.c -o build/libfzf.so

build/test: build/libfzf.so test/test.c
	$(CC) $(CFLAGS) test/test.c -o build/test -I./src -L./build -lfzf -lcmocka

.PHONY: lint format clangdhappy clean test
lint:
	luacheck lua

format:
	clang-format --style=file --dry-run -Werror src/* test/*

test: build/test
	@LD_LIBRARY_PATH=${PWD}/build:${LD_LIBRARY_PATH} ./build/test

clangdhappy:
	compiledb make

clean:
	rm -rf build compile_commands.json
