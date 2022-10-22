#include <unistd.h>
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
        if (r_buffer != 0 && _bytes > -1)
        {
            int answer = sm_process_states(r_buffer[0], fd, LlRx);
            if (answer == 1)
                return 1; // POSSÍVEL REFACTOR AQUI
        }
    }

    return 0;
}
