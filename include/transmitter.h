#ifndef _TRANSMITTER_H_
#define _TRANSMITTER_H_
#include "alarm.h"


int sendSET();

int transmitter_ctrl_receive();

void transmitter_alarm_handler(int signal);

int transmitter_start(int fd_, int nRetransmissions_, int timeout);

int buildInformationFrame(unsigned char *frame, unsigned char packet[], int packetSize, unsigned int CA);

int sendFrame(unsigned char frame_to_send[], int frameToSendSize);

int sender_information_send(unsigned char frameToSend[], int frameToSendSize, int new_NRetransmissions, int timeout);

#endif // _TRANSMITTER_H_