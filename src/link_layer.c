// Link layer protocol implementation
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "link_layer.h"
#include "macros.h"

#include "receiver.h"
#include "transmitter.h"

// MISC
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

int fd = 0;

struct termios oldtio;
struct termios newtio;

LinkLayer ll_info;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopen(LinkLayer connectionParameters)
{

    printf("Opening connection %s", connectionParameters.serialPort);

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY | O_NONBLOCK);

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

    //Discard data written but not transmitted
    tcflush(fd, TCIOFLUSH);

    // Set new port configuration
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    printf("\nNew termios structure set\n");

    printf("\n------------------------------LLOPEN------------------------------\n\n");
    
    if (connectionParameters.role == LlRx)
    {
        if (receiverStart(fd))
            return -1;
    }
    else if (connectionParameters.role == LlTx)
    {
        if (transmitter_start(fd, connectionParameters))
            return -1;
    }

    ll_info = connectionParameters;
    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

int ca = 0;

int llwrite(const unsigned char *buf, int bufSize)
{
    unsigned char frame[2 * PACKET_MAX_SIZE + 6] = {0};

    // buildFrame
    int frameSize = buildInformationFrame(&frame, buf, bufSize, ca);

    // SendFrame, this is using the global variable, will refactor if I have time
    if (transmitter_info_send(frame, frameSize, ll_info.nRetransmissions, ll_info.timeout, ca) == -1)
        return -1;
    else
    {
        (ca == 0) ? (ca = 1) : (ca = 0); // toggle ca
    }
    // check for errors
    if (frameSize < 0)
        return -1;

    return 1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    printf("\n------------ LL READ ------------ \n");
    unsigned char buffer[1] = {0};

    int stuffing = FALSE;
    int packet_size = 0;
    unsigned char read_packet[PACKET_MAX_SIZE] = {0};

    while (TRUE) {
        int bytes_ = read(fd, &buffer, 1);

        if (buffer != 0 && bytes_ > -1) {
            int ans = data_state_machine(buffer[0], fd, ca);
            switch (ans)
            {
            case -1:
                reset_data_state_machine();
                return -1;
                break;
            case 1:;
                unsigned char bcc2 = read_packet[0];
                for (int i = 1; i < (packet_size - 1); i++) {
                    bcc2 = (bcc2 ^ read_packet[i]);
                }
                if (bcc2 != read_packet[packet_size-1]) {
                    printf("log > Data error. \n");
                    reset_data_state_machine();
                    send_supervision_frame(fd, 0, ca);
                    break;
                }
                for (int i = 0; i < packet_size-1; i++) {
                    packet[i*8] = read_packet[i];
                }
                send_supervision_frame(fd, 1, ca);
                if (ca == 0) ca = 1; else ca = 0;
                printf("Information frame received. \n");
                return packet_size-1;
                break;
            case 2:
                if (stuffing) {
                    stuffing = FALSE;
                    read_packet[packet_size++] = buffer[0] + 0x20;
                } else {
                    read_packet[packet_size++] = buffer[0];
                }
                break;
            case 3:
                stuffing = TRUE;
                break;
            case 5:
                send_supervision_frame(fd, 1, (ca == 0) ? 1 : 0);
                break;
            default:
                break;
            }
        }
    }
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{

    // use some kind of C library for time measurement

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    close(fd);

    return 1;
}
