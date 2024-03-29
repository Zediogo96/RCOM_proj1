#include <stdio.h>
#include <unistd.h>
#include "macros.h"

#include "link_layer.h"
#include "state_machine.h"
#include "transmitter.h"

int t_fd;
int t_nRetransmissions;

//Sends the SET message

int sendSET(int fd)
{
    unsigned char buffer_SET[5] = {FLAG, A, C_SET, A ^ C_SET, FLAG};

    int bytes = write(fd, buffer_SET, 5);
    printf("\nlog > SET flag sent, %d bytes written\n", bytes);
    return bytes;
}

// Starts the transmitter and all related stuff

int transmitter_start(int fd, LinkLayer ll)
{
    unsigned char buffer[5] = {};

    while (TRUE)
    {
        if (alarm_count > ll.nRetransmissions)
        {
            printf("\nlog > Alarm limit reached, SET message not sent\n");
            return -1;
        }

        if (!alarm_enabled)
        {
            printf("\nWarning > Alarm nº %d\n", alarm_count);
            sendSET(fd);
            start_alarm(ll.timeout);
        }

        int bytes = read(fd, buffer, 5);

        if (bytes > -1 && buffer[0] == FLAG)
        {
            if (buffer[2] != C_UA || (buffer[3] != (buffer[1] ^ buffer[2])))
            {
                printf("\nlog > UA not correct, continuing...\n");
                alarm_enabled = FALSE;

                continue;
            }
            else
            {
                printf("\nlog > UA correctly received.\n");
                alarm_enabled = FALSE;
                break;
            }
        }
    }
    return 0;
}

//Sends the DISC message from the transmitter

int transmitter_send_disc(int fd)
{
    unsigned char MSG[5] = {FLAG, A, C_DISC, A ^ C_DISC, FLAG};

    int bytes = write(fd, MSG, 5);
    printf("\nlog > Transmitter DISC flag sent, %d bytes written\n", bytes);
    return bytes;
}

//Sends the UA message from the transmitter

int transmitter_send_UA(int fd)
{
    unsigned char MSG[5] = {FLAG, A_RCV, C_UA, A_RCV ^ C_UA, FLAG};

    int bytes = write(fd, MSG, 5);
    printf("\nlog > Transmitter UA flag sent, %d bytes written\n", bytes);
    return bytes;
}

//Waits for the DISC message from the receiver

int transmitter_await_disconnect(int fd)
{
    unsigned char t_buffer[BUFFER_SIZE] = {0};
    int bytes = read(fd, t_buffer, 1);
    if (bytes > -1)
    {
        int c_ans = llclose_state_machine(t_buffer[0], fd);
        if (c_ans == 2)
        {
            kill_alarm();
            return 1;
        }
    }
    return 0;
}

//Calls all the other functions needed to close the connection

int transmitter_stop(int fd, int nNRetransmissions, int timeout)
{

    t_nRetransmissions = nNRetransmissions;

    while (1)
    {
        if (!alarm_enabled)
        {
            if (t_nRetransmissions == 0)
            {
                printf("log > Timeout\n");
                return 0;
            }
            transmitter_send_disc(fd);
            t_nRetransmissions--;
            start_alarm(timeout);
        }

        if (transmitter_await_disconnect(fd) == 1)
        {
            printf("\nlog > DISC Received, sending UA\n");
            transmitter_send_UA(fd);
            return 1;
        }
    }

    return 0;
}