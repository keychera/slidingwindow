
#include "segment.h"

void print_segment(segment seg)
{
	//printf("SOH : 0x%02x\n", seg.soh);
	printf("SeqNum : 0x%02x (%d in decimal)\n", seg.seqNum, seg.seqNum);
	//printf("STX : 0x%02x\n", seg.stx);
	printf("Data : 0x%02x\n", seg.data & 0xff);
	//printf("ETX : 0x%02x\n", seg.etx);
	printf("Checksum : 0x%02x\n", seg.checksum & 0xff);
	printf("\n");
}

void print_ack_segment(ack_segment ack_seg)
{
	//printf("ACK : 0x%02x\n", ack_seg.ack);
	printf("NextSeqNum : 0x%02x (%d in decimal)\n", ack_seg.nextSeq, ack_seg.nextSeq);
	//printf("ADV Window Size: 0x%02x\n", ack_seg.windowSize);
	printf("Checksum : 0x%02x\n", ack_seg.checksum & 0xff);
	printf("\n");
}

// unsigned char CRC8(unsigned char data, unsigned char len)
// {
// 	unsigned char crc = 0x00;
// 	while (len--)
// 	{
// 		unsigned char extract = data++;
// 		for (unsigned char tempI = 8; tempI; tempI--)
// 		{
// 			unsigned char sum = (crc ^ extract) & 0x01;
// 			crc >>= 1;
// 			if (sum)
// 			{
// 				crc ^= 0x8C;
// 			}
// 			extract >>= 1;
// 		}
// 	}
// 	return crc;
// }

unsigned char CRC8(const unsigned char * data, const unsigned int size)
{
    unsigned char crc = 0;
    for ( unsigned int i = 0; i < size; ++i )
    {
        unsigned char inbyte = data[i];
        for ( unsigned char j = 0; j < 8; ++j )
        {
            unsigned char mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if ( mix ) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}


int main(int argc, char **argv)
{
	// Handle parameters, initialize
	if (argc < 6)
	{
		perror("Wrong parameters");
		exit(1);
	}

	int sockfd;
	char *filename = argv[1];
	int maxWindowSize = atoi(argv[2]);
	int bufferSize = atoi(argv[3]);
	char *destIP = argv[4];
	int destPort = atoi(argv[5]);

	// Create UDP Socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(1);
	}

	// Timeout for ACK recv
	struct timeval rcvtimeo;
	rcvtimeo.tv_sec = 5;
	rcvtimeo.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeo, sizeof(struct timeval));
	//recv/sendbuffer that is 256bit(bufferSize input that is also 256)
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize));

	// Configure socket
	struct sockaddr_in serveraddr;
	struct hostent *host;
	int sin_size;
	host = (struct hostent *)gethostbyname(destIP);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(destPort);
	serveraddr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(serveraddr.sin_zero), 8);
	sin_size = sizeof(struct sockaddr);

	unsigned char buff[256];
	segment seg;
	ack_segment ack_seg;

	//opening the files
	FILE *fp;
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		perror("Error while opening file\n");
		exit(EXIT_FAILURE);
	}

	//Sliding Window
	unsigned int LAR = -1;
	unsigned int LFS = -1;

	int windowSize = maxWindowSize;
	while (fread(buff, 1, bufferSize, fp) > 0)
	{
		int i = 0;

		while ((i < bufferSize) && (buff[i] != '\0'))
		{
			if (LFS - LAR < windowSize)
			{
				// Convert data to segment
				seg.soh = '\01';
				seg.seqNum = LFS + 1;
				seg.stx = '\02';
				seg.data = buff[i];
				seg.etx = '\03';
				// seg.checksum = 'c';
				unsigned char crc = CRC8(reinterpret_cast<unsigned char *>(&seg), 8);
				seg.checksum = crc;

				char seg_buf[9];
				*seg_buf = seg.soh;
				char hex[4];
				sprintf(hex,"%04x",seg.seqNum);
				*(seg_buf + 1) = hex[0];
				*(seg_buf + 2) = hex[1];
				*(seg_buf + 3) = hex[2];
				*(seg_buf + 4) = hex[3];
				*(seg_buf + 5) = seg.stx;
				*(seg_buf + 6) = seg.data;
				*(seg_buf + 7) = seg.etx;
				*(seg_buf + 8) = seg.checksum;

				printf("Segment going to be sent : \n");
				print_segment(seg);
				fflush(stdout);

				// Send data
				sendto(sockfd, seg_buf, 9, 0, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
				LFS++;
				i++;
			}
			else
			{
				char ack_buf[7];
				int bytes_recv = -1;
				bytes_recv = recvfrom(sockfd, ack_buf, 7, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&sin_size);
				char hex[4];
				hex[0] = *(ack_buf + 1);
				hex[1] = *(ack_buf + 2);
				hex[2] = *(ack_buf + 3);
				hex[3] = *(ack_buf + 4);
				std::string hexstr(hex);
				hexstr.resize(4);
				ack_seg.nextSeq = std::stoul(hexstr,nullptr,16);
				if (bytes_recv > 0)
				{
					LAR = ack_seg.nextSeq - 1;
				}
				else
				{
					i -= LFS - LAR;
					LFS = LAR;
				}
				ack_seg.ack = *ack_buf;
				ack_seg.windowSize = *(ack_buf + 5);
				ack_seg.checksum = *(ack_buf + 6);

				// Print ACK Segment
				printf("Ack Received : \n");
				print_ack_segment(ack_seg);

				//windowSize = ack_seg.windowSize < maxWindowSize ? ack_seg.windowSize : maxWindowSize;
				fflush(stdout);
			}
		}

		for (int i=0; i<256; i++)
			buff[i] = '\0';
	}

	// Mark end of data
	seg.soh = '\01';
	seg.seqNum = LFS + 1;
	seg.stx = '\02';
	seg.data = '\0';
	seg.etx = '\03';
	// seg.checksum = 'c';
	unsigned char crc = CRC8(reinterpret_cast<unsigned char *>(&seg), 8);
	seg.checksum = crc;

	char seg_buf[9];
	*seg_buf = seg.soh;
	*(seg_buf + 1) = seg.seqNum;
	*(seg_buf + 5) = seg.stx;
	*(seg_buf + 6) = seg.data;
	*(seg_buf + 7) = seg.etx;
	*(seg_buf + 8) = seg.checksum;

	// Send data
	sendto(sockfd, seg_buf, 9, 0, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));

	fclose(fp);
	close(sockfd);
}