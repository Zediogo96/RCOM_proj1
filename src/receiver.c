#include <unistd.h>
#include <stdio.h>

#include "macros.h"
#include "link_layer.h"
#include "state_machine.h"
#include "receiver.h"

unsigned char r_buffer[BUFFER_SIZE] = {0};

int receiverStart(int fd)
{

    unsigned char buf[1] = {0}, saved_buffer[5] = {0};

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

    unsigned char ua_message [5] = {FLAG, A, C_UA, A ^ C_UA, FLAG};

    int bytes = write(fd, ua_message, sizeof(ua_message));
    printf("\nlog > UA message sent, %d bytes written\n", bytes);

    alarm_enabled = FALSE;
    
    return 0;
}

int send_supervision_frame(int fd, int type, int ca)
{
    unsigned char buffer[5] = {FLAG, A, C_SET, A ^ C_SET, FLAG};
    if (type == 0)
        buffer[2] = C_REJ(ca); // CA is 0 or 1
    else
        buffer[2] = C_RR(ca); // CA is 0 or 1

    buffer[3] = (buffer[1] ^ buffer[2]);

    int bytes = write(fd, buffer, 5);

    printf("%s%d frame sent, %d bytes written\n", (type == 0 ? "REJ" : "RR"), ca, bytes);
    return bytes;
}

// received disconnect
