#include "utils.h"
#include "serial_port.h"
#include "stateMachine.h"

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


int processInformationFrame(unsigned char* packet)
{
    // REPEATED
    if (!isInfoRepeated())
    {
        // ERROR
        unsigned char Bcc = getMachineData()[getMachineDataSize() - 1];
        unsigned char calculatedBcc = 0;
        for (unsigned int i = 0; i < getMachineDataSize(); i++)
            calculatedBcc ^= getMachineData()[i];
        if (calculatedBcc != Bcc)
        {
            // TODO: WRITE ERROR FRAME
            printf("ERROR! Not valid format\n");
            unsigned char C = REJ0 + (getControlByte() == C_INFO_1);
            unsigned char response[5] = {FLAG, AR, C, AR ^ C, FLAG};
            int bytes = fullWrite(response, 5);
        }
        else
        {
            // OK
            memcpy(finalBuffer + writePosition, getMachineData(), getMachineDataSize() - 1);
            writePosition += getMachineDataSize() - 1; // Exclude Bcc byte
        }
    }

    // Clean up machine
    cleanMachineData();

    // TODO: WRITE OK INFO FRAME

    printf("Asking for next data frame\n");
    unsigned char C = RR0 + (getControlByte() == C_INFO_0);
    unsigned char response[5] = {FLAG, AR, C, AR ^ C, FLAG};
    int bytes = fullWrite(response, 5);
}