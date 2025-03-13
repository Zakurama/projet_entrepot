INC_ROBOT = -I./include

CFLAGS=-Wall
CSTFLAGS =-DDEBUG 

all  : build/central_pi.o build/marvelmind.o bin/marvelmind

# Robot
build/marvelmind.o: src/marvelmind.c
	gcc -c $(CFLAGS) $(INC_ROBOT) $^ -o $@ -pthread

build/central_pi.o: src/central_pi.c
	gcc -c $(CFLAGS) $(INC_ROBOT) $^ -o $@ -pthread

bin/marvelmind: build/marvelmind.o build/central_pi.o
	gcc $(CFLAGS) $(INC_ROBOT) $^ -o $@ -pthread

# Clean
clean:  clean_robot

clean_robot:
	rm -f bin/* build/*
