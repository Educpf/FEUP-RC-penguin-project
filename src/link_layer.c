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
static int retransmitionCount = 0; // Only transmiter
static int timeoutCount = 0;
static int rejectedCount = 0;
FILE *file;

int llopen(LinkLayer connectionParameters)
{

    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        printf("Unable to Open Serial Port (Link Layer)\n");
        return -1;
    }

    timeout = connectionParameters.timeout;
    nRetransmissions = connectionParameters.nRetransmissions;
    role = connectionParameters.role;

    // Opens file to store data to better understand errors
    file = fopen("output.txt", "w"); //change name ahabsknqifbnefc
    if (file == NULL)
    {
        perror("Error opening file");
    }

    int STOP = FALSE;
    while (STOP == FALSE)
    {
        switch (role)
        {

        case (LlRx):
            unsigned char byte;
            int nbytes = readByteSerialPort(&byte);
            if (nbytes == -1)
            {
                printf("Error when reading from Serial Port (Open - Receiver)\n");
                return -1;
            }
            if (nbytes)
            {
                printf("New byte:  %x\n", byte);
                printf("The current state -> %d", getMachineState());

                // End of frame reached
                if (handleByte(byte) == END)
                {
                    printf("Reached the end\n");

                    // SET Frame Received
                    if (getControlByte() == SET)
                    {
                        printf("Found set frame!\n");
                        unsigned char response[5] = {FLAG, AS, UA, AS ^ UA, FLAG};
                        if (fullWrite(response, 5) == -1)
                        {
                            printf("Error Sending UA (Open - Receiver)\n");
                            return -1;
                        }
                    }
                    // Information Frame Received! Connection is finished
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

                // Send SET Frame
                unsigned char setFrame[5] = {FLAG, AS, SET, AS ^ SET, FLAG};
                if (fullWrite(setFrame, 5) == -1)
                {
                    printf("Error Sending SET (Open - Transmitter)\n");
                    return -1;
                }
                break;

            case 1:

                printf("MAXIMUM TENTATIVES REACHED! (Open - Transmitter)\n");
                return -1;
                break;

            case 2:

                // Waits response to SET
                unsigned char byte;
                int nbyte = readByteSerialPort(&byte);
                if (nbyte == -1)
                {
                    printf("Error when reading from Serial Port (Open - Transmitter)\n");
                    return -1;
                }
                if (nbyte)
                {
                    printf("Processing a byte\n");
                    printf("The state: %d\n", getMachineState());
                    printf("The byte: %x\n", byte);

                    // End of frame reached
                    if (handleByte(byte) == END)
                    {
                        // SET Response
                        if (getControlByte() == UA)
                        {
                            turnOffAlarm();
                            printf("Machines Connected!!\n");
                            STOP = TRUE;
                            cleanMachineData();
                        }
                    }
                }
                break;
            }
            break;
        }
    }
    return 1;
}

int llwrite(const unsigned char *buf, int bufSize)
{

    frameCount++; // Est PERdido

    unsigned char dataFrame[bufSize * 2 + 7];

    dataFrame[0] = FLAG;
    dataFrame[1] = AS;
    dataFrame[2] = getFrameNum();
    dataFrame[3] = AS ^ getFrameNum();

    unsigned char bcc = 0;
    unsigned int byteNum = 4;

    // Iterates through buffer and stores stuffed bytes in dataFrame
    for (unsigned int i = 0; i < bufSize; i++)
    {
        unsigned char transferByte = buf[i];
        bcc ^= transferByte;
        // Applies byte stuffing
        byteNum += addByteWithStuff(transferByte, dataFrame + byteNum) + 1;
    }

    // Applies byte stuffing to BCC2
    byteNum += addByteWithStuff(bcc, dataFrame + byteNum) + 1;
    dataFrame[byteNum++] = FLAG;

    for (int i = 0; i < byteNum; i++)
    {
        printf("%x - ", dataFrame[i]);
    }

    int STOP = FALSE;
    while (STOP == FALSE)
    {
        switch (setupAlarm(nRetransmissions, timeout))
        {

        case 0:

            retransmitionCount++; // Est
            // Sends dataFrame to Serial Port
            if (fullWrite(dataFrame, byteNum) == -1)
            {
                printf("Error Sending Information Frame (Link Layer - Write)\n");
                return -1;
            }
            break;

        case 1:

            printf("MAXIMUM TENTATIVES REACHED! (Link Layer - Write)\n");
            return -1;
            break;

        case 2:
            
            // Waits response to Information Frame sended
            unsigned char byte;
            int nbyte = readByteSerialPort(&byte);
            if (nbyte == -1)
            {
                printf("Error when reading from Serial Port (Link Layer - Write)\n");
                return -1;
            }
            if (nbyte)
            {
                printf("Processing a byte\n");
                printf("The state: %d\n", getMachineState());
                printf("The byte: %x\n", byte);

                // End of frame reached
                if (handleByte(byte) == END)
                {
                    turnOffAlarm();
                    // GOOD INFORMATION RESPONSE
                    if (isReadyToReceiveByte(getControlByte()))
                    {
                        int requestedFrame = receiveToSendControlByte(getControlByte());
                        // Assures that requested frame is really the supposed to send
                        if (requestedFrame != getFrameNum())
                        {
                            STOP = TRUE;
                            invertFrameNum();
                        }
                    }
                    cleanMachineData();
                }
            }
            break;
        }
    }
    return bufSize;
}


