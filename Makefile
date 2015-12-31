
SOURCES := src/main.c src/opcode.c 

BINARY  := rhdas


all:
	gcc -g -Iinclude $(SOURCES) -o $(BINARY) -Wall
