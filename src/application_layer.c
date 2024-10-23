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
 //    = { var, actualRole, baudRate, nTries, timeout};
   // linkLayer.serialPort = var;
    

    if (llopen(linkLayer) == -1)
    {
        printf("Unable to connect\n");
    }
    
    switch (actualRole)
    {

        case LlRx:

            
        break;

        case LlTx:


        break;

    }

}
