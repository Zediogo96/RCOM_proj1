#include <unistd.h>
#include <stdio.h>
#include "macros.h"
#include "link_layer.h"
#include "state_machine.h"
#include "receiver.h"

unsigned char r_buffer[BUFFER_SIZE] = {0};

// Starts the receiver and all related stuff

int receiverStart(int fd)
{

    unsigned char saved_buffer[5] = {0};

    int state = 0;
    int STOP = FALSE;

    while (STOP == FALSE)
    {
        int _bytes = read(fd, r_buffer, 1);

        if (_bytes > -1)
        {
            // SM machine stopping is handled with the variable stop
            sm_process_states(r_buffer[0], fd, &state, &saved_buffer, &STOP);
        }
    }

    unsigned char ua_message[5] = {FLAG, A, C_UA, A ^ C_UA, FLAG};

    int bytes = write(fd, ua_message, sizeof(ua_message));
    printf("\nlog > UA message sent, %d bytes written\n", bytes);

    alarm_enabled = FALSE;

    return 0;
}

// Sends the DISC message from the receiver

int receiver_send_disconnect(int fd)
{
    unsigned char MSG[5] = {FLAG, A_RCV, C_DISC, A_RCV ^ C_DISC, FLAG};

    int bytes = write(fd, MSG, 5);
    printf("\nlog > Receiver DISC flag sent, %d bytes written\n", bytes);
    return bytes;
}

// Processes received bytes on state machine waiting for the DISC message

int receiver_await_disconnect(int fd)
{
    unsigned char r_buffer[BUFFER_SIZE] = {0};

    int bytes = read(fd, r_buffer, 1);

    if (bytes > -1)
    {
        int c_answer = llclose_state_machine(r_buffer[0], fd);
        if (c_answer == 1)
        {
            return 1;
        }
    }
    return 0;
}

// Processes received bytes on state machine waiting for the UA message

int receiver_await_UA(int fd)
{
    unsigned char r_buffer[BUFFER_SIZE] = {0};

    int bytes = read(fd, r_buffer, 1);

    if (bytes > -1)
    {
        int c_answer = llclose_state_machine(r_buffer[0], fd);
        if (c_answer == 3)
        {
            return 1;
        }
    }
    return 0;
}

int receiver_NRetransmissions = 0;

// Calls all the other functions needed to close the connection

int receiver_stop(int nNRetransmissions, int timeout, int fd)
{

    receiver_NRetransmissions = nNRetransmissions;

    while (1)
    {
        if (receiver_await_disconnect(fd) == 1)
        {
            break;
        }
    }

    if (!alarm_enabled)
    {
        if (receiver_NRetransmissions == 0)
        {
            printf("log > Timeout\n");
            return 0;
        }
        receiver_send_disconnect(fd);
        receiver_NRetransmissions--;
        start_alarm(timeout);
    }

    if (receiver_await_UA(fd) == 1)
    {
        printf("\nlog > DISC Received, sending UA\n");
        transmitter_send_UA(fd);
    }

    return 1;
}