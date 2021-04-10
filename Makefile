all: libfzf.so

build/fzf.o: src/fzf.c src/fzf.h
	mkdir -pv build
	gcc -c -Wall -Werror -fpic src/fzf.c -o build/fzf.o

build/util.o: src/util.c src/util.h
	mkdir -pv build
	gcc -c -Wall -Werror -fpic src/util.c -o build/util.o

libfzf.so: build/fzf.o build/util.o
	gcc -shared -o build/libfzf.so build/fzf.o build/util.o

.PHONY: test lint format db clean
test:
	nvim --headless --noplugin -u scripts/minimal_init.vim -c "PlenaryBustedDirectory tests/ { minimal_init = './scripts/minimal_init.vim' }"

lint:
	luacheck lua

format:
	clang-format --style=file --dry-run -Werror src/*

db:
	compiledb make

clean:
	rm -rf build compile_commands.json
