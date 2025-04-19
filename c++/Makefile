CPP=g++
CPPFLAGS=-O3 -Wall -pedantic

abm: main.o abm.o
	$(CPP) $(CPPFLAGS) -o abm main.o abm.o

main.o: main.cpp abm.hpp
	$(CPP) $(CPPFLAGS) -c main.cpp

abm.o: abm.cpp abm.hpp
	$(CPP) $(CPPFLAGS) -c abm.cpp

tests: tests.cpp
	$(CPP) -Wall -pedantic -g -o tests tests.cpp abm.cpp

clean: FORCE
	rm abm *.o tests

FORCE:
