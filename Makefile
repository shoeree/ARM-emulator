# Makefile for Project "ARMed"
# Author: 	Sterling Hoeree
# ID:		3090300043
# Date Created:  2011/04/18
# Date Modified: 2011/06/28

# Flags
CFLAGS=-g -std=c99

all: armvm

armvm: main.o armvm.o armops.o armhash.o lnklists.o vmem.o
	gcc $(CFLAGS) main.o armvm.o armops.o armhash.o lnklists.o vmem.o -o armvm
	
main.o: armvm.o lnklists.o vmem.o main.c armvm.h
	gcc $(CFLAGS) -c main.c
	
armvm.o: vmem.o armvm.c armvm.h
	gcc $(CFLAGS) -c armvm.c

armops.o: vmem.o armops.c armops.h armdefs.def
	gcc $(CFLAGS) -c armops.c

armhash.o: lnklists.o armhash.c armhash.h armdefs.def
	gcc $(CFLAGS) -c armhash.c

lnklists.o: lnklists.c lnklists.h
	gcc $(CFLAGS) -c lnklists.c
	
vmem.o:	vmem.c vmem.h armdefs.def
	gcc $(CFLAGS) -c vmem.c
		
clean:
	rm *.o
