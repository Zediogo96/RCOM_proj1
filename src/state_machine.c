#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "macros.h"
#include "link_layer.h"
#include "state_machine.h"

//refactor this spaghetti

int sm_process_states(unsigned char byte, int fd, int *state, unsigned char *saved_buffer, int *stop)
{
    switch (*state)
    {
        case 0:
            if (byte == FLAG) // octet 01111110
            {
                *state = 1;
                saved_buffer[0] = byte;
            }
            break;
        case 1:
            if (byte != FLAG)
            {
                *state = 2;
                saved_buffer[1] = byte;
            }
            else 
            {
                *state = 0;
                memset(saved_buffer, 0, 5);
            }
            break;
        case 2:
            if (byte != FLAG)
            {
                *state = 3;
                saved_buffer[2] = byte;
            }
            else
            {
                *state = 0;
                memset(saved_buffer, 0, 5);
            }
        case 3:
            if (byte != FLAG)
            {
                saved_buffer[3] = byte;
                *state = 4;
            }
            else 
            {
                *state = 0;
                memset(saved_buffer, 0, 5);
            }
            break;
        case 4:
            if (byte == FLAG)
            {
                saved_buffer[4] = byte;
                state = 5;
            }
            else 
            {
                *state = 0;
                memset(saved_buffer, 0, 5);
            }
        case 5:
            if ((saved_buffer[1] ^ saved_buffer[2]) == saved_buffer[3])
            {
                printf("\nlog > SET message received without any errors!\n");
                *stop = TRUE;
            }
            else {
                *state = 0;
                memset(saved_buffer, 0, 5);
            }
            break;
        default:
            break;
    }
}

enum mst
{
    START,
    F_RECEIVED,
    A_RECEIVED,
    C_RECEIVED,
    BCC1_RECEIVED,
    RECEIVING_PACKET,
    WAITING_END_FLAG
} typedef MACHINE_STATE;


//State machine for handling llclose shit - still need to refactor the other one for opening

int end_state = 0;
unsigned char end_saved_c[BUFFER_SIZE] = {};
int end_ptr = 0;

int llclose_state_machine(unsigned char byte, int fd) {  //thanks copilot
    while (TRUE)
    {
        switch (end_state)
        {
        case START:
            if (byte == FLAG)
            {
                end_state = F_RECEIVED;
                end_saved_c[end_ptr++] = byte;
                return 0;
            }
            break;
        case F_RECEIVED:
            if (byte == A_RCV || byte == A)
            {
                end_state = A_RECEIVED;
                end_saved_c[end_ptr++] = byte;
                return 0;
            }
            break;
        case A_RECEIVED:
            if (byte != FLAG)
            {
                end_state = C_RECEIVED;
                end_saved_c[end_ptr++] = byte;
                return 0;
            }
            break;
        case C_RECEIVED:
            if (byte == (end_saved_c[1] ^ end_saved_c[2]))
            {
                end_state = RECEIVING_PACKET;
                end_saved_c[end_ptr++] = byte;
                return 0;
            }
            else 
            {
                printf("log > Protocol error. \n");
                end_state = F_RECEIVED;
                end_ptr = 0;
                return -1;
            }
            break;
        case RECEIVING_PACKET:
            if (byte == FLAG) {
                end_state = START;
                end_ptr = 0;
                if (end_saved_c[2] == C_UA)
                    return 3;
                else if (end_saved_c[2] == C_DISC && end_saved_c[1] == A_RCV)
                    return 2;
                else if (end_saved_c[2] == C_DISC && end_saved_c[1] == A)
                    return 1;
                return 0;
            }
            break;
        }
    }
}

