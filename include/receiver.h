#ifndef _RECEIVER_H_
#define _RECEIVER_H_

#include "alarm.h"

int receiverStart(int fd);

int receiver_send_disconnect(int fd);

int receiver_await_disconnect(int fd);

int receiver_await_UA(int fd);

int receiver_stop(int nNRetransmissions, int timeout, int fd);

#endif // _RECEIVER_H_
