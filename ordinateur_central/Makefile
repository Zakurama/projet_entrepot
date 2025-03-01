CPFLAGS=-I./
LDFLAGS=-L./lib
CFLAGS=-Wall

all : bin/ordinateur_central

bin/ordinateur_central: src/main_ordi_central.c ../Inventaire/lib/tcp.so
	gcc $(CFLAGS) $(CPFLAGS) $(LDFLAGS) -I../Inventaire/include $^ -o $@ 

clean:
	rm -f bin/* build/*