// Link layer protocol implementation
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
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

clock_t start;

LinkLayer ll_info;

int senderNumber = 0, receiverNumber = 1, lastFrameNumber = -1;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopen(LinkLayer connectionParameters)
{
    start = clock();

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

int llwrite(const unsigned char *buf, int bufSize)
{

    printf("\n------------------------------LLWRITE------------------------------\n\n");

    alarm_count = 0;

    unsigned char BCC = 0x00, info_frame[PACKET_MAX_SIZE + 5 + 5] = {0}, parcels[5] = {0};
    int index = 4, STOP = 0, controlReceiver = C_RR(!senderNumber);

    // BCC working correctly
    for (int i = 0; i < bufSize; i++)
        BCC ^= buf[i];

    info_frame[0] = FLAG;
    info_frame[1] = A;
    info_frame[2] = (senderNumber == 1) ? C_ONE : C_ZERO;
    info_frame[3] = info_frame[1] ^ info_frame[2];

    // FRAME STUFFING
    for (int i = 0; i < bufSize; i++)
    {
        if (buf[i] == FLAG)
        {
            info_frame[index++] = ESCAPE_OCTET;
            info_frame[index++] = FLAG_OCTET_SUB;
            continue;
        }
        else if (buf[i] == FLAG)
        {
            info_frame[index++] = ESCAPE_OCTET;
            info_frame[index++] = ESCAPE_OCTET_SUB;
            continue;
        }

        info_frame[index++] = buf[i];
    }

    if (BCC == FLAG)
    {
        info_frame[index++] = ESCAPE_OCTET;
        info_frame[index++] = FLAG_OCTET_SUB;
    }

    else if (BCC == ESCAPE_OCTET)
    {
        info_frame[index++] = ESCAPE_OCTET;
        info_frame[index++] = ESCAPE_OCTET_SUB;
    }

    else
        info_frame[index++] = BCC;

    info_frame[index++] = FLAG; // LAST FLAG

    while (!STOP)
    {
        if (!alarm_enabled)
        {
            write(fd, info_frame, index);
            printf("\ninfo_frame sent NS=%d\n", senderNumber);
            start_alarm(ll_info.timeout);
        }

        int result = read(fd, parcels, 5);

        if (result != -1)
        {

            if (parcels[2] != (controlReceiver) || (parcels[3] != (parcels[1] ^ parcels[2])))
            {
                printf("\n log > RR not correct: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                alarm_enabled = FALSE;
                continue;
            }

            else
            {
                printf("\nlog > RR correctly received: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                alarm_enabled = FALSE;
                STOP = 1;
            }
        }

        if (alarm_count >= ll_info.nRetransmissions)
        {
            printf("\n log > llwrite error: Exceeded number of tries when sending frame\n");
            STOP = 1;
            close(fd);
            return -1;
        }
    }

    (senderNumber) ? (senderNumber = 0) : (senderNumber = 1);

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet, int *sizeOfPacket)
{
    printf("\n --------------------- LL_READ ---------------------\n");

    unsigned char info_frame[PACKET_MAX_SIZE] = {0}, superv_frame[5] = {0}, BCC2 = 0x00, aux[PACKET_MAX_SIZE] = {0}, STOP = FALSE;
    int control = (!receiverNumber) << 6, index = 0, sizeInfo = 0;

    unsigned char buf[1] = {0};

    int state = 0;

    while (STOP == FALSE)
    {
        int _bytes = read(fd, buf, 1);

        if (_bytes > -1)
        {
            // SM machine stopping is handled with the variable stop
            data_state_machine(buf[0], &state, &info_frame, &STOP, &sizeInfo);
        }
    }

    superv_frame[0] = FLAG;
    superv_frame[1] = A;
    superv_frame[4] = FLAG;

    if ((info_frame[1] ^ info_frame[2]) != info_frame[3] || info_frame[2] != control)
    {
        printf("\nLog > info_frame not received correctly. Protocol error. Sending REJ.\n");
        superv_frame[2] = C_REJ(receiverNumber);
        superv_frame[3] = superv_frame[1] ^ superv_frame[2];
        write(fd, superv_frame, 5);

        printf("\n-----REJ-----\n");
        printf("\nSize of REJ: %d\nREJ: 0x", 5);

        for (int i = 0; i < 5; i++)
        {
            printf("%02X ", superv_frame[i]);
        }

        printf("\n\n");

        return -1;
    }

    // FRAME DESTUFFING
    for (int i = 0; i < sizeInfo; i++)
    {
        if (info_frame[i] == ESCAPE_OCTET && info_frame[i + 1] == FLAG_OCTET_SUB)
        {
            packet[index++] = FLAG;
            i++;
        }

        else if (info_frame[i] == ESCAPE_OCTET && info_frame[i + 1] == ESCAPE_OCTET_SUB)
        {
            packet[index++] = ESCAPE_OCTET;
            i++;
        }

        else
            packet[index++] = info_frame[i];
    }

    int size = 0; // tamanho da secçao de dados

    if (packet[4] == C_DATA)
    {
        size = 256 * packet[6] + packet[7] + 4 + 6; //+4 para contar com os bytes de controlo, numero de seq e tamanho
        for (int i = 4; i < size - 2; i++)
            BCC2 ^= packet[i];
    }
    else
    {
        size += packet[6] + 3 + 4;        //+3 para contar com os bytes de C, T1 (packet[6]) e L1 // +4 para contar com os bytes FLAG, A, C, BCC
        size += packet[size + 1] + 2 + 2; //+2 para contar com T2 e L2 //+2 para contar com BCC2 e FLAG

        for (int i = 4; i < size - 2; i++)
            BCC2 ^= packet[i];
    }

    if (packet[size - 2] == BCC2)
    {

        if (packet[4] == C_DATA)
        {
            if (info_frame[5] == lastFrameNumber)
            {
                printf("\nlog > info_frame received correctly. Repeated Frame. Sending RR.\n");
                superv_frame[2] = C_RR(receiverNumber);
                superv_frame[3] = superv_frame[1] ^ superv_frame[2];
                write(fd, superv_frame, 5);
                return -1;
            }
            else
            {
                lastFrameNumber = info_frame[5];
            }
        }
        printf("\nlog > info_frame received correctly. Sending RR.\n");
        superv_frame[2] = C_RR(receiverNumber);
        superv_frame[3] = superv_frame[1] ^ superv_frame[2];
        write(fd, superv_frame, 5);
    }

    else
    {
        printf("\nlog > info_frame not received correctly. Error in data packet. Sending REJ.\n");
        superv_frame[2] = C_REJ(receiverNumber);
        superv_frame[3] = superv_frame[1] ^ superv_frame[2];
        write(fd, superv_frame, 5);

        return -1;
    }

    (*sizeOfPacket) = size;

    index = 0;

    // FRAGMENT THE PACKET TO ONLY READ THE DATA ////////////////
    for (int i = 4; i < (*sizeOfPacket) - 2; i++)
    {
        aux[index++] = packet[i];
    }

    (*sizeOfPacket) = size - 6;

    memset(packet, 0, sizeof(packet));

    for (int i = 0; i < (*sizeOfPacket); i++)
    {
        packet[i] = aux[i];
    }
    // FRAGMENT THE PACKET TO ONLY READ THE DATA ////////////////

    (receiverNumber == 0) ? (receiverNumber = 1) : (receiverNumber = 0);

    return 1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics, int count_frames)
{

    printf("\n------------------------------LLCLOSE------------------------------\n\n");

    // handling logic here

    printf("log > Closing connection procedure innitiated.\n");

    if (showStatistics)
    {
        printf("\n------------------------------STATISTICS------------------------------\n\n");
        double cpu_time_used = ((double)(clock() - start)) / CLOCKS_PER_SEC * 1000; // ms
        printf(" > The application took %f miliseconds to execute.", cpu_time_used);
        printf("\n > Number of frames %s: %d\n", (ll_info.role == LlRx) ? "received" : "sent", count_frames);
        printf("\n-----------------------------------------------------------------------\n\n");
    }

    if (ll_info.role == LlRx)
    {
        if (receiver_stop(ll_info.nRetransmissions, ll_info.timeout, fd))
        {
            printf("\nlog > Connection terminated.\n");
        }
        else
        {
            printf("\nlog > Connection failed to terminate.\n");
        }
    }
    else
    {
        if (transmitter_stop(ll_info.nRetransmissions, ll_info.timeout, fd))
        {
            printf("\nlog > Connection terminated.\n");
        }
        else
        {
            printf("\nlog > Connection failed to terminate.\n");
        };
    }

    // delete this shit

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    close(fd);

    return 1;
}
