#ifndef _ALARM_H_
#define _ALARM_H_

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "macros.h"

extern int alarm_enabled , alarm_count;

void kill_alarm();

int start_alarm(unsigned int duration);

void alarm_handler();

#endif // _ALARM_H_