CC=g++

all: sendfile rcvfile 

sendfile: sendfile.o
	$(CC) sendfile.o -o bin/sendfile
	
rcvfile: rcvfile.o
	$(CC) rcvfile.o -o bin/rcvfile

sendfile.o : src/sendfile.cpp
	$(CC) -c src/sendfile.cpp -o sendfile.o

rcvfile.o : src/rcvfile.cpp
	$(CC) -c src/rcvfile.cpp -o rcvfile.o