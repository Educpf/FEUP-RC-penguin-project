#pragma once

#define ESC 0x7D
#define ESCAPED_ESC 0x5D

#define FLAG 0x7E
#define ESCAPED_FLAG 0x5E

#define AS 0x03
#define AR 0x01

#define SET 0x03
#define UA 0x07
#define C_INFO_0 0x00
#define C_INFO_1 0x80

#define RR0 0xAA
#define RR1 0xAB
#define REJ0 0x54
#define REJ1 0x55

#define DISC 0x0B

#define isFlag(x) 
#define isInfoControl(x) (x == C_INFO_0 || x == C_INFO_1)
#define isRejectionByte(x) (x == REJ0 || x == REJ1)
#define isReadyToReceiveByte(x) (x == RR0 || x == RR1)


#define receiveToSendControlByte(x) ( C_INFO_1 *  (x - RR0) )


#define BUF_SIZE 256
#define CHUNK_SIZE 40




