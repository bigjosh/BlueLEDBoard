CC=gcc
CFLAGS=-I.

blueled: blueled.cpp blueled.h
	$(CC) -c -o $@ $< $(CFLAGS)

blueled: blueled.o
	gcc -o blueled blueled.o $(CFLAGS)
	chmod +x $@ 
