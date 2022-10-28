#include "alarm.h"

int alarm_enabled = FALSE, alarm_count = 1;

void kill_alarm()
{
    printf("\nlog > Alarm: killed\n"); // debugging
    alarm_enabled = FALSE;
    alarm_count = 1;
}

void alarm_handler(int signal)
{
    alarm_enabled = FALSE;
    alarm_count++;

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
