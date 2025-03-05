INC_INV = -I./inventaire/include
INC_ORDI = -I./ordinateur_central/include

LIB_UTILS = ./utils/lib/
INC_UTILS = -I./utils/include

CFLAGS=-Wall

export PKG_CONFIG_PATH := /usr/lib/x86_64-linux-gnu/pkgconfig
CUNIT_CFLAGS=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags cunit)
CUNIT_LDFLAGS=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs cunit)

all : ordinateur_central/bin/ordinateur_central inventaire/bin/inventaire inventaire/bin/client libs

# Ordinateur Central
ordinateur_central/bin/ordinateur_central: ordinateur_central/src/main_ordi_central.c ordinateur_central/build/ordi_central.o inventaire/build/inventaire.o $(LIB_UTILS)tcp.so
	gcc $(CFLAGS) $(INC_ORDI) $(INC_UTILS) $^ -o $@ 

ordinateur_central/build/ordi_central.o: ordinateur_central/src/ordi_central.c
	gcc -c $(CFLAGS) $(INC_ORDI) $(INC_UTILS) $(INC_INV) $^ -o $@

# Inventaire
inventaire/bin/inventaire: inventaire/src/main_inventaire.c inventaire/build/inventaire.o $(LIB_UTILS)tcp.so
	gcc $(CFLAGS) $(INC_INV) $(INC_UTILS) -lpthread $^ -o $@ 

inventaire/bin/client : inventaire/src/client.c $(LIB_UTILS)tcp.so
	gcc $(CFLAGS) $(INC_INV) $(INC_UTILS) $^ -o $@

inventaire/build/inventaire.o: inventaire/src/inventaire.c
	gcc -c $(CFLAGS) $(INC_INV) $(INC_UTILS) $^ -o $@

# Tests
tests: tests/bin/test_ordinateur_central tests/bin/test_inventaire
	./tests/bin/test_ordinateur_central
	./tests/bin/test_inventaire

tests/bin/test_ordinateur_central: tests/test_ordinateur_central.c ordinateur_central/build/ordi_central.o inventaire/build/inventaire.o $(LIB_UTILS)tcp.so
	gcc $(CFLAGS) $(INC_ORDI) $(CUNIT_CFLAGS) $(INC_UTILS) $(INC_INV) $^ -o $@ $(CUNIT_LDFLAGS)

tests/bin/test_inventaire: tests/test_inventaire.c inventaire/build/inventaire.o $(LIB_UTILS)tcp.so
	gcc $(CFLAGS) $(INC_INV) $(CUNIT_CFLAGS) $(INC_UTILS) $^ -o $@ $(CUNIT_LDFLAGS)

# Libs
libs: utils/lib/tcp.so

utils/lib/tcp.so: utils/build/tcp-fpic.o
	gcc -shared $^ -o $@

utils/build/tcp-fpic.o: utils/src/tcp.c
	gcc -c -fPIC $(CFLAGS) $(INC_UTILS) $^ -o $@

# Clean
clean: clean_utils clean_ordi clean_inventaire clean_tests

clean_tests:
	rm -f tests/bin/*

clean_utils:
	rm -f utils/lib/* utils/build/*

clean_ordi:
	rm -f ordinateur_central/bin/* ordinateur_central/build/*

clean_inventaire:
	rm -f inventaire/bin/* inventaire/build/*
