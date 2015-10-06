CC=gcc
CFLAGS=-I.
DEPS = BlueManLEDMaster
OBJ = main.o demo.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

master: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)
	chmod +x $@ 

