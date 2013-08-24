CC=gcc
DEBUG=-g
CFLAGS=$(DEBUG) -Wall
PROGS=tsp

all: $(PROGS)

tsp: tsp.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

tsp.o: tsp.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(PROGS) *.o *~

