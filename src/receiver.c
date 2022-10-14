#include <unistd.h>
#include "macros.h"
#include "link_layer.h"
#include "state_machine.h"
#include "receiver.h"

unsigned char buffer[BUFFER_SIZE] = {0};

int receiverStart(int fd)
{
    while (TRUE)
    {
        int _bytes = read(fd, buffer, 1);
        if (buffer != 0 && _bytes > -1)
        {
            int ans = startVerifyState(buffer[0], fd, LlRx);
            if (ans == 1)
                return 1; // POSSÍVEL REFACTOR AQUI
        }
    }

    return 0;
}
