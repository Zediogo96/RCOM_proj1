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

    // Discard data written but not transmitted
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

    printf("\n------------ LL WRITE ------------ \n");

    alarm_count = 0;
    unsigned char bcc = 0x00, info_frame[PACKET_MAX_SIZE] = {0}, temp_buffer[5] = {0};
    int idx = 4, stop = FALSE, controlReceiver = (!ca << 7) | 0x05;

    // check bcc
    for (int i = 0; i < bufSize; i++)
        bcc ^= buf[i];

    info_frame[0] = FLAG;
    info_frame[1] = A;
    info_frame[2] = (ca == 1) ? (C_ONE) : (C_ZERO);

    info_frame[3] = info_frame[1] ^ info_frame[2];

    for (int i = 0; i < bufSize; i++)
    {
        if (buf[i] == FLAG)
        {
            info_frame[idx++] = ESCAPE_OCTET;
            info_frame[idx++] = FLAG_OCTET_SUB;
            continue;
        }
        else if (buf[i] == ESCAPE_OCTET)
        {
            info_frame[idx++] = ESCAPE_OCTET;
            info_frame[idx++] = ESCAPE_OCTET_SUB;
            continue;
        }

        info_frame[idx++] = buf[i];
    }

    if (bcc == FLAG)
    {
        info_frame[idx++] = FLAG;
        info_frame[idx++] = FLAG_OCTET_SUB;
    }
    else if (bcc == 0x7D)
    {
        info_frame[idx++] = ESCAPE_OCTET;
        info_frame[idx++] = ESCAPE_OCTET_SUB;
    }

    else
    {
        info_frame[idx++] = bcc;
    }

    info_frame[idx++] = 0x7E;

    while (!stop)
    {
        if (!alarm_enabled)
        {
            write(fd, info_frame, idx);
            printf("\ninfo_frame sent NS=%d\n", ca);
            start_alarm(ll_info.timeout);
        }

        int result = read(fd, temp_buffer, 5);

        if (result != -1 && temp_buffer != 0)
        {

            if (temp_buffer[2] != (controlReceiver) || (temp_buffer[3] != (temp_buffer[1] ^ temp_buffer[2])))
            {
                // PRINT SOMETHING
                alarm_enabled = FALSE;
                continue;
            }

            else
            {
                printf("SUCCESS");
                alarm_enabled = FALSE;
                stop = 1;
            }
        }

        if (alarm_count >= NUM_MAX_TRIES)
        {
            printf("\nllwrite error: Exceeded number of tries when sending frame\n");
            stop = 1;
            close(fd);
            return -1;
        }
    }

    (ca) ? (ca = 0) : (ca = 1);

    return 0;
}

/* int llwrite(const unsigned char *buf, int bufSize)
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
} */

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

    while (TRUE)
    {
        int bytes_ = read(fd, &buffer, 1);

        if (buffer != 0 && bytes_ > -1)
        {
            int ans = data_state_machine(buffer[0], fd, ca);
            switch (ans)
            {
            case -1:
                reset_data_state_machine();
                return -1;
                break;
            case 1:;
                unsigned char bcc2 = read_packet[0];
                for (int i = 1; i < (packet_size - 1); i++)
                {
                    bcc2 = (bcc2 ^ read_packet[i]);
                }
                if (bcc2 != read_packet[packet_size - 1])
                {
                    printf("log > Data error. \n");
                    reset_data_state_machine();
                    send_supervision_frame(fd, 0, ca);
                    break;
                }
                for (int i = 0; i < packet_size - 1; i++)
                {
                    packet[i * 8] = read_packet[i];
                }
                send_supervision_frame(fd, 1, ca);
                if (ca == 0)
                    ca = 1;
                else
                    ca = 0;
                printf("Information frame received. \n");
                return packet_size - 1;
                break;
            case 2:
                if (stuffing)
                {
                    stuffing = FALSE;
                    read_packet[packet_size++] = buffer[0] + 0x20;
                }
                else
                {
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
