VENDOR_SRC_FILES=src/vendor/mkdir_p.c src/vendor/toml.c
SOURCE_FILES=$(VENDOR_SRC_FILES) src/main.c src/file.c src/rf.c src/patchlist.c src/explorer.c src/filetree.c src/config.c

all:
	mkdir -p bin
	cc -g -ggdb -Wall $(SOURCE_FILES) -o bin/dtls -lz -lraylib -lm
