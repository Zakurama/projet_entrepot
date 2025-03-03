CPFLAGS=-I./include
CFLAGS=-Wall

all: lib/tcp.so

lib/tcp.so: build/tcp-fpic.o
	gcc -shared $^ -o $@

build/tcp-fpic.o: src/tcp.c
	gcc -c -fPIC $(CFLAGS) $(CPFLAGS) $^ -o $@

clean:
	rm -f lib/* build/*