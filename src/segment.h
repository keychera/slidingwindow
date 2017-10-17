typedef struct
{
    char soh;
    int seqNum;
    char stx;
    char data;
    char etx;
    char checksum;
} segment;

typedef struct
{
    char ack;
    int nextSeq;
    char windowSize;
    char checksum;
} ack_segment;