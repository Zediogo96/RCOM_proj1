#include "alarm.h"

int alarm_enabled = FALSE, alarm_count = 0;

void kill_alarm()
{
    alarm_enabled = FALSE;
    alarm_count = 0;
}

void alarm_handler()
{
    alarm_count++;
    alarm_enabled = TRUE;
    printf("Alarm nº %d\n", alarm_count);
}


int start_alarm(unsigned int duration)
{       
    if (signal(SIGALRM, alarm_handler) < 0)
    {
        perror("signal");
        return -1;
    }  

    alarm_enabled = TRUE;
    alarm(duration);


    return 0;
}
