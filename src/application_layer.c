// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    printf("Hi world!\n");
    // Open Connection
    LinkLayerRole actualRole = role[0] == 'r' ? LlRx : LlTx;
    LinkLayer linkLayer;
    strncpy(linkLayer.serialPort, serialPort, sizeof(linkLayer.serialPort)-1);
    linkLayer.role = actualRole;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;
    int sequenceNumber = 0;
 //    = { var, actualRole, baudRate, nTries, timeout};
   // linkLayer.serialPort = var;
    

    if (llopen(linkLayer) == -1)
    {
        printf("Unable to connect\n");
    }
    
    unsigned char packet[MAX_PAYLOAD_SIZE];
    switch (actualRole)
    {

        case LlRx:
            

            int STOP = FALSE;
            while (STOP == FALSE)
            {
                int nbytes = llread(packet);
                if (nbytes == -1) printf("Error when reading\n");
                if (nbytes == 0)
                {
                    printf("Not supposed to be reading, no information received\n");
                    return;
                }
                if (nbytes != 0)
                {
                    // DECOMPOSE AND SEE CONTROL BYTE
                    unsigned char controlField = packet[0];

                    switch (controlField)
                    {

                        case 1:


                            break;
                        case 2:

                            FILE *file = fopen(filename, "a");
                            if (file == NULL) {
                                perror("Error opening file");
                            }

                            unsigned char sequenceNumber = packet[1];
                            unsigned char l2 = packet[2];
                            unsigned char l1 = packet[3];

                            int k = 256 * l2 + l1;                            
                            int bytesWritten = 0;
                            if ( (bytesWritten = fwrite(packet + 4, sizeof(unsigned char), k, file)) < k)
                            {
                                printf("ERROR WRITTING TO FILE IN Sequence: %u\n", sequenceNumber);
                                printf("[Expectations/Reality] %d/%d\n", k, bytesWritten);
                            }


                            break;
                        case 3:
                            STOP = TRUE;
                            break;
                        default:
                            printf("Error??? Not supposed to read that\n");
                            break;
                    }

                } 

            }
            
        break;

        case LlTx:
            
            FILE *file = fopen(filename, "rb");
            if (file == NULL) {
                perror("Error opening file");
            }
            // Point to the end of the file
            fseek(file, 0, SEEK_END);
            // Get size
            unsigned int fileSize = ftell(file);
            // Point to the start again
            fseek(file, 0, SEEK_SET);

            // Write start
            packet[0] = 1;
            // file size
            packet[1] = 0;
            packet[2] = sizeof(fileSize);
            memcpy(packet + 3, &fileSize, packet[2]);
            // Filename
            packet[7] = 1;
            packet[8] = (unsigned char)strlen(filename);
            memcpy(packet + 9, filename, packet[8]);

            printf("Sending packet of %d bytes\n", packet[8] + packet[2] + 5);
            if (llwrite(packet, packet[8] + packet[2] + 5) == -1){ printf("Error writting") ; return;}

            // Write Data
            packet[0] = 2;
            int bytesRead = 0;
            while ((bytesRead = fread(packet + 4, 1, MAX_PAYLOAD_SIZE-4, file)) > 0)
            {  
                printf("Sending packet of %d bytes\n", bytesRead);
                packet[1] = sequenceNumber++;
                packet[2] = bytesRead / 256;
                packet[3] = bytesRead % 256; 
                if (llwrite(packet, bytesRead) == -1)
                {
                    printf("ERROR WRITTING\n");
                }                
            }

            // Write end
            unsigned char packet[MAX_PAYLOAD_SIZE];
            packet[0] = 3;
            // file size
            packet[1] = 0;
            packet[2] = sizeof(fileSize);
            memcpy(packet + 3, &fileSize, packet[2]);
            // Filename
            packet[7] = 1;
            packet[8] = (unsigned char)strlen(filename);
            memcpy(packet + 9, filename, packet[8]);

            llwrite(packet, packet[8] + packet[2] + 5);

            fclose(file);

        break;

    }

    llclose(0);
}
