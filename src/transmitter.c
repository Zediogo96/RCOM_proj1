#include <stdio.h>
#include <unistd.h>
#include "macros.h"
#include "link_layer.h"
#include "state_machine.h"
#include "alarm.h"
#include "transmitter.h"

int sendSET()
{

    unsigned char buffer_SET[5] = {FLAG, A, C_SET, A ^ C_SET, FLAG};

    int bytes = write(fd, buffer_SET, 5);
    printf("SET flag sent, %d bytes written\n", bytes);
    return bytes;
}

int transmitter_ctrl_receive()
{
    unsigned char buffer[BUFFER_SIZE] = {0};

    int bytes = read(fd, buffer, 1);
    if ((buffer != 0) && (bytes > -1))
    {
        int answer = sm_process_states(buffer[0], fd, LlTx);
        if (answer == 1)
        {
            killAlarm();
            return 1;
        }
    }

    return 0;
}

void transmitter_alarm_handler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    sendSET();
    nRetransmissions--;
    printf("Alarm no. %d\n", alarmCount);
}

int transmitter_start(int fd_, int nRetransmissions_, int timeout)
{
    fd = fd_;
    nRetransmissions = nRetransmissions_;
    sendSET();

    while (nRetransmissions > 0)
    {
        alarm_start(timeout, transmitter_alarm_handler);

        if (transmitter_ctrl_receive(fd) == 1) return 1;
    }

    return 0;
}