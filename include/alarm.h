#ifndef _ALARM_H_
#define _ALARM_H_

#include "macros.h"

extern int alarmEnabled, alarmCount;

void killAlarm();

void alarmHandler();

#endif // _ALARM_H_