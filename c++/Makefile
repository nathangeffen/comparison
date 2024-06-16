CPP=g++
CPPFLAGS=-O3 -Wall -pedantic

abm: main.o abm.o
	$(CPP) -o abm main.o abm.o

tests: tests.cpp
	$(CPP) -Wall -pedantic -g -o tests tests.cpp abm.cpp

clean: FORCE
	rm abm *.o tests

FORCE:
