#include <unistd.h>
#include <stdio.h>

#include "macros.h"
#include "link_layer.h"
#include "state_machine.h"
#include "receiver.h"

unsigned char r_buffer[BUFFER_SIZE] = {0};

int receiverStart(int fd)
{
    while (TRUE)
    {
        int _bytes = read(fd, r_buffer, 1);
        if (/* r_buffer != 0 && COMPILER DÁ WARNING */ _bytes > -1)
        {
            int answer = sm_process_states(r_buffer[0], fd, LlRx);
            if (answer == 1)
                return 1; // POSSÍVEL REFACTOR AQUI
        }
    }

    return 0;
}

int send_supervision_frame(int fd, int type, int ca)
{
    unsigned char buffer[5] = {FLAG, A, C_SET, A^C_SET, FLAG};
    if (type == 0) buffer[2] = C_REJ(ca); // CA is 0 or 1
    else buffer[2] = C_RR(ca); // CA is 0 or 1

    buffer[3] = (buffer[1] ^ buffer[2]);

    int bytes = write(fd, buffer, 5);

    printf("%s%d frame sent, %d bytes written\n", (type == 0 ? "REJ" : "RR"), ca, bytes);
    return bytes;
}




// received disconnect

/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////

