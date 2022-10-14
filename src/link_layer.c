// Link layer protocol implementation
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "link_layer.h"
#include "macros.h"

#include "receiver.h"
#include "transmitter.h"

// MISC
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define FALSE 1

#define BUF_SIZE 256

int fd;

struct termios oldtio;
struct termios newtio;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopen(LinkLayer connectionParameters)
{
    print("Opening connection %s", connectionParameters.serialPort);

    fd = open(connectionParameters.serialPort, 0_RDWR | 0_NOCTTY, 0_NONBLOCK);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        return -1;
    }

    // Save the current settings for use in later reset
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        return -1;
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received
    tcflush(fd, TCIOFLUSH);

    // Set new port configuration
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    printf("New termios structure set\n");

    if (connectionParameters.role == LlRx)
        if (!receiverStart(fd))
            return -1;
        else if (connectionParameters.role == LlTx) // NOT SURE IF I NEED THIS GUARD OR JUST ELSE
            if (!transmitter_start(fd, connectionParameters.nRetransmissions, connectionParameters.timeout))
                return -1;

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    if (tcsetattr(fd, TCSANOW, &oldtio == -1))
    {
        perror("tcsetattr");
        return -1;
    }

    close(fd);

    return 1;
}
