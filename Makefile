CC=gcc
CFLAGS=-Wall -g -msse4.2 -msse4a -std=c99
OUT=build
SRCS=*.c

all: clean build

build: tree.o random.o build.o
	$(CC) $(CFLAGS) tree.o random.o build.o -o $(OUT)

tree.o: tree.c
	$(CC) $(CFLAGS) -c tree.c -o tree.o

random.o: random.c
	$(CC) $(CFLAGS) -c random.c -o random.o

build.o: build.c
	$(CC) $(CFLAGS) -c build.c -o build.o

clean:
	rm -rf $(OUT) *.o *~ *dSYM
