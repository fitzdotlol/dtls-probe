SOURCE_FILES=src/main.c src/file.c src/rf.c src/patchlist.c src/explorer.c src/vendor/mkdir_p.c

all:
	mkdir -p bin
	cc -g -ggdb -Wall $(SOURCE_FILES) -o bin/dtls -lz -lraylib -lm
