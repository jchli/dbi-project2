CC=clang
CFLAGS=-Wall -g
OUT=build
SRCS=*.c

all: build

build: tree.c tree.h build.c
	$(CC) $(CFLAGS) tree.c build.c -o $(OUT)

clean:
	rm -rf $(OUT) *.o *~ *dSYM
