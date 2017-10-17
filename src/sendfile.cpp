#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

typedef struct {
	char soh;
	int seqNum;
	char stx;
	char data;
	char etx;
	char checksum;
} segment;

typedef struct {
	char ack;
	int nextSeq;
	char windowSize;
	char checksum;
} ack_segment;

void print_segment(segment seg) {
	printf("SOH : 0x%02x\n", seg.soh);
	printf("SeqNum : 0x%02x (%d in decimal)\n", seg.seqNum, seg.seqNum);
	printf("STX : 0x%02x\n", seg.stx);
	printf("Data : 0x%02x\n", seg.data & 0xff);
	printf("ETX : 0x%02x\n", seg.etx);
	printf("Checksum : 0x%02x\n", seg.checksum & 0xff);
}

void print_ack_segment(ack_segment ack_seg) {
	printf("ACK : 0x%02x\n", ack_seg.ack);
	printf("NextSeqNum : 0x%02x (%d in decimal)\n", ack_seg.nextSeq, ack_seg.nextSeq);
	printf("ADV Window Size: 0x%02x\n", ack_seg.windowSize);
	printf("Checksum : 0x%02x\n", ack_seg.checksum & 0xff);
}

int main(int argc, char** argv) {
	// Handle parameters, initialize
	if (argc < 6) {
		perror("Wrong parameters");
		exit(1);
	}

	int sockfd;
	char* filename = argv[1];
	int windowSize = atoi(argv[2]);
	int bufferSize = atoi(argv[3]);
	char* destIP = argv[4];
	int destPort = atoi(argv[5]);

	// Create UDP Socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Cannot create socket");
		exit(1);
	}

	// Configure socket
	struct sockaddr_in serveraddr;
	struct hostent *host;
	int sin_size;
	host = (struct hostent *) gethostbyname(destIP);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(destPort);
	serveraddr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(serveraddr.sin_zero), 8);
	sin_size = sizeof(struct sockaddr);

	char buff[256];
	segment seg;
	ack_segment ack_seg;

	FILE *fp;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("Error while opening file\n");
		exit(EXIT_FAILURE);
	}

	while (fgets(buff, bufferSize, fp) != NULL) {
		int i = 0;
		buff[strlen(buff)] = '\0';

		while ((i < bufferSize) && (buff[i] != '\0')) {
			// Convert data to segment
			seg.soh = '\01';
			seg.seqNum = 1;
			seg.stx = '\02';
			seg.data = buff[i];
			seg.etx = '\03';
			seg.checksum = 'c';

			char seg_buf[9];
			*seg_buf = seg.soh;
			*(seg_buf+1) = seg.seqNum;
			*(seg_buf+5) = seg.stx;
			*(seg_buf+6) = seg.data;
			*(seg_buf+7) = seg.etx;
			*(seg_buf+8) = seg.checksum;

			printf("Segment going to be sent : \n");
			print_segment(seg);
			fflush(stdout);

			// Send data
			sendto(sockfd, seg_buf, 9, 0, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));

			i++;
		}
	}

	fclose(fp);
	
	char ack_buf[7];
	int bytes_recv = recvfrom(sockfd, ack_buf, 7, 0, (struct sockaddr *)&serveraddr, (socklen_t*)&sin_size);
	printf("ACK Received : %s\n", ack_buf);
	ack_seg.ack = *ack_buf;
	ack_seg.nextSeq = *(ack_buf+1);
	ack_seg.windowSize = *(ack_buf+5);
	ack_seg.checksum = *(ack_buf+6);
	print_ack_segment(ack_seg);
	fflush(stdout);

	if (ack_seg.checksum == 'c') {
		// Sliding window
	}

	close(sockfd);
}