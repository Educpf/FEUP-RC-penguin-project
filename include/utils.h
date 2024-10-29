#pragma once

typedef struct
{
    int frameCount;
    int timeoutCount;
    int approvedCount; // Only Receiver
    int rejectedCount;
    int repeatedCount; // Only Receiver
    int strangeCount;
} Statistics;

// Assures that all the data is written to serial port
// Return number of chars written, or "-1" on error
int fullWrite(unsigned char* data, int nBytes);

// Processes info received by Receiver, sends a reply and if info received is new and ok writes it to packet
// Return number of chars written in packet, or "-1" on error
int processInformationFrame(unsigned char* packet);

// Applies stuffing to byte if needed and stores it in buf
// Return "0" if byte not stuffed and "1" if byte stuffed
int addByteWithStuff(unsigned char byte, unsigned char* buf);

//Initialize stats
void statsConstructor(Statistics* stats);


