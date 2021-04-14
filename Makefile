CC=gcc
CFLAGS=-Ofast -Wall -Werror -fpic

all: build/libfzf.so

build/libfzf.so: src/fzf.c src/fzf.h
	mkdir -pv build
	$(CC) $(CFLAGS) -shared src/fzf.c -o build/libfzf.so

.PHONY: lint format db clean
lint:
	luacheck lua

format:
	clang-format --style=file --dry-run -Werror src/*

db:
	compiledb make

clean:
	rm -rf build compile_commands.json
