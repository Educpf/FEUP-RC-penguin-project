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

Statistics stats;

extern FILE *outputPackets;
extern FILE *packetT;

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

    statsConstructor(&stats);



    int STOP = FALSE;
    while (STOP == FALSE)
    {
        switch (role)
        {

        case (LlRx):
        {
            unsigned char byte;
            int nbytes = readByteSerialPort(&byte);
            if (nbytes == -1)
            {
                printf("Error when reading from Serial Port (Open - Receiver)\n");
                return -1;
            }
            else if (nbytes)
            {

                // End of frame reached
                if (handleByte(byte) == END)
                {
                    stats.frameCount++;
                    // SET Frame Received
                    if (getControlByte() == SET)
                    {
                        unsigned char response[5] = {FLAG, AS, UA, AS ^ UA, FLAG};
                        if (fullWrite(response, 5) == -1)
                        {
                            printf("Error Sending UA (Open - Receiver)\n");
                            return -1;
                        }
                        printf("SET Frame Received - Sending UA Frame\n");

                        stats.approvedCount++;
                        cleanMachineData();
                    }
                    // Information Frame Received! Connection is finished
                    else if (isInfoControl(getControlByte()))
                    {
                        stats.frameCount--;
                        STOP = TRUE;
                    }
                    // RESET MACHINE STATE AND CLEAR DATA IF RECEIVED SET FRAME
                    else
                    {
                        stats.strangeCount++;
                        cleanMachineData();
                    }
                }
            }
            break;
        }
        case (LlTx):

            switch (setupAlarm(nRetransmissions, timeout))
            {

            case 0:
            {
                // Send SET Frame
                unsigned char setFrame[5] = {FLAG, AS, SET, AS ^ SET, FLAG};
                if (fullWrite(setFrame, 5) == -1)
                {
                    printf("Error Sending SET (Open - Transmitter)\n");
                    return -1;
                }
                printf("Sending SET Frame\n");
                stats.frameCount++;
                break;
            }
            case 1:
            {
                printf("MAXIMUM TENTATIVES REACHED! (Open - Transmitter)\n");
                return -1;
                break;
            }
            case 2:
            {
                // Waits UA Response
                unsigned char byte;
                int nbyte = readByteSerialPort(&byte);
                if (nbyte == -1)
                {
                    printf("Error when reading from Serial Port (Open - Transmitter)\n");
                    return -1;
                }
                else if (nbyte)
                {

                    // End of frame reached
                    if (handleByte(byte) == END)
                    {
                        // SET Response
                        if (getControlByte() == UA)
                        {
                            turnOffAlarm();
                            printf("UA Frame Received\n");
                            printf("Machines Connected!!\n");
                            STOP = TRUE;
                            cleanMachineData();
                        }
                        else
                        {
                            stats.strangeCount++;
                        }
                    }
                }
                break;
            }
            }
            break;
        }
    }
    return 1;
}

