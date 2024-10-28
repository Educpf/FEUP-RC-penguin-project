#include "stateMachine.h"
#include "macros.h"
#include "link_layer.h"
#include <string.h>

static unsigned char addressByte, controlByte, newControl;
static enum machineStates state = START;
//  MAXIMUM_DATA + BCC2
static unsigned char buf[MAX_PAYLOAD_SIZE + 1] = {0};
static int byteSize = 0;
static unsigned char repeated = 0;
static unsigned char escapeFound = 0;
static int frameNumToSend = 0;

enum machineStates handleByte(unsigned char byte)
{

    switch (state)
    {
    case START:
        if (byte == FLAG)
            state = FLAG_RCV;
        break;
    case FLAG_RCV:
        addressByte = byte;
        if (byte == AS || byte == AR)
            state = A_RCV;
        else if (byte != FLAG)
            state = START;
        break;
    case A_RCV:
        if (byte == SET || byte == UA || byte == RR0 || byte == RR1 || byte == REJ0 || byte == REJ1 || byte == DISC || isInfoControl(byte))
            state = C_RCV;
        else if (byte == FLAG)
            state = FLAG_RCV;
        else
            state = START;

        newControl = byte;
        break;
    case C_RCV:
        if (byte == (addressByte ^ newControl))
            state = BCC_OK;
        else if (byte == FLAG)
            state = FLAG_RCV;
        else
            state = START;
        break;
    case BCC_OK:
        if (byte == FLAG)
        {
            // if it is sender getting OK response
            state = END;
            if (isInfoControl(newControl))
            {
                if (newControl == controlByte) 
                    repeated = 1;              
            }
            controlByte = newControl;
        }
        // If is receiver receving information
        else if (isInfoControl(newControl) && addressByte == AS)
        {
            if (byte == ESC)
                escapeFound = 1;
            else
            {
                if (escapeFound)
                {
                    if (byte == ESCAPED_FLAG)
                        byte = FLAG;
                    else if (byte == ESCAPED_ESC)
                        byte = ESC;
                    escapeFound = 0;
                }
                buf[byteSize] = byte;
                byteSize++;
            }
        }
        else
            state = START;
        break;
    case END:

        break;
    default:
        break;
    }
    return state;
}

enum machineStates getMachineState() { return state; }

enum machineStates getControlByte() { return controlByte; }

unsigned char isInfoRepeated() { return repeated; }

unsigned char *getMachineData() { return buf; }

int getMachineDataSize() { return byteSize; }

int getFrameNum() { return frameNumToSend; }

void cleanMachineData()
{
    memset(buf, 0, MAX_PAYLOAD_SIZE + 1);
    byteSize = 0;
    state = START;
    repeated = 0;
    escapeFound = 0;
    addressByte = 0;
}

void invertControlByte()
{
    controlByte = controlByte == C_INFO_0 ? C_INFO_1 : C_INFO_0;
}

void invertFrameNum()
{
    frameNumToSend = frameNumToSend == C_INFO_0 ? C_INFO_1 : C_INFO_0;
}