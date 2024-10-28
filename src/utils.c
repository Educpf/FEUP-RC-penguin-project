#include "utils.h"
#include "serial_port.h"
#include "stateMachine.h"

#include <stdio.h>
#include <string.h>

extern FILE* file;
extern Statistics statsData;
//HHHHHHHHHHHHHHH
int fullWrite(unsigned char *data, int nBytes)
{
    int nBytesWritten = 0;
    while (nBytesWritten < nBytes)
    {
        int nbytes = writeBytesSerialPort(data + nBytesWritten, nBytes - nBytesWritten);

        if (nbytes == -1){
            printf("Error when writing to Serial Port");
            return -1;
        }

        nBytesWritten += nbytes;
    }

    return 0;
}

// -1 -> error
// n -> bytes written to packet      HHHHHHHHHHHHHHHHHHHHH
int processInformationFrame(unsigned char *packet)
{
    // If Repeated does not calculate again
    if (isInfoRepeated() == 0)
    {
        int datasize = getMachineDataSize();
        unsigned char Bcc2 = getMachineData()[datasize - 1];
        unsigned char calculatedBcc = 0;

        // Calculates BCC2
        for (int i = 0; i < datasize-1; i++){
            calculatedBcc ^= getMachineData()[i];
        }

        // Checks if has error in BCC2
        if (calculatedBcc != Bcc2)
        {
            fprintf(file,"ERROR in BCC2\n");
            // Sends REJ Frame
            unsigned char C = REJ0 + (getControlByte() == C_INFO_1);
            unsigned char response[5] = {FLAG, AS, C, AS ^ C, FLAG};
            if (fullWrite(response, 5) == -1)
            {
                printf("Error Sending REJ Frame (Link Layer - Read)\n");
                return -1;
            }
            statsData.rejectedCount++;
            invertControlByte();
            cleanMachineData();
        }
        else
        {
            memcpy(packet, getMachineData(), (size_t)(datasize - 1));
            // Sends RR Frame
            fprintf(file,"Asking for next data frame(OKOKOKOK)\n");
            unsigned char C = RR0 + (getControlByte() == C_INFO_0);
            unsigned char response[5] = {FLAG, AS, C, AS ^ C, FLAG};
            if (fullWrite(response, 5) == -1)
            {
                printf("Error Sending RR Frame (Link Layer - Read)\n");
                return -1;
            }
            statsData.approvedCount++;
            cleanMachineData();
            return (datasize - 1);
        }
    }
    else
    {   
        // Sends RR Frame (case info repeated)
        fprintf(file,"Asking for next data frame(REPEATED)\n");
        unsigned char C = RR0 + (getControlByte() == C_INFO_0);
        unsigned char response[5] = {FLAG, AS, C, AS ^ C, FLAG};
        if (fullWrite(response, 5) == -1)
        {
            printf("Error Sending RR Frame - Repeated (Link Layer - Read)\n");
            return -1;
        }
        statsData.repeatedCount++;
        cleanMachineData();

    }
    return 0;
}



// 0 -> not stuffed  HHHHHHHHHHHHHh
// 1 -> stuffed
int addByteWithStuff(unsigned char byte, unsigned char *buf)
{
    // Stuffing FLAG byte -> ESC (FLAG^0x20)
    if (byte == FLAG)
    {
        *buf++ = ESC;
        *buf = ESCAPED_FLAG;
        return 1;
    }
    // Stuffing ESC byte -> ESC (ESC^0x20)
    if (byte == ESC)
    {
        *buf++ = ESC;
        *buf = ESCAPED_ESC;
        return 1;
    }
    // Does not need stuffing
    *buf = byte;
    return 0;
}


void statsConstructor(Statistics* stats){
    stats->frameCount = 0;
    stats->timeoutCount = 0;
    stats->rejectedCount = 0;
    stats->approvedCount = 0;
    stats->repeatedCount = 0;
}
