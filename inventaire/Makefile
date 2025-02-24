CPFLAGS=-I./
LDFLAGS=-L./lib
CFLAGS=-Wall

all : bin/inventaire bin/client

bin/inventaire: src/inventaire.c src/tcp.c
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) -lpthread -Iinclude $^ -o $@

bin/client : src/client.c src/tcp.c
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) -Iinclude $^ -o $@

clean:
	rm -f bin/* build/*

clean-libs:
	rm -f lib/* build/*
