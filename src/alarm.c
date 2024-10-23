// Alarm example
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]
#include "alarm.h"
#include "macros.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1


int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

// 0 -> alarm set up
// 1 -> alarm not set because of 
// 2 -> already enabled
int setupAlarm(int maximumRetransmitions, int timeout)
{
    if (alarmCount > maximumRetransmitions) return 1;
    if (alarmEnabled == TRUE) return 2;

    printf("Alarm set\n");
    (void)signal(SIGALRM, alarmHandler);
    alarmEnabled = TRUE;
    alarm(timeout);
    return 0;
}

int getAlarmState() { return alarmEnabled; }

void turnOffAlarm()
{
    printf("Turning off alarm\n");
    alarm(0);
    alarmEnabled = FALSE;
    alarmCount = 0;
}


// int main()
// {
//     // Set alarm function handler
//     (void)signal(SIGALRM, alarmHandler);

//     while (alarmCount < 4)
//     {
//         if (alarmEnabled == FALSE)
//         {
//             alarm(3); // Set alarm to be triggered in 3s
//             alarmEnabled = TRUE;
//         }
//         
//     }

//     printf("Ending program\n");

//     return 0;
// }
