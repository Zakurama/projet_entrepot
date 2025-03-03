CPFLAGS=-I./include
LDFLAGS=-L./lib

LIB_UTILS = ../utils/lib/
INC_UTILS = -I../utils/include

CFLAGS=-Wall

export PKG_CONFIG_PATH := /usr/lib/x86_64-linux-gnu/pkgconfig
CUNIT_CFLAGS=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags cunit)
CUNIT_LDFLAGS=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs cunit)

all : bin/inventaire bin/client

build/inventaire.o: src/inventaire.c
	gcc -c $(CFLAGS) $(CPFLAGS) $(INC_UTILS) $^ -o $@

bin/inventaire: src/main_inventaire.c build/inventaire.o $(LIB_UTILS)tcp.so
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) $(INC_UTILS) -lpthread $^ -o $@ 

bin/client : src/client.c $(LIB_UTILS)tcp.so
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) $(INC_UTILS) $^ -o $@

tests: bin/test
	bin/test

bin/test: tests/test.c build/inventaire.o $(LIB_UTILS)tcp.so
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) $(CUNIT_CFLAGS) $(INC_UTILS) $^ -o $@ $(CUNIT_LDFLAGS)

clean: 
	rm -f bin/* build/*
