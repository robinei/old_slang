CC=gcc
CFLAGS=-I. -fno-exceptions -fno-rtti -std=c++98 -g -ldl -ltcc -Wall
DEPS=$(wildcard *.h *.cpp *.c)

main: $(DEPS)
	$(CC) $(CFLAGS) -o main main.cpp

.PHONY: clean

clean:
	rm -f main *.o 
