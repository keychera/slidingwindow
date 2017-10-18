
#include "segment.h"
#include <vector>

void print_segment(segment seg)
{
	//printf("SOH : 0x%02x\n", seg.soh);
	printf("SeqNum : 0x%02x (%u in decimal)\n", seg.seqNum, seg.seqNum);
	//printf("STX : 0x%02x\n", seg.stx);
	printf("Data : 0x%02x\n", seg.data & 0xff);
	//printf("ETX : 0x%02x\n", seg.etx);
	printf("Checksum : 0x%02x\n", seg.checksum & 0xff);
	printf("\n");
}

void print_ack_segment(ack_segment ack_seg)
{
	//printf("ACK : 0x%02x\n", ack_seg.ack);
	printf("NextSeqNum : 0x%02x (%u in decimal)\n", ack_seg.nextSeq, ack_seg.nextSeq);
	//printf("ADV Window Size: 0x%02x\n", ack_seg.windowSize);
	printf("Checksum : 0x%02x\n", ack_seg.checksum & 0xff);
	printf("\n");
}

// unsigned char CRC8(const reinterpret_cast<unsigned char *>data, unsigned char len) {
//     unsigned char crc = 0x00;
//     while (len--) {
//         unsigned char extract = data++;
//         for (unsigned char tempI = 8; tempI; tempI--) {
//             unsigned char sum = (crc ^ extract) & 0x01;
//             crc >>= 1;
//             if (sum) {
//                 crc ^= 0x8C;
//             }
//             extract >>= 1;
//         }
//     }
//     return crc;
// }

unsigned char CRC8(const unsigned char *data, const unsigned int size)
{
	unsigned char crc = 0;
	for (unsigned int i = 0; i < size; ++i)
	{
		unsigned char inbyte = data[i];
		for (unsigned char j = 0; j < 8; ++j)
		{
			unsigned char mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix)
				crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
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
	int maxWindowSize = atoi(argv[2]);
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

	unsigned char buff[256];
	segment seg;
	ack_segment ack_seg;

	FILE *fp;
	fp = fopen(filename, "w+b");

	unsigned int LAF = maxWindowSize - 1;
	unsigned int LAS = 0; //Last ACK sent
	std::vector<int> segmentsReceived;
	std::vector<int>::iterator segIt;
	for (int i = 0; i < maxWindowSize; i++)
	{
		segmentsReceived.push_back(-1);
	}

	while (1)
	{
		int bytes_read = recvfrom(sockfd, buff, 9, 0, (struct sockaddr *)&clientaddr, (socklen_t *)&addr_len);

		if (bytes_read < 0)
		{
			perror("Error reading buffer");
			exit(1);
		}

		// Read segment (first segment of buff)
		if (*buff == '\01' && *(buff + 5) == '\02' && *(buff + 7) == '\03')
		{
			seg.soh = *buff;
			char hex[4];
			hex[0] = *(buff + 1);
			hex[1] = *(buff + 2);
			hex[2] = *(buff + 3);
			hex[3] = *(buff + 4);
			std::string hexstr(hex);
			hexstr.resize(4);
			try
			{
				seg.seqNum = std::stoul(hexstr, nullptr, 16);
			}
			catch (...)
			{
			}
			seg.stx = *(buff + 5);
			seg.data = *(buff + 6);
			seg.etx = *(buff + 7);
			seg.checksum = *(buff + 8);
		}

		// End of data
		if (seg.data == '\0')
			break;

		//check if this is the window
		if (seg.seqNum <= LAF && (seg.seqNum > LAS || LAS == 0))
		{

			unsigned char crc = CRC8(reinterpret_cast<unsigned char *>(&seg), 8);
			if (seg.checksum == crc)
			//if (seg.checksum == 'c')
			{
				// Print Segment
				printf("Segment received : \n");
				print_segment(seg);
				fflush(stdout);
				printf("\n");

				segIt = segmentsReceived.begin();
				for (int i = 0; i < seg.seqNum - LAS; i++)
				{
					segIt++;
				}
				segmentsReceived.insert(segIt, seg.seqNum);
				segmentsReceived.resize(maxWindowSize);

				//process vector

				int seqNumToAck = -1;
				for (int i = 0; i < segmentsReceived.size(); i++)
				{
					if (segmentsReceived[i] != -1)
					{
						seqNumToAck = segmentsReceived[i];
					}
					else
					{
						break;
					}
				}
				if (seqNumToAck != -1)
				{
					// Write data from segment to external file
					fwrite(&(seg.data), 1, 1, fp);

					// Prepare ack segment
					ack_seg.ack = '\06';
					ack_seg.nextSeq = seqNumToAck + 1;
					ack_seg.windowSize = maxWindowSize;
					unsigned char crc = CRC8(reinterpret_cast<unsigned char *>(&ack_seg), 6);
					ack_seg.checksum = crc;
					//ack_seg.checksum = 'c';

					char ack_buf[7];
					*ack_buf = ack_seg.ack;
					char hex[4];
					sprintf(hex, "%04x", ack_seg.nextSeq);
					*(ack_buf + 1) = hex[0];
					*(ack_buf + 2) = hex[1];
					*(ack_buf + 3) = hex[2];
					*(ack_buf + 4) = hex[3];
					*(ack_buf + 5) = ack_seg.windowSize;
					*(ack_buf + 6) = ack_seg.checksum;

					// Print ACK Segment
					printf("Sending Ack : \n");
					print_ack_segment(ack_seg);
					printf("\n");

					sendto(sockfd, ack_buf, 7, 0, (struct sockaddr *)&clientaddr, sizeof(struct sockaddr));
					fflush(stdout);

					for (int i = 0; i < seqNumToAck - LAS; i++)
					{
						segmentsReceived.erase(segmentsReceived.begin());
						segmentsReceived.push_back(-1);
					}
					LAS = seqNumToAck;
					LAF = LAS + maxWindowSize;
				}
			}
		}
		else
		{
		}
	}

	fclose(fp);
	close(sockfd);
	return 0;
}