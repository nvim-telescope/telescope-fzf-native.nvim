all: libfzf.so

build/fzf.o: src/fzf.c
	mkdir -pv build
	gcc -c -Wall -Werror -fpic src/fzf.c -o build/fzf.o

libfzf.so: build/fzf.o
	gcc -shared -o build/libfzf.so build/fzf.o

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
