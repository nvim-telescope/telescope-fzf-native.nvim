CC=gcc
CFLAGS=-Wall -fpic -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wconversion
COVER=#--coverage

all: build/libfzf.so

build/libfzf.so: src/fzf.c src/fzf.h
	mkdir -pv build
	$(CC) -Ofast $(CFLAGS) -shared src/fzf.c -o build/libfzf.so

build/test: build/libfzf.so test/test.c
	$(CC) -Og -ggdb3 $(CFLAGS) $(COVER) test/test.c -o build/test -I./src -L./build -lfzf -lcmocka

.PHONY: lint format clangdhappy clean test debug
lint:
	luacheck lua

format:
	clang-format --style=file --dry-run -Werror src/* test/*

debug:
	mkdir -pv build
	$(CC) -Og -ggdb3 $(CFLAGS) $(COVER) -shared src/fzf.c -o build/libfzf.so

test: build/test
	@LD_LIBRARY_PATH=${PWD}/build:${LD_LIBRARY_PATH} ./build/test

ntest:
	nvim --headless --noplugin -u test/minrc.vim -c "PlenaryBustedDirectory test/ { minimal_init = './test/minrc.vim' }"

clangdhappy:
	compiledb make

clean:
	rm -rf build compile_commands.json
