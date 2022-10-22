#include <stdio.h>
#include <unistd.h>
#include "macros.h"

#include "link_layer.h"
#include "state_machine.h"
#include "transmitter.h"


int t_fd;
int t_nRetransmissions;

int sendSET()
{
    unsigned char buffer_SET[5] = {FLAG, A, C_SET, A ^ C_SET, FLAG};

    int bytes = write(t_fd, buffer_SET, 5);
    printf("SET flag sent, %d bytes written\n", bytes);
    return bytes;
}

int transmitter_ctrl_receive()
{
    unsigned char t_buffer[BUFFER_SIZE] = {0};

    int bytes = read(t_fd, t_buffer, 1);
    if ((t_buffer != 0) && (bytes > -1))
    {
        int answer = sm_process_states(t_buffer[0], t_fd, LlTx);
        if (answer == 1)
        {
            kill_alarm();
            return 1;
        }
    }

    return 0;
}

int transmitter_start(int t_fd_, int t_nRetransmissions_, int timeout)
{
    t_fd = t_fd_;
    t_nRetransmissions = t_nRetransmissions_;
    sendSET();

    while (t_nRetransmissions > 0)
    {
        if (!alarm_enabled)
        {
            sendSET();
            t_nRetransmissions--;
            start_alarm(timeout);
        }
    }

    return 0;
}
