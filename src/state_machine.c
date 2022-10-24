#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "macros.h"
#include "link_layer.h"
#include "state_machine.h"

int state = 0;
unsigned char saved_c[BUFFER_SIZE] = {};
__uint8_t idx = 0;

int sm_process_states(unsigned char value, int fd, LinkLayerRole role)
{
    while (TRUE)
    {
        switch (state)
        {
        case 0: 
            if (value == FLAG) // octet 01111110
            {
                printf("log > Received flag \n");
                state = 1;
                saved_c[idx] = value;
                idx++;
                return 0;
            }
            break;
        case 1:
            if (value != FLAG)
            {
                state = 2;
                saved_c[idx] = value;
                idx++;
                return 0;
            }
        case 2:
            saved_c[idx] = value;
            idx++;
            if (value == 0x7E)
                state = 3;
            else
                return 0;
        case 3:
            if (saved_c[3] == (saved_c[1] ^ saved_c[2]) && idx > 4)
                state = 4;
            else
            {
                state = 0;
                printf("log -> Bad input, restarting... \n");
                idx = 0;
                return 0;
            }
        case 4:
            state = 0;
            idx = 0;
            if (role == LlRx)
            {
                if (saved_c[2] == 0x03)
                {
                    saved_c[2] = 0x07;
                    saved_c[3] = saved_c[1] ^ saved_c[2];
                    saved_c[4] = 0x7E;
                    printf("log -> UA will be sent.. \n");
                    write(fd, saved_c, BUFFER_SIZE);
                    return 1;
                }
            }
            else if (role == LlTx)
            {
                if (saved_c[2] == 0x07)
                {
                    printf("log -> Deactivating alarm. \n");
                    return 1;
                }
            }
            printf("log > Wrong C \n");
            return -1;
        default:
            break;
        }
        return 0;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// HANDLING STATE MACHINE FOR DATA
/////////////////////////////////////////////////////////////////////////////////////////////

enum mst {
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

int data_state_machine (unsigned char byte, int fd, LinkLayerRole role)
{
    while (1)
    {  
        switch (data_state)
        {
        case START:
            if (byte == FLAG) {
                data_state = F_RECEIVED;
                dataSavedChars[data_ptr++] = byte;
                return 0;
            }
            break;
        case F_RECEIVED:
            if (byte == A) {
                data_state = A_RECEIVED;
                dataSavedChars[data_ptr++] = byte;
                return 0;
            }
            break;
        // ANALISAR MELHOR ESTE
        case A_RECEIVED: 
            if (byte != FLAG) {
                data_state = C_RECEIVED;
                dataSavedChars[data_ptr++] = byte;
                return 0;
            }
            break;
        
        // ESTE TAMBÉM É DUVIDOSO
        case C_RECEIVED:
            if (byte == (dataSavedChars[1] ^ dataSavedChars[2])) {
                data_state = RECEIVING_PACKET;
                dataSavedChars[data_ptr++] = byte;
                return 0;
            }
            else 
            {
                data_state = BCC1_RECEIVED;
                data_ptr = 0;
                return -1;
            }
            break;
        case RECEIVING_PACKET: 
        if (byte == ESCAPE_OCTET) return 3;
        else if (byte == FLAG) 
        {
            data_state = START;
            data_ptr = 0;
            return 1;
        }
        else return 2;

        break;

        default:
            break;
        }
    }
}
