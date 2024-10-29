
#pragma once

// Alarm Handler
void alarmHandler(int signal);

// Tries to set up alarm 
// Return "0" when set up alarm
// Return "1" when maximum of retransmission reached
// Return "2" when alarm already enabled
int setupAlarm(int maximumRetransmitions, int timeout);


// Turns off alarm
void turnOffAlarm();

