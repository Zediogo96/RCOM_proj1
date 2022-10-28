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

    // 1º criar o BCC para o dataPacket
    // 2º fazer byte stuffing
    // 3º criar a nova infoFrame com o dataPacket (ja stuffed) la dentro
    // 4º enviar a infoFrame e contar o alarme
    // 5º factCheck a frame recebida do llread (ver se tem erros ou assim)
    // 6º llwrite so termina quando recebe mensagem de sucesso ou quando o limite de tentativas é excedido

    printf("\n------------------------------LLWRITE------------------------------\n\n");

    alarm_count = 0;

    unsigned char BCC = 0x00, infoFrame[600] = {0}, parcels[5] = {0};
    int index = 4, STOP = 0, controlReceiver = (!senderNumber << 7) | 0x05;

    // BCC working correctly
    for (int i = 0; i < bufSize; i++)
    {
        BCC = (BCC ^ buf[i]);
    }

    infoFrame[0] = 0x7E;                // Flag
    infoFrame[1] = 0x03;                // Address
    infoFrame[2] = (senderNumber << 6); // Control
    infoFrame[3] = infoFrame[1] ^ infoFrame[2];

    for (int i = 0; i < bufSize; i++)
    {
        if (buf[i] == 0x7E)
        {
            infoFrame[index++] = 0x7D;
            infoFrame[index++] = 0x5e;
            continue;
        }
        else if (buf[i] == 0x7D)
        {
            infoFrame[index++] = 0x7D;
            infoFrame[index++] = 0x5D;
            continue;
        }

        infoFrame[index++] = buf[i];
    }

    if (BCC == 0x7E)
    {
        infoFrame[index++] = 0x7D;
        infoFrame[index++] = 0x5e;
    }

    else if (BCC == 0x7D)
    {
        infoFrame[index++] = 0x7D;
        infoFrame[index++] = 0x5D;
    }

    else
    {
        infoFrame[index++] = BCC;
    }

    infoFrame[index++] = 0x7E;

    while (!STOP)
    {
        if (!alarm_enabled)
        {
            write(fd, infoFrame, index);
            printf("\nInfoFrame sent NS=%d\n", senderNumber);
            start_alarm(ll_info.timeout);
        }

        int result = read(fd, parcels, 5);

        if (result != -1 && parcels != 0)
        {
            /*alarm_enabled = FALSE;
            return 1;*/

            if (parcels[2] != (controlReceiver) || (parcels[3] != (parcels[1] ^ parcels[2])))
            {
                printf("\nRR not correct: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                alarm_enabled = FALSE;
                continue;
            }

            else
            {
                printf("\nRR correctly received: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                alarm_enabled = FALSE;
                STOP = 1;
            }
        }

        if (alarm_count >= ll_info.nRetransmissions)
        {
            printf("\nllwrite error: Exceeded number of tries when sending frame\n");
            STOP = 1;
            close(fd);
            return -1;
        }
    }

    (senderNumber) ? (senderNumber = 0) : (senderNumber = 1);

    return 0;
}

enum state {
    STATE0,
    STATE1,
    STATE2,
    STATE3, 
    STATE4,
    STATE5      
} typedef STATE;

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet, int *sizeOfPacket)
{
    // TO RE-DO FROM SCRATCH, REFACTORING WAS GETTING IMPOSSIBLE WITH ERRORS 
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    
    printf("\n------------------------------LLCLOSE------------------------------\n\n");

    //handling logic here

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    close(fd);

    return 1;
}
