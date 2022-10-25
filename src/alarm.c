#include "alarm.h"

int alarm_enabled = FALSE, alarm_count = 0;

void kill_alarm()
{
    printf("log -> Alarm: killed\n"); // debugging
    alarm_enabled = FALSE;
    alarm_count = 0;
}

void alarm_handler(int signal)
{
    alarm_enabled = FALSE;
    alarm_count++;
    printf("Alarm nº %d\n", alarm_count);
}

int start_alarm(unsigned int duration)
{
    if (signal(SIGALRM, alarm_handler) < 0)
    {
        perror("signal");
        return -1;
    }

    if (alarm_enabled == FALSE)
    {
        alarm(duration);
        alarm_enabled = TRUE;
    }

    return 0;
}
