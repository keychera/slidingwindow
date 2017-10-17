CC=g++

all: sendfile recvfile 

sendfile: sendfile.o
	$(CC) sendfile.o -o bin/sendfile
	
recvfile: recvfile.o
	$(CC) recvfile.o -o bin/recvfile

sendfile.o : src/sendfile.cpp
	$(CC) -c src/sendfile.cpp -o sendfile.o

recvfile.o : src/recvfile.cpp
	$(CC) -c src/recvfile.cpp -o recvfile.o
