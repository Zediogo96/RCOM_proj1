#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "macros.h"
#include "link_layer.h"
#include "state_machine.h"

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
        else
        {
            *state = 0;
            memset(saved_buffer, 0, 5);
        }
        break;
    default:
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// HANDLING STATE MACHINE FOR DATA
/////////////////////////////////////////////////////////////////////////////////////////////
int data_ptr = 0;

int data_state_machine(unsigned char byte, int *state, unsigned char *info_frame, int *stop, int *sizeInfo)
{
    switch (*state)
    {
    case 0:
        if (byte == FLAG)
        {
            *state = 1;
            info_frame[data_ptr++] = byte;
        }
        break;
    case 1:
        if (byte != FLAG)
        {
            *state = 2;
            info_frame[data_ptr++] = byte;
        }
        else
        {
            memset(info_frame, 0, 5);
            *state = 1;
            data_ptr = 0;
            info_frame[data_ptr++] = byte;
        }
        break;

    case 2:
        if (byte != FLAG)
        {
            info_frame[data_ptr++] = byte;
        }
        else
        {
            printf("\nlog > Final FLAG was received!\n");
            *stop = TRUE;
            info_frame[data_ptr++] = byte;
            *sizeInfo = data_ptr;
            data_ptr = 0;
            
        }
        break;
    }
}

int response_state = 0;
unsigned char response_saved_c[BUFFER_SIZE] = {};
int res_ptr = 0;

void reset_answer_state_machine()
{
    response_state = 0;
    res_ptr = 0;
}

int data_answer_machine(unsigned char byte, int fd, int CA)
{
    while (TRUE)
    {
        switch (response_state)
        {
        case 0:
            if (byte == FLAG)
            {
                response_state = 1;
                response_saved_c[res_ptr++] = byte;
                return 0;
            }
            break; // case 0
        case 1:
            if (byte != FLAG)
            {
                response_state = 2;
                response_saved_c[res_ptr++] = byte;
                return 0;
            }
            break;
        case 2:
            response_saved_c[res_ptr++] = byte;
            if (byte == FLAG)
                response_state = 3;
            else
                return 0;
            break;
        // related to BCC, still needs to check it better
        case 3:
            if (response_saved_c[3] == ((response_saved_c[1] ^ response_saved_c[2])) && res_ptr > 4)
            {
                response_state = 4;
            }
            else
            {
                reset_answer_state_machine();
                return 0;
            }

        case 4:

            if (response_saved_c[2] == C_RR(0))
            {
                reset_answer_state_machine();
                if (CA == 0)
                    return 1;
                else
                    return -1;
            }
            else if (response_saved_c[2] == C_REJ(0) || response_saved_c[2] == C_REJ(1))
            {
                reset_answer_state_machine();
                if (CA == 0)
                    return 1;
                else
                    return -1;
            }

        default:
            break; // default
        }          // switch
    }

    return 0;
}
