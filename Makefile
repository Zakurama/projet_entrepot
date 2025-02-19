CPFLAGS=-I./
LDFLAGS=-L./lib
CFLAGS=-Wall

all : bin/inventaire bin/client

bin/inventaire: src/inventaire.c
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) -lpthread $^ -o $@

bin/client : src/client.c
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f bin/* build/*

clean-libs:
	rm -f lib/* build/*
