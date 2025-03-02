CPFLAGS=-I./include
LDFLAGS=-L./lib
CFLAGS=-Wall

export PKG_CONFIG_PATH := /usr/lib/x86_64-linux-gnu/pkgconfig
CUNIT_CFLAGS=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags cunit)
CUNIT_LDFLAGS=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs cunit)

all : bin/ordinateur_central

bin/ordinateur_central: src/main_ordi_central.c ../Inventaire/lib/tcp.so
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) -I../Inventaire/include $^ -o $@ 

tests: bin/test
	bin/test

build/ordi_central.o: src/ordi_central.c
	gcc -c $(CFLAGS) $(CPFLAGS) $^ -o $@

bin/test: tests/test.c build/ordi_central.o
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) $(CUNIT_CFLAGS) $^ -o $@ $(CUNIT_LDFLAGS)

clean:
	rm -f bin/* build/*