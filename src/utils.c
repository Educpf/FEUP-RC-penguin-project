#include "utils.h"
#include "serial_port.h"
#include "stateMachine.h"

#include <stdio.h>
#include <string.h>

int fullWrite(unsigned char *data, int nBytes)
{
    int nBytesWritten = 0;
    while (nBytesWritten < nBytes)
    {
        int nbytes = writeBytesSerialPort(data + nBytesWritten, nBytes - nBytesWritten);
        if (nbytes == -1)
            return -1;
        nBytesWritten += nbytes;
    }

    return 0;
}

// -1 -> error
// n -> bytes written to packet
int processInformationFrame(unsigned char *packet)
{
    // REPEATED
    if (isInfoRepeated() == 0)
    {
        // ERROR
        unsigned char Bcc = getMachineData()[getMachineDataSize() - 1];
        unsigned char calculatedBcc = 0;
        for (unsigned int i = 0; i < getMachineDataSize(); i++)
            calculatedBcc ^= getMachineData()[i];

        if (calculatedBcc != Bcc)
        {
            printf("ERROR in BCC2\n");
            unsigned char C = REJ0 + (getControlByte() == C_INFO_1);
            unsigned char response[5] = {FLAG, AS, C, AS ^ C, FLAG};
            if (fullWrite(response, 5) == -1)
                return -1;
        }
        else
        {
            // OK
            memcpy(packet, getMachineData(), getMachineDataSize() - 1);

            printf("Asking for next data frame\n");
            unsigned char C = RR0 + (getControlByte() == C_INFO_0);
            unsigned char response[5] = {FLAG, AS, C, AS ^ C, FLAG};
            if (fullWrite(response, 5) == -1)
                return -1;

            return getMachineDataSize() - 1;
        }
    }
    else
    {
        printf("Asking for next data frame\n");
        unsigned char C = RR0 + (getControlByte() == C_INFO_0);
        unsigned char response[5] = {FLAG, AS, C, AS ^ C, FLAG};
        if (fullWrite(response, 5) == -1)
            return -1;
    }
    return 0;
}



// 0 -> not stuffed
// 1 -> stuffed
int addByteWithStuff(unsigned char byte, unsigned char *buf)
{
    if (byte == FLAG)
    {
        *buf++ = ESC;
        *buf = ESCAPED_FLAG;
        return 1;
    }
    // If its ESC change to ESC ESCAPED_ESC
    if (byte == ESC)
    {
        *buf++ = ESC;
        *buf = ESCAPED_ESC;
        return 1;
    }
   
    *buf = byte;
    return 0;
}