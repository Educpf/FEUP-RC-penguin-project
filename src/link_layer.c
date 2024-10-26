// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "stateMachine.h"
#include "utils.h"
#include <stdio.h>
#include "alarm.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

static int timeout;
static int nRetransmissions;
static LinkLayerRole role;

static int frameCount = 0;
static int retransmitionCount = 0; // Ony transmiter
static int timeoutCount = 0; 
static int rejectedCount = 0;

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
    timeout = connectionParameters.timeout;
    nRetransmissions = connectionParameters.nRetransmissions;
    role = connectionParameters.role;


    int STOP = FALSE;
    while (STOP == FALSE)
    {
        switch (role)
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
            switch (setupAlarm(nRetransmissions, timeout))
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

    frameCount++;

    unsigned char dataFrame[bufSize * 2 + 7];

    dataFrame[0] = FLAG;
    dataFrame[1] = AS;
    dataFrame[2] = getFrameNum();
    dataFrame[3] = AS ^ getFrameNum();

    unsigned char bcc = 0;
    unsigned int byteNum = 4;

    for (unsigned int i = 0; i < bufSize; i++)
    {
        unsigned char transferByte = buf[i];

        bcc ^= transferByte;
        byteNum += addByteWithStuff(transferByte, dataFrame[byteNum]) + 1;
        // If its FLag change to ESC ESCAPED_FLAG
    }

    // BCC == FLAG || BCC == ESC
    byteNum += addByteWithStuff(bcc, dataFrame[byteNum]) + 1;
    dataFrame[byteNum++] = FLAG;




    int STOP = FALSE;
    while (STOP == FALSE)
    {
        switch (setupAlarm(nRetransmissions, timeout))
        {
        case 0:
            retransmitionCount++;
            if (fullWrite(dataFrame, byteNum) == -1) return -1;
            break;

        case 1:

            printf("MAXIMUM TENTATIVES REACHED!\n");
            printf("TERMINATING CONNECTION\n");
            return -1;
            break;

        case 2:

            unsigned char byte;
            int nbyte = readByteSerialPort(&byte);
            if (nbyte == -1) return -1;

            printf("Processing a byte\n");
            printf("The state: %d\n", getMachineState());
            printf("The byte: %x\n", byte);

            if (handleByte(byte) == END)
            {
                turnOffAlarm();

                // GOOD INFORMATION RESPONSE
                if (isReadyToReceiveByte(getControlByte())) STOP = TRUE;

                cleanMachineData();
            }
            break;
        }
    }

    return bufSize;
}


// -1 -> error
// 0 -> DISC
// n -> packet size
int llread(unsigned char *packet)
{

    int STOP = FALSE;
    // Process the rest
    while (STOP == FALSE)
    {
        // Verify machine State
        if (getMachineState() == END)
        {
            // INFORMATION FRAME
            if (isInfoControl(getControlByte()))
            {
                return processInformationFrame(packet);
            }

            if (getControlByte() == DISC)
                STOP = TRUE;

            cleanMachineData();
        }
        else
        {
            unsigned char byte;
            int nbytes = readByteSerialPort(&byte);
            if (nbytes == -1)
                return -1;
            if (nbytes)
                handleByte(byte);
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

    int STOP = FALSE;
    while (STOP == FALSE)
    {

        switch(role){

            case (LlRx):
                
                switch (setupAlarm(nRetransmissions, timeout))
                {

                    case 0:
                        unsigned char discFrame[5] = {FLAG, AS, DISC, AS ^ DISC, FLAG};
                        if (fullWrite(discFrame, 5) == -1) return -1;
                    break;

                    case 1:
                        return -1;
                    break;

                    case 2:
                        unsigned char byte;
                        int nbyte = readByteSerialPort(&byte); 
                        if (nbyte == -1) return -1;
                        if (nbyte)
                        {
                            
                            if (handleByte(byte) == END)
                            {
                                turnOffAlarm();
                                if (getControlByte() == UA)
                                {
                                    STOP = TRUE;
                                }
                                cleanMachineData();
                            }
                        }

                    break;



                }           

            break;



            case (LlTx):

                
                switch(setupAlarm(nRetransmissions, timeout))
                {
                    case 0:
                        unsigned char discFrame[5] = {FLAG, AS, DISC, AS ^ DISC, FLAG};
                        if (fullWrite(discFrame, 5) == -1) return -1;
                    break;

                    case 1:
                        printf("MAXIMUM RETRANSMiSSIONs");
                        return -1;                   
                    break;
                    
                    case 2:
                        unsigned char byte;
                        int nbyte = readByteSerialPort(&byte);
                        if (nbyte == -1) return -1;
                        if (nbyte)
                        {
                            
                            if (handleByte(byte) == END)
                            {
                                turnOffAlarm();
                                
                                if (getControlByte() == DISC)
                                {
                                    unsigned char responseFrame[5] = {FLAG, AR, UA, AR ^ UA, FLAG};
                                    if (fullWrite(responseFrame, 5) == -1) return -1;
                                    STOP = TRUE;

                                    // WRITE STATISTICS
                                    if (showStatistics)
                                    {
                                        printf("-- STATISTICS --\n");
                                        printf("Frames : %d\n", frameCount);
                                        printf("Retransmitions : %d\n", retransmitionCount);
                                        printf("Timeouts : %d\n", timeoutCount);
                                        printf("Rejected frames : %d\n", rejectedCount);
                                    }
                                }

                                cleanMachineData();
                            }
                        }
                        
                    break;
                    
                    
                }  
                break;
        }
    }

    int clstat = closeSerialPort();
    return clstat;
}
