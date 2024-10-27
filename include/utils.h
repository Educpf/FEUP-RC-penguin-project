#pragma once



int fullWrite(unsigned char* data, int nBytes);

int processInformationFrame(unsigned char* packet);

int addByteWithStuff(unsigned char byte, unsigned char* buf);