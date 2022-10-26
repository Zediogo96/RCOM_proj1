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
            else {
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

unsigned char dataSavedChars[PACKET_MAX_SIZE] = {0};

MACHINE_STATE data_state = START;

int data_ptr = 0;

void reset_data_state_machine()
{
    data_state = START;
    data_ptr = 0;
}

int data_state_machine(unsigned char byte, int fd, LinkLayerRole role)
{
    while (TRUE)
    {
        switch (data_state)
        {
        case START:
            if (byte == FLAG)
            {
                data_state = F_RECEIVED;
                dataSavedChars[data_ptr++] = byte;
                return 0;
            }
            break;
        case F_RECEIVED:
            if (byte == A)
            {
                data_state = A_RECEIVED;
                dataSavedChars[data_ptr++] = byte;
                return 0;
            }
            break;
        case A_RECEIVED:
            if (byte != FLAG)
            {
                data_state = C_RECEIVED;
                dataSavedChars[data_ptr++] = byte;
                return 0;
            }
            break;
        case C_RECEIVED:
            if (byte == (dataSavedChars[1] ^ dataSavedChars[2]))
            {
                data_state = RECEIVING_PACKET;
                dataSavedChars[data_ptr++] = byte;
                return 0;
            }
            else if (byte == FLAG)
            {
                data_state = F_RECEIVED;
                data_ptr = 0;
                return 0;
            }
            else
            {
                printf("log > Protocol error. \n");
                data_ptr = 0;
                reset_data_state_machine();
                return 0;
            }
            break;
        case RECEIVING_PACKET:
            if (byte == ESCAPE_OCTET)
                return 3;
            else if (byte == FLAG)
            {
                data_state = START;
                data_ptr = 0;
                return 1;
            }
            else
                return 2;
            break;
        }
        return 0;
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
            if (byte == FLAG) response_state = 3;
            else
                return 0;
            break;
        // related to BCC, still needs to check it better
        case 3:
            if (response_saved_c[3] == ((response_saved_c[1] ^ response_saved_c[2])) && res_ptr > 4)
            {
                response_state = 4;
            }
            else {
                reset_answer_state_machine();
                return 0;
            }

        case 4:

            if (response_saved_c[2] == C_RR(0))
            {
                reset_answer_state_machine();
                if (CA == 0) return 1; else return -1;
            }
            else if (response_saved_c[2] == C_REJ(0) || response_saved_c[2] == C_REJ(1))
            {
                reset_answer_state_machine();
                if (CA == 0) return 1; else return -1;
            }

        default:
            break; // default
        }          // switch
    }

    return 0;
}
