all: build/libfzf.so

build/fzf.o: src/fzf.c src/fzf.h
	mkdir -pv build
	gcc -c -Wall -Werror -fpic src/fzf.c -o build/fzf.o

build/util.o: src/util.c src/util.h
	mkdir -pv build
	gcc -c -Wall -Werror -fpic src/util.c -o build/util.o

build/libfzf.so: build/fzf.o build/util.o
	gcc -shared -o build/libfzf.so build/fzf.o build/util.o

.PHONY: lint format db clean
lint:
	luacheck lua

format:
	clang-format --style=file --dry-run -Werror src/*

db:
	compiledb make

clean:
	rm -rf build compile_commands.json
