#ifndef _RECEIVER_H_
#define _RECEIVER_H_

int receiverStart(int fd);

int send_supervision_frame(int fd, int type, int ca);

#endif // _RECEIVER_H_
