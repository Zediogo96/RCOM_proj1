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
int sendSET(int fd)
{
    unsigned char buffer_SET[5] = {FLAG, A, C_SET, A ^ C_SET, FLAG};

    int bytes = write(fd, buffer_SET, 5);
    printf("\nlog > SET flag sent, %d bytes written\n", bytes);
    return bytes;
}


int transmitter_start(int fd, LinkLayer ll)
{

    unsigned char buffer[5] = {};

    while (TRUE)
    {   
        if (alarm_count > ll.nRetransmissions)
        {
            printf("\nlog > Alarm limit reached, SET message not sent\n");
            return -1;
        }

        if (!alarm_enabled)
        {
            int bytes = sendSET(fd);
            start_alarm(ll.timeout);
        }

        int _bytes = read(fd, buffer, 5);

        if (_bytes > -1 && buffer != 0 && buffer[0] == FLAG)
        {   
            if (buffer[2] != C_UA || (buffer[3] != (buffer[1] ^ buffer[2])))
            {
                printf("\nlog > UA not correct, continuing...\n");
                alarm_enabled = FALSE;
                continue;
            }
            else 
            {
                printf("\nlog > UA correctly received.\n");
                alarm_enabled = FALSE;
                break;
            }
        }
    }
    return 0;
}

// GOOD
int transmitter_ctrl_receive()
{
    unsigned char t_buffer[BUFFER_SIZE] = {0};

    int bytes = read(t_fd, t_buffer, 1);
    if (bytes > -1)
    {   
        /* printf("Received %02x \n", t_buffer[0]); // debugging */
        if (sm_process_states(t_buffer[0], t_fd, LlTx) == 1)
        {
            printf("log -> Transmitter: received UA\n");
            kill_alarm();
            return 1;
        }
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
    (CA == 0) ? (frame[2] = C_ZERO) : (frame[2] = C_ONE);

    // SET BCC1
    frame[3] = frame[1] ^ frame[2];

    // VERIFY IF IT'S SIZE IT'S CORRECT
    unsigned char newPacket[PACKET_MAX_SIZE * 2 + 2] = {0};

    // FIND BCC2
    unsigned char bcc2 = 0x00;

    for (int i = 0; i < packetSize; i++)
    {
        newPacket[i] = packet[i];
        bcc2 ^= packet[i];
    }

    newPacket[packetSize] = bcc2;

    packetSize = stuffing(newPacket, packetSize + 1);

    // copy newPacket to frame
    for (int i = 0; i < packetSize; i++)
    {
        frame[frameSize++] = newPacket[i];
    }

    // SET LAST POS TO FLAG
    frame[frameSize++] = FLAG;

    return frameSize;
}

int sendFrame(unsigned char frame_to_send[], int frameToSendSize)
{
    int bytes = write(t_fd, frame_to_send, frameToSendSize);
    printf("\n");
    /* for (int i = 0; i < frameToSendSize; i++) printf("%02x ", frame_to_send[i]); */
    printf("\nInformation frame sent, %d bytes written\n", bytes);
    return bytes;
}

// TODO 
int transmitter_info_receive(int ca) {

    unsigned char buffer[BUFFER_SIZE] = {0};

    int bytes_ = read(t_fd, buffer, 1);
    if (bytes_ > -1) {
        if (data_answer_machine(buffer[0], t_fd, ca)) {
            kill_alarm();
            return 1;
        }
    }

    return 0;
}

int transmitter_info_send(unsigned char frameToSend[], int frameToSendSize, int new_NRetransmissions, int timeout, int ca)
{
    t_nRetransmissions = new_NRetransmissions;

    while (TRUE)
    {
        if (!alarm_enabled)
        {
            if (t_nRetransmissions == 0) {
                printf("\nlog > Maximum number of retransmissions, aborting.\n");
            }
            sendFrame(frameToSend, frameToSendSize);
            t_nRetransmissions--;
            start_alarm(timeout);
        }

        // TODO, STILL DON'T UNDERSTAND IT ENTIRELY
        if (transmitter_info_receive(ca))
            return 1;
    }

    return 0;
}