int llread(unsigned char *packet)
{

    int STOP = FALSE;
    while (STOP == FALSE)
    {
        // Verify Machine State
        if (getMachineState() == END)
        {
            // Information Frame
            if (isInfoControl(getControlByte()))
            {
                int bytesRead = processInformationFrame(packet);
                if(bytesRead == -1){
                    printf("Error when processing received data (Link Layer - Read)\n");
                    return -1;
                }
                if (bytesRead != 0){
                    return bytesRead;
                }
            }
            cleanMachineData();
        }
        else
        {
            //Reads and handles byte from Serial Port
            unsigned char byte;
            int nbytes = readByteSerialPort(&byte);
            if (nbytes == -1)
            {
                printf("Error when reading from Serial Port (Link Layer - Read)\n");
                return -1;
            }
            if (nbytes)
            {
                // Writes to file in order to better understand errors
                fprintf(file, "New byte:  %x\n", byte);
                fprintf(file, "The current state -> %d", getMachineState());

                handleByte(byte);
            }
        }
    }
    return -1; //Acho que sim
}


int llclose(int showStatistics)
{
    int disc = 0; // Controls reception of DISC (Only necessary to Receiver)
    int STOP = FALSE;
    while (STOP == FALSE)
    {
        switch (role)
        {

        case (LlRx):
            // Waiting DISC from Transmitter
            if (disc == 0)
            {
                unsigned char byte;
                int nbyte = readByteSerialPort(&byte);
                if (nbyte == -1)
                {
                    printf("Error when reading from Serial Port (Close - Receiver(DISC))\n");
                    return -1;
                }
                if (nbyte)
                {
                    printf("New byte:  %x\n", byte);
                    printf("The current state -> %d\n", getMachineState());

                    // End of frame reached
                    if (handleByte(byte) == END)
                    {
                        // Receives DISC
                        if (getControlByte() == DISC)
                        {
                            disc = 1;
                        }
                        cleanMachineData();
                    }
                }
            }
            else
            {
                switch (setupAlarm(nRetransmissions, timeout))
                {

                case 0:

                    // Sends DISC Frame to Transmitter
                    unsigned char discFrame[5] = {FLAG, AR, DISC, AR ^ DISC, FLAG}; 
                    printf("SENDING DISC FROM RECEIVER");
                    if (fullWrite(discFrame, 5) == -1)
                    {
                        printf("Error Sending DISC (Close - Receiver)\n");
                        return -1;
                    }
                    break;

                case 1:

                    printf("MAXIMUM TENTATIVES REACHED! (Close - Receiver)\n");
                    return -1;
                    break;

                case 2:

                    //Waits UA from Transmitter
                    unsigned char byte;
                    int nbyte = readByteSerialPort(&byte);
                    if (nbyte == -1)
                    {
                        printf("Error when reading from Serial Port (Close - Receiver(UA))\n");
                        return -1;
                    }
                    if (nbyte)
                    {
                        // End of frame reached
                        if (handleByte(byte) == END)
                        {
                            turnOffAlarm();
                            // Received UA -> Ends Progrma
                            if (getControlByte() == UA)
                            {
                                STOP = TRUE;
                            }
                            cleanMachineData();
                        }
                    }
                    break;
                }
            }
            break;

        case (LlTx):

            switch (setupAlarm(nRetransmissions, timeout))
            {
    
            case 0:
                // Sends DISC Frame to Receiver
                unsigned char discFrame[5] = {FLAG, AS, DISC, AS ^ DISC, FLAG};
                printf("SENDING DISC FROM TRANSM");
                if (fullWrite(discFrame, 5) == -1)
                {
                    printf("Error Sending DISC (Close - Transmitter)\n");
                    return -1;
                }
                break;

            case 1:

                printf("MAXIMUM TENTATIVES REACHED! (Close - Transmitter)\n");
                return -1;
                break;

            case 2:

                unsigned char byte;
                int nbyte = readByteSerialPort(&byte);
                if (nbyte == -1)
                {
                    printf("Error when reading from Serial Port (Close - Transmitter)\n");
                    return -1;
                }
                if (nbyte)
                {
                    // End of frame reached
                    if (handleByte(byte) == END)
                    {
                        turnOffAlarm();
                        // DISC Received Sends UA
                        if (getControlByte() == DISC)
                        {
                            unsigned char responseFrame[5] = {FLAG, AR, UA, AR ^ UA, FLAG};
                            if (fullWrite(responseFrame, 5) == -1)
                            {
                            printf("Error Sending UA (Close - Transmitter)\n");
                                return -1;
                            }
                            STOP = TRUE;

                            // Show Statistics
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
    // Closes file
    fclose(file);
    // Closes Serial Port
    int clstat = closeSerialPort();
    if(clstat == -1){
        printf("Unable to close Serial Port (Link Layer)\n");
    }
    return clstat;
}
