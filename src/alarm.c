
#include "alarm.h"
#include "macros.h"
#include "utils.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1


int alarmEnabled = FALSE;
int alarmCount = 0;
extern Statistics stats;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    stats.timeoutCount++;

    printf("Alarm #%d\n", alarmCount);
}


int setupAlarm(int maximumRetransmitions, int timeout)
{
    if (alarmCount > maximumRetransmitions)
    {
        return 1;
    }
    if (alarmEnabled == TRUE) return 2;

    (void)signal(SIGALRM, alarmHandler);
    alarmEnabled = TRUE;
    alarm((unsigned int)timeout);
    return 0;
}


void turnOffAlarm()
{
    alarm(0);
    alarmEnabled = FALSE;
    alarmCount = 0;
}
