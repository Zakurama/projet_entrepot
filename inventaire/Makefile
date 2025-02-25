CPFLAGS=-I./include
LDFLAGS=-L./lib
CFLAGS=-Wall

all : bin/inventaire bin/client libs

bin/inventaire: src/inventaire.c lib/tcp.so
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) -lpthread $^ -o $@ 

bin/client : src/client.c lib/tcp.so
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) $^ -o $@


libs: lib/tcp.so

lib/tcp.so: build/tcp-fpic.o
	gcc -shared $^ -o $@

build/tcp-fpic.o: src/tcp.c
	gcc -c -fPIC $(CFLAGS) $(CPFLAGS) $^ -o $@

clean:
	rm -f bin/* build/*

clean-libs:
	rm -f lib/* build/*
