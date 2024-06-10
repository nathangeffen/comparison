CPP=g++
CPPFLAGS=-O3

abm: main.o abm.o
	$(CPP) -o abm main.o abm.o

tests: tests.cpp
	$(CPP) -g -o tests tests.cpp abm.cpp

clean: FORCE
	rm abm *.o tests

FORCE:
