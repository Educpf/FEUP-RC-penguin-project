#include "utils.h"
#include "serial_port.h"
#include "stateMachine.h"

#include <stdio.h>
#include <string.h>

extern FILE* file;
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
        unsigned int datasize = getMachineDataSize();
        unsigned char Bcc2 = getMachineData()[datasize - 1];
        unsigned char calculatedBcc = 0;

        // Calculates BCC2
        for (unsigned int i = 0; i < datasize-1; i++){
            calculatedBcc ^= getMachineData()[i];
        }

        // Checks if has error in BCC2
        if (calculatedBcc != Bcc2)
        {
            fprintf(file,"ERROR in BCC2\n");
            unsigned char C = REJ0 + (getControlByte() == C_INFO_1);
            unsigned char response[5] = {FLAG, AS, C, AS ^ C, FLAG};
            if (fullWrite(response, 5) == -1) return -1;
            invertControlByte();
            cleanMachineData();
        }
        else
        {
            memcpy(packet, getMachineData(), datasize - 1);

            fprintf(file,"Asking for next data frame(OKOKOKO)\n");
            unsigned char C = RR0 + (getControlByte() == C_INFO_0);
            unsigned char response[5] = {FLAG, AS, C, AS ^ C, FLAG};
            if (fullWrite(response, 5) == -1){
                return -1;
            }
            cleanMachineData();
            return (datasize - 1);
        }
    }
    else
    {
        fprintf(file,"Asking for next data frame(REPEATED)\n");
        unsigned char C = RR0 + (getControlByte() == C_INFO_0);
        unsigned char response[5] = {FLAG, AS, C, AS ^ C, FLAG};
        if (fullWrite(response, 5) == -1)
            return -1;
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