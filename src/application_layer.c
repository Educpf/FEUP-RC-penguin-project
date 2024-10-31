// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <string.h>

#define MAX_SEQUENCE_NUMBER 99


int fileSizeReceivedStart = 0;
unsigned char filenameReceivedStart[MAX_PAYLOAD_SIZE];


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // Open Connection
    LinkLayerRole actualRole = role[0] == 'r' ? LlRx : LlTx;
    LinkLayer linkLayer;
    strncpy(linkLayer.serialPort, serialPort, sizeof(linkLayer.serialPort) - 1);
    linkLayer.role = actualRole;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;
    int currentSequenceNumber = 0;

    if (llopen(linkLayer) == -1)
    {
        printf("Unable to connect (Application Layer)\n");
        return;
    }

    unsigned char packet[MAX_PAYLOAD_SIZE];
    FILE *outputFile;
    FILE *outputPackets;
    FILE *inputFile;
    FILE *packetT;

    int STOP = FALSE;

    switch (actualRole)
    {
    case LlRx:
    {
        // Opens Files
        outputFile = fopen(filename, "w");
        if (outputFile == NULL)
        {
            perror("Error opening output file");
        }

        outputPackets = fopen("PacketsReceiver.txt", "w");
        if (outputPackets == NULL)
        {
            perror("Error opening PacketsReceiver file");
        }

        // Reading and handle packets
        while (STOP == FALSE)
        {

            int nbytes = llread(packet);
            if (nbytes == -1)
            {
                printf("Error when reading (Application Layer - Receiver)\n");
                return;
            }

            if (nbytes == 0)
            {
                printf("Not supposed to be reading, no information received\n");
                return;
            }

            if (nbytes > 0)
            {
                // Decompose and get control field
                unsigned char controlField = packet[0];

                switch (controlField)
                {

                case 1:
                {
                    for(int l = 0; l < nbytes; l++){
                        printf("%x-",packet[l]);
                    }
                    // Store filename and file size to compare in the end
                    int i = 1;  
                    while (i < nbytes)
                    {
                        unsigned char type = packet[i++];
                        unsigned char length = packet[i++];
                        if (type == 0)
                        {
                            memcpy(&fileSizeReceivedStart, packet + i, length);
                            i += (int)length;
                        }
                        else if (type == 1)
                        {
                            memcpy(filenameReceivedStart, packet + i, length);
                            filenameReceivedStart[length] = '\0';
                            printf("filename(start):%s",filenameReceivedStart);
                            i += (int)length;
                        }
                    }
                    break;
                }

                case 2:
                {
                    // Unpack values and write to file
                    unsigned char sequenceNumber = packet[1];
                    unsigned char l2 = packet[2];
                    unsigned char l1 = packet[3];

                    int k = 256 * (int)l2 + (int)l1;
                    int bytesWritten = 0;

                    if ((bytesWritten = (int)fwrite(packet + 4, 1,(size_t)k, outputFile)) < k)
                    {
                        printf("Error writing to file in Sequence Number: %u\n", sequenceNumber);
                        printf("[Expectations/Reality] %d/%d\n", k, bytesWritten);
                    }

                    // Writing to file PacketsReceiver in order to better understand errors
                    fprintf(outputPackets, "\n\nPACKET %d: ", packet[1]);
                    for (int d = 4; d < k + 4; d++)
                    {
                        fprintf(outputPackets, "%x ", packet[d]);
                    }

                    break;
                }

                case 3:
                {
                    // Verify if filename and file size from end is equal to start
                    int j = 1;
                    unsigned char filenameReceivedEnd[MAX_PAYLOAD_SIZE];
                    int fileSizeReceivedEnd = 0;
                    while (j < nbytes)
                    {
                        unsigned char type = packet[j++];
                        unsigned char length = packet[j++];
                        if (type == 0)
                        {
                            memcpy(&fileSizeReceivedEnd, packet + j, length);
                            j += (int)length;

                            if (fileSizeReceivedStart != fileSizeReceivedEnd)
                            {
                                printf("Size different in control packet start and control packet end");
                                return;
                            }
                        }
                        else if (type == 1)
                        {
                            memcpy(filenameReceivedEnd, packet + j, length);
                            filenameReceivedEnd[length] = '\0';
                            printf("filename(end):%s",filenameReceivedEnd);
                            j += (int)length;
                        }
                    }
                    if (strcmp((const char *)filenameReceivedStart, (const char *)filenameReceivedEnd) != 0)
                    {
                    printf("Filename different in control packet start and control packet end");
                    return;
                    }
                    // Breaks loop since it has reached final packet
                    STOP = TRUE;
                    break;
                }

                default:
                {
                    printf("Error!! Wrong CONTROL FIELD in packet\n");
                    break;
                }
                }
            }
            
        }

        // Closes Files
        fclose(outputFile);
        fclose(outputPackets);
        break;
    }

    case LlTx:
    {
        // Opens Files
        inputFile = fopen(filename, "rb");
        if (inputFile == NULL)
        {
            perror("Error opening input file");
        }

        packetT = fopen("PacketsTransmitter.txt", "w");
        if (packetT == NULL)
        {
            perror("Error opening PacketsTransmitter file");
        }

        // Get File Size
        fseek(inputFile, 0, SEEK_END);
        long int fileSize = ftell(inputFile);
        fseek(inputFile, 0, SEEK_SET);


        // CONTROL PACKET (Start)
        packet[0] = 1;

        // File Size
        packet[1] = 0;
        packet[2] = sizeof(fileSize);
        memcpy(packet + 3, &fileSize, packet[2]);

        // Filename
        packet[11] = 1;
        packet[12] = (unsigned char)strlen(filename)+1;
        memcpy(packet + 13, filename, packet[12]);

        printf("Sending packet of %d bytes\n", packet[2] + packet[12] + 5);
        if (llwrite(packet, packet[2] + packet[12] + 5) == -1)
        {
            printf("Error when writting (Application Layer - Transmitter - Control Packet 1)\n");
            return;
        }

        // DATA PACKET

        packet[0] = 2;

        int bytesRead = 0;
        while ((bytesRead = (int)fread(packet + 4, 1, (size_t)(MAX_PAYLOAD_SIZE - 4), inputFile)) > 0)
        {
            printf("Sending packet of %d bytes\n", bytesRead);
            packet[1] = (unsigned char)(currentSequenceNumber++ % (MAX_SEQUENCE_NUMBER + 1)); // Assure sequence number between 0-99
            packet[2] = (unsigned char)(bytesRead / 256);
            packet[3] = (unsigned char)(bytesRead % 256);
            if (llwrite(packet, bytesRead + 4) == -1)
            {
                printf("Error when writting (Application Layer - Transmitter - Data Packet)\n");
                return;
            }

            // Writing to file PacketsTransmitter in order to better understand errors
            fprintf(packetT, "\n\nPACKET %d: ", packet[1]);
            for (int i = 4; i < bytesRead + 4; i++)
            {
                fprintf(packetT, "%x ", packet[i]);
            }
        }


        // CONTROL PACKET (END)
        packet[0] = 3;

        // File Size
        packet[1] = 0;
        packet[2] = sizeof(fileSize);
        memcpy(packet + 3, &fileSize, packet[2]);

        // Filename
        packet[11] = 1;
        packet[12] = (unsigned char)strlen(filename)+1;
        memcpy(packet + 13, filename, packet[12]);

        if (llwrite(packet, packet[2] + packet[12] + 5) == -1)
        {
            printf("Error when writting (Application Layer - Transmitter - Control Packet 3)\n");
            return;
        }

        // Closes Files
        fclose(inputFile);
        fclose(packetT);
        break;
    }
    }

    // Closes Connection
    if (llclose(1) == -1)
    {
        printf("Unable to close connection (Application Layer)\n");
        return;
    }
}
