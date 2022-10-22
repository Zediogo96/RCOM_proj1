#ifndef _TRANSMITTER_H_
#define _TRANSMITTER_H_
#include "alarm.h"


int sendSET();

int transmitter_ctrl_receive();

void transmitter_alarm_handler(int signal);

int transmitter_start(int fd_, int nRetransmissions_, int timeout);

#endif // _TRANSMITTER_H_