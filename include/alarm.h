
#pragma once


void alarmHandler(int signal);


int setupAlarm(int maximumRetransmitions, int timeout);

int getAlarmState();

void turnOffAlarm();

