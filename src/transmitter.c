#include <stdio.h>
#include <unistd.h>
#include "macros.h"

#include "link_layer.h"
#include "state_machine.h"
#include "transmitter.h"
#include "aux.h"

int t_fd;
int t_nRetransmissions;

// GOOD
int sendSET()
{
    unsigned char buffer_SET[5] = {FLAG, A, C_SET, A ^ C_SET, FLAG};

    int bytes = write(t_fd, buffer_SET, 5);
    printf("SET flag sent, %d bytes written\n", bytes);
    return bytes;
}

// GOOD
int transmitter_ctrl_receive()
{
    unsigned char t_buffer[BUFFER_SIZE] = {0};

    int bytes = read(t_fd, t_buffer, 1);
    if ((t_buffer != 0) && (bytes > -1))
    {
        int answer = sm_process_states(t_buffer[0], t_fd, LlTx);
        if (answer == 1)
        {
            printf("Transmitter: received UA\n");
            kill_alarm();
            return 1;
        }
    }

    return 0;
}

int transmitter_start(int new_fd, int new_nRetransmissions, int timeout)
{
    t_fd = new_fd;
    t_nRetransmissions = new_nRetransmissions_;

    // Number maximum number of retransmissions was not reached
    while (t_nRetransmissions > 0)
    {
        if (!alarm_enabled)
        {
            sendSET();
            t_nRetransmissions--;
            start_alarm(timeout);
        }

        // Verificar se estar parte está certa
        if (transmitter_ctrl_receive())
            return 1;
    }

    return 0;
}

int buildInformationFrame(unsigned char *frame, unsigned char packet[], int packetSize, unsigned int CA)
{
    int frameSize = 4; // VERIFY THIS

    // INITIAL FLAG
    frame[0] = FLAG;
    // SET ADDRESS
    frame[1] = A;
    // SET ALTERNATING CONTROL ADDRESS
    frame[2] = CA;

    if (CA == 0)
        frame[2] = C_ZERO;
    else
        frame[2] = C_ONE;

    // SET BCC1
    frame[3] = frame[1] ^ frame[2];

    // VERIFY IF IT'S SIZE IT'S CORRECT
    unsigned char newPacket[PACKET_MAX_SIZE * 2 + 2] = {0};

    // FIND BCC2
    unsigned char bcc2 = 0x00;

    for (int i = 0; i < packetSize; i++)
    {
        newPacket[i] = packet[i];
        if (i == 1)
        {
            bcc2 = packet[i - 1] ^ packet[i];
        }
        else
        {
            bcc2 ^= packet[i];
        }
    }

    newPacket[packetSize + 1] = bcc2;

    packetSize = stuffing(newPacket, packetSize + 2);

    // copy newPacket to frame
    for (int i = 0; i < packetSize; i++)
    {
        frame[frameSize] = newPacket[i];
        frameSize++;
    }

    // SET LAST POS TO FLAG
    frame[frameSize++] = FLAG;

    return frameSize;
}