INC_ROBOT = -I./include

CFLAGS=-Wall
CSTFLAGS =-DDEBUG 

all  : build/send_marvelmind_pos.o build/marvelmind.o bin/marvelmind

# Robot
build/marvelmind.o: src/marvelmind.c
	gcc -c $(CFLAGS) $(INC_ROBOT) $^ -o $@ -pthread

build/rcv_marvelmind_pos.o: src/rcv_marvelmind_pos.c
	gcc -c $(CFLAGS) $(INC_ROBOT) $^ -o $@ -pthread

build/send_marvelmind_pos.o: src/send_marvelmind_pos.c
	gcc -c $(CFLAGS) $(INC_ROBOT) $^ -o $@ -pthread

bin/marvelmind: build/marvelmind.o build/send_marvelmind_pos.o
	gcc $(CFLAGS) $(INC_ROBOT) $^ -o $@ -pthread

# Clean
clean:  clean_robot

clean_robot:
	rm -f bin/* build/*
