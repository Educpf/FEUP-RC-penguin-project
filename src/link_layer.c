// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "stateMachine.h"
#include "utils.h"
#include <stdio.h>
#include "alarm.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        return -1;
    }

    int STOP = FALSE;
    while (STOP == FALSE)
    {
        switch (connectionParameters.role)
        {

        case (LlRx):
            unsigned char byte;
            int nbytes = readByteSerialPort(&byte);
            if (nbytes == -1)
                return -1;
            if (nbytes)
            {
                printf("New byte:  %x\n", byte);
                printf("The current state -> %d", getMachineState());

                // If end of frame reached
                if (handleByte(byte) == END)
                {
                    printf("REached the end\n");

                    // SET FRAME
                    if (getControlByte() == SET)
                    {
                        printf("Found set frame!\n");
                        unsigned char response[5] = {FLAG, AS, UA, AS ^ UA, FLAG};
                        if (fullWrite(response, 5) == -1)
                            return -1;
                    }

                    // Information frame! Connection is finished
                    if (isInfoControl(getControlByte()))
                        STOP = TRUE;
                    // RESET MACHINE STATE AND CLEAR DATA IF RECEIVED SET FRAME
                    else
                        cleanMachineData();
                }
            }

            break;
        case (LlTx):
            switch (setupAlarm(connectionParameters.nRetransmissions, connectionParameters.timeout))
            {
            case 0:
                unsigned char setFrame[5] = {FLAG, AS, SET, AS ^ SET, FLAG};
                if (fullWrite(setFrame, 5) == -1)
                    return -1;
                break;
            case 1:
                printf("MAXIMUM TENTATIVES REACHED!\n");
                printf("TERMINATING CONNECTION\n");
                return -1;
                break;
            case 2:
                unsigned char byte;
                int nbyte = readByteSerialPort(&byte);
                if (nbyte == -1)
                    return -1;
                if (nbyte)
                {
                    printf("Processing a byte\n");
                    printf("The state: %d\n", getMachineState());
                    printf("The byte: %x\n", byte);

                    if (handleByte(byte) == END)
                    {
                        // SET RESPONSE
                        if (getControlByte() == UA)
                        {
                            turnOffAlarm();
                            printf("CONNECTED!!!!!!\n");
                            STOP = TRUE;
                        }
                        // TERMINATE CONNECTION
                    }
                }
                break;
            }
            break;
        }
    }
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

// Maybe use this in order to be able to
void receiverHandleInfoFrame()
{
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // Start by processing bytes read already from open

    // PROCESS INFORMATION FRAME
    processInformationFrame(packet);


    int STOP = FALSE;
    // Process the rest
    while (STOP == FALSE)
    {
        unsigned char byte;
        int nbytes = readByteSerialPort(&byte);
        if (nbytes == -1) return -1;
        if (nbytes)
        {
            if (handleByte(byte) == END)
            {
                // INFORMATION FRAME
                if (isInfoControl(getControlByte()))
                {
                    processInformationFrame(packet);
                }

            }
        }
    }

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}
