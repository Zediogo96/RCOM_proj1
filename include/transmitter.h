#ifndef _TRANSMITTER_H_
#define _TRANSMITTER_H_
#include "alarm.h"

int sendSET();

int transmitter_start(int fd, LinkLayer ll);

int transmitter_send_disc(int fd);

int transmitter_send_UA(int fd);

int transmitter_await_disconnect(int fd);

int transmitter_stop(int fd, int nNRetransmissions, int timeout);

#endif // _TRANSMITTER_H_