CC=gcc

abm: main.o abm.o
	$(CC) -Wall -pedantic -O3 -o abm main.o abm.o `pkg-config --libs --cflags glib-2.0` -lm

main.o: main.c abm.h
	$(CC) -Wall -pedantic -O3 -c main.c `pkg-config --libs --cflags glib-2.0` -lm

abm.o: abm.c abm.h
	$(CC) -Wall -pedantic -O3 -c abm.c `pkg-config --libs --cflags glib-2.0` -lm

abm-debug: abm.c
	$(CC) -Wall -pedantic -g -o abm-debug main.c abm.c `pkg-config --libs --cflags glib-2.0` -lm

clean: FORCE
	rm *.o abm

FORCE:

