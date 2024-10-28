#pragma once

typedef struct
{
    int frameCount;
    int timeoutCount;
    int approvedCount; // Only Receiver
    int rejectedCount;
    int repeatedCount; // Only Receiver
} Statistics;


int fullWrite(unsigned char* data, int nBytes);

int processInformationFrame(unsigned char* packet);

int addByteWithStuff(unsigned char byte, unsigned char* buf);

void statsConstructor(Statistics* stats);


