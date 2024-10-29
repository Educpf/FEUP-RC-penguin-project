
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

// 0 -> alarm set up
// 1 -> alarm not set because of HHHHHHHHHHH
// 2 -> already enabled
int setupAlarm(int maximumRetransmitions, int timeout)
{
    if (alarmCount > maximumRetransmitions)
    {
        printf("MAXIMUM RETRANSMITION REACHED\n");
        return 1;
    }
    if (alarmEnabled == TRUE) return 2;

    printf("Alarm set\n");
    (void)signal(SIGALRM, alarmHandler);
    alarmEnabled = TRUE;
    alarm((unsigned int)timeout);
    return 0;
}

void turnOffAlarm()
{
    printf("Turning off alarm\n");
    alarm(0);
    alarmEnabled = FALSE;
    alarmCount = 0;
}