int llwrite(const unsigned char *buf, int bufSize)
{

    unsigned char dataFrame[bufSize * 2 + 7];

    dataFrame[0] = FLAG;
    dataFrame[1] = AS;
    dataFrame[2] = (unsigned char)getFrameNum();
    dataFrame[3] = (unsigned char)(AS ^ getFrameNum());

    unsigned char bcc = 0;
    int byteNum = 4;

    // Iterate through buffer and store stuffed bytes in dataFrame
    for (int i = 0; i < bufSize; i++)
    {
        unsigned char transferByte = buf[i];
        bcc ^= transferByte;
        // Apply byte stuffing
        byteNum += addByteWithStuff(transferByte, dataFrame + byteNum) + 1;
    }

    // Apply byte stuffing to BCC2
    byteNum += addByteWithStuff(bcc, dataFrame + byteNum) + 1;
    dataFrame[byteNum++] = FLAG;

    fprintf(packetT,"\n\n%x ",bcc);

    int STOP = FALSE;
    while (STOP == FALSE)
    {
        switch (setupAlarm(nRetransmissions, timeout))
        {

        case 0:
        {
            // Sends dataFrame to Serial Port
            if (fullWrite(dataFrame, byteNum) == -1)
            {
                printf("Error Sending Information Frame (Link Layer - Write)\n");
                return -1;
            }
            printf("Sending Data Frame(%d bytes)\n",byteNum);
            stats.frameCount++;
            break;
        }
        case 1:
        {
            printf("MAXIMUM TENTATIVES REACHED! (Link Layer - Write)\n");
            return -1;
            break;
        }
        case 2:
        {
            // Waits response to Information Frame sended
            unsigned char byte;
            int nbyte = readByteSerialPort(&byte);
            if (nbyte == -1)
            {
                printf("Error when reading from Serial Port (Link Layer - Write)\n");
                return -1;
            }
            else if (nbyte)
            {
                // End of frame reached
                if (handleByte(byte) == END)
                {
                    turnOffAlarm();
                    if (isRejectionByte(getControlByte()))
                    {
                        stats.rejectedCount++;

                    }  // GOOD INFORMATION RESPONSE
                    else if (isReadyToReceiveByte(getControlByte()))
                    {
                        int requestedFrame = (int)receiveToSendControlByte(getControlByte());

                        // Assure that requested frame is really the supposed to send
                        if (requestedFrame != getFrameNum())
                        {
                            STOP = TRUE;
                            invertFrameNum();
                        }
                    }
                    else
                    {
                        stats.strangeCount++;
                    }
                    cleanMachineData();
                }
            }
            break;
        }
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
            stats.frameCount++;
            // Information Frame
            if (isInfoControl(getControlByte()))
            {
                int bytesRead = processInformationFrame(packet);
                if (bytesRead == -1)
                {
                    printf("Error when processing received data (Link Layer - Read)\n");
                    return -1;
                }
                if (bytesRead != 0)
                {
                    return bytesRead;
                }
            }
            else
            {
                stats.strangeCount++;
            }
            cleanMachineData();
        }
        else
        {
            // Read and handle bytes from Serial Port
            unsigned char byte;
            int nbytes = readByteSerialPort(&byte);
            if (nbytes == -1)
            {
                printf("Error when reading from Serial Port (Link Layer - Read)\n");
                return -1;
            }
            else if (nbytes)
            {

                handleByte(byte);
            }
        }
    }
    return -1;
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
        { 
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
                    // End of frame reached
                    if (handleByte(byte) == END)
                    {
                        stats.frameCount++;
                        // Receives DISC
                        if (getControlByte() == DISC)
                        {
                            disc = 1;
                        }
                        else{
                            stats.strangeCount++;
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
                {
                    // Sends DISC Frame to Transmitter
                    unsigned char discFrame[5] = {FLAG, AR, DISC, AR ^ DISC, FLAG};
                    if (fullWrite(discFrame, 5) == -1)
                    {
                        printf("Error Sending DISC (Close - Receiver)\n");
                        return -1;
                    }
                    printf("DISC Frame Received - Sending DISC Frame\n");
                    stats.approvedCount++;
                    break;
                }

                case 1:
                {
                    printf("Receiver couldn't receive UA");
                    printf("MAXIMUM TENTATIVES REACHED! (Close - Receiver)\n");
                    return -1;
                    break;
                }
                case 2:
                {
                    // Wait UA from Transmitter
                    unsigned char byte;
                    int nbyte = readByteSerialPort(&byte);
                    if (nbyte == -1)
                    {
                        printf("Error when reading from Serial Port (Close - Receiver(UA))\n");
                        return -1;
                    }
                    else if (nbyte)
                    {
                        // End of frame reached
                        if (handleByte(byte) == END)
                        {
                            stats.frameCount++;
                            turnOffAlarm();
                            // Received UA -> Ends Progrma
                            if (getControlByte() == UA)
                            {
                                STOP = TRUE;
                                printf("UA Frame Received\n");
                                printf("TERMINATING...\n");
                                // Show Statistics
                                if (showStatistics)
                                {
                                    printf("-- STATISTICS --\n");
                                    printf("Number of Frames Received: %d\n", stats.frameCount);
                                    printf("Number of Timeouts: %d\n", stats.timeoutCount);
                                    printf("Number of Approved Frames: %d\n", stats.approvedCount);
                                    printf("Number of Rejected Frames: %d\n", stats.rejectedCount);
                                    printf("Number of Repeated Frames: %d\n", stats.repeatedCount);
                                    printf("Number of Strange Frames: %d\n", stats.strangeCount);
                                }
                            }
                            else
                            {
                                stats.strangeCount++;
                            }
                            cleanMachineData();
                        }
                    }
                    break;
                }
                }
            }
            break;
        }

        case (LlTx):
        {
            switch (setupAlarm(nRetransmissions, timeout))
            {

            case 0:
            { 
                // Send DISC Frame to Receiver
                unsigned char discFrame[5] = {FLAG, AS, DISC, AS ^ DISC, FLAG};
                if (fullWrite(discFrame, 5) == -1)
                {
                    printf("Error Sending DISC (Close - Transmitter)\n");
                    return -1;
                }
                printf("Sending DISC Frame\n");
                stats.frameCount++;
                break;
            }

            case 1:
            {
                printf("MAXIMUM TENTATIVES REACHED! (Close - Transmitter)\n");
                return -1;
                break;
            }

            case 2:
            {
                unsigned char byte;
                int nbyte = readByteSerialPort(&byte);
                if (nbyte == -1)
                {
                    printf("Error when reading from Serial Port (Close - Transmitter)\n");
                    return -1;
                }
                else if (nbyte)
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
                            printf("DISC Frame Received - Sending UA Frame\n");
                            stats.frameCount++;
                            STOP = TRUE;

                            // Show Statistics
                            if (showStatistics)
                            {
                                printf("-- STATISTICS --\n");
                                printf("Number of Frames Sent: %d\n", stats.frameCount);
                                printf("Number of Timeouts: %d\n", stats.timeoutCount);
                                printf("Number of Rejected Frames: %d\n", stats.rejectedCount);
                                printf("Number of Strange Frames: %d\n", stats.strangeCount);
                            }
                        }
                        else{
                            stats.strangeCount++;
                        }
                        cleanMachineData();
                    }
                }
                break;
            }
            }
            break;
        }
        }
    }

    // Close Serial Port
    int clstat = closeSerialPort();
    if (clstat == -1)
    {
        printf("Unable to close Serial Port (Link Layer)\n");
    }
    return clstat;
}
