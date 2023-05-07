CC=g++
CFLAGS=-c

all: censor

censor: main.o words.o
	$(CC) -o censor main.o words.o

main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp

words.o: words.cpp
	$(CC) $(CFLAGS) words.cpp
	
clean: 
	rm -rf *.o censor

test:
	bash test.sh
