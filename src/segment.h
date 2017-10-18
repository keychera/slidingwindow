#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <iostream>
#include <sstream>

typedef struct
{
    char soh;
    unsigned int seqNum;
    char stx;
    unsigned char data;
    char etx;
    char checksum;
} segment;

typedef struct
{
    char ack;
    unsigned int nextSeq;
    char windowSize;
    char checksum;
} ack_segment;
