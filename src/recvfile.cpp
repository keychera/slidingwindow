#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "segment.h"

void print_segment(segment seg)
{
	printf("SOH : 0x%02x\n", seg.soh);
	printf("SeqNum : 0x%02x (%d in decimal)\n", seg.seqNum, seg.seqNum);
	printf("STX : 0x%02x\n", seg.stx);
	printf("Data : 0x%02x\n", seg.data & 0xff);
	printf("ETX : 0x%02x\n", seg.etx);
	printf("Checksum : 0x%02x\n", seg.checksum & 0xff);
}

void print_ack_segment(ack_segment ack_seg)
{
	printf("ACK : 0x%02x\n", ack_seg.ack);
	printf("NextSeqNum : 0x%02x (%d in decimal)\n", ack_seg.nextSeq, ack_seg.nextSeq);
	printf("ADV Window Size: 0x%02x\n", ack_seg.windowSize);
	printf("Checksum : 0x%02x\n", ack_seg.checksum & 0xff);
}

int main(int argc, char **argv)
{
	// Handle parameters, initialize
	if (argc < 5)
	{
		perror("Wrong parameters");
		exit(1);
	}

	int sockfd;
	char *filename = argv[1];
	int windowSize = atoi(argv[2]);
	int bufferSize = atoi(argv[3]);
	int port = atoi(argv[4]);

	// Create UDP Socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(1);
	}

	// Configure socket
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof serveraddr);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);

	// Bind socket
	if ((bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) == -1)
	{
		close(sockfd);
		perror("Can't bind");
		exit(1);
	}

	int addr_len = sizeof(struct sockaddr);
	printf("\nWaiting for client on port %d\n", port);
	fflush(stdout);

	struct sockaddr_in clientaddr;

	char buff[256];
	segment seg;
	ack_segment ack_seg;

	FILE *fp;
	fp = fopen(filename, "w+");

	while (1)
	{
		int bytes_read = recvfrom(sockfd, buff, 9, 0, (struct sockaddr *)&clientaddr, (socklen_t *)&addr_len);

		if (bytes_read < 0)
		{
			perror("Error reading buffer");
			exit(1);
		}

		buff[bytes_read] = '\0';

		// Read segment (first segment of buff)
		if (*buff == '\01' && *(buff + 5) == '\02' && *(buff + 7) == '\03')
		{
			seg.soh = *buff;
			seg.seqNum = *(buff + 1);
			seg.stx = *(buff + 5);
			seg.data = *(buff + 6);
			seg.etx = *(buff + 7);
			seg.checksum = *(buff + 8);
		}

		if (seg.checksum == 'c')
		{
			// Print Segment
			printf("Segment received : \n");
			print_segment(seg);
			fflush(stdout);

			// Write data from segment to external file
			fputc(seg.data, fp);
			if (seg.data == '\n')
				break;

			// Prepare ack segment
			ack_seg.ack = '\06';
			ack_seg.nextSeq = seg.seqNum + 1;
			ack_seg.windowSize = windowSize;
			ack_seg.checksum = 'c';

			char ack_buf[7];
			*ack_buf = ack_seg.ack;
			*(ack_buf + 1) = ack_seg.nextSeq;
			*(ack_buf + 5) = ack_seg.windowSize;
			*(ack_buf + 6) = ack_seg.checksum;

			sendto(sockfd, ack_buf, 7, 0, (struct sockaddr *)&clientaddr, sizeof(struct sockaddr));
			fflush(stdout);

			// Print ACK Segment
			print_ack_segment(ack_seg);
		}
	}

	fclose(fp);
	close(sockfd);
	return 0;
}