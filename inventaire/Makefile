CPFLAGS=-I./include
LDFLAGS=-L./lib
CFLAGS=-Wall

export PKG_CONFIG_PATH := /usr/lib/x86_64-linux-gnu/pkgconfig
CUNIT_CFLAGS=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags cunit)
CUNIT_LDFLAGS=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs cunit)

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

tests: bin/test
	bin/test

bin/test: tests/test.c lib/tcp.so
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) $(CUNIT_CFLAGS) $^ -o $@ $(CUNIT_LDFLAGS)

clean:
	rm -f bin/* build/*

clean-libs:
	rm -f lib/* build/*
