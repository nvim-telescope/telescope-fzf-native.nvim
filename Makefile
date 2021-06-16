CFLAGS = -Wall -Werror -fpic
COVERAGE ?=

ifeq ($(OS),Windows_NT)
    MKD = -mkdir
    RM = cmd /C rmdir /Q /S
    CC = gcc
    TARGET := libfzf.dll
else
    MKD = mkdir -p
    RM = rm -rf
    TARGET := libfzf.so
endif

all: build/$(TARGET)

build/$(TARGET): src/fzf.c src/fzf.h
	$(MKD) build
	$(CC) -O3 $(CFLAGS) -shared src/fzf.c -o build/$(TARGET)

build/test: build/$(TARGET) test/test.c
	$(CC) -Og -ggdb3 $(CFLAGS) $(COVERAGE) test/test.c -o build/test -I./src -L./build -lfzf -lcmocka

build/benchmark: build/$(TARGET) test/benchmark.c
	$(CC) -O3 $(CFLAGS) test/benchmark.c -o build/benchmark -I./src -L./build -lfzf -lcurl -lm

.PHONY: lint format clangdhappy clean test debug ntest benchmark
lint:
	luacheck lua

format:
	clang-format --style=file --dry-run -Werror src/fzf.c src/fzf.h test/test.c test/benchmark.c
	stylua --color always -c --glob "**/*.lua" -- lua

debug:
	$(MKD) build
	$(CC) -Og -ggdb3 $(CFLAGS) $(COVERAGE) -shared src/fzf.c -o build/$(TARGET)

test: build/test
	LD_LIBRARY_PATH=${PWD}/build:${LD_LIBRARY_PATH} ./build/test

benchmark: build/benchmark
	LD_LIBRARY_PATH=${PWD}/build:${LD_LIBRARY_PATH} ./build/benchmark

ntest:
	nvim --headless --noplugin -u test/minrc.vim -c "PlenaryBustedDirectory test/ { minimal_init = './test/minrc.vim' }"

clangdhappy:
	compiledb make

clean:
	$(RM) build
