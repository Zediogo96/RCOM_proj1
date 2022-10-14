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
            if (value == 0x7E) // octet 01111110
            {
                printf("log > Received flag \n");
                state = 1;
                saved_c[idx] = value;
                idx++;
                return 0;
            }
            break;
        case 1:
            if (value != 0x7E)
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
            if (role == L1Rx)
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
            else if (role == L1Tx)
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
