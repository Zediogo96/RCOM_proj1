// Application layer protocol implementation

#include "application_layer.h"
#include <sys/stat.h> // for stat()
#include <fcntl.h>    // for open() and O_RDONLY

int stats = 1; // for now, always show statistics
// change for macros
// statistics will be the number of packets sent

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connection;

    // snprintf(connection.serialPort, sizeof(connection.serialPort), "%s\n", serialPort);  //copy serial port number to link layer struct

    // swap snprintf connection.serialPort to serialPort
    strncpy(connection.serialPort, serialPort, sizeof(serialPort) + 4);

    // see what is the role of the device running the application

    if (strcmp(role, "tx") == 0)
    {
        connection.role = LlTx; // transmitter
    }
    else if (strcmp(role, "rx") == 0)
    {
        connection.role = LlRx; // receiver
    }
    else
    {
        return; // role is invalid
    }

    // set link layer properties
    connection.baudRate = baudRate;       // copy baud rate to link layer struct
    connection.nRetransmissions = nTries; // copy number of tries to link layer struct
    connection.timeout = timeout;         // copy timeout to link layer struct

    if (llopen(connection) < 0)
    {
        printf("\nlog > Error in llopen, aborting...\n");
        // llclose
        return;
    }
    else
        printf("\nlog > Connection was sucessfully established \n");

    // if transmitter, send data

    if (connection.role == LlTx)
    {
        // open file in binary mode
        FILE *file;
        file = fopen(filename, "rb");

        unsigned char packet[PACKET_MAX_SIZE] = {0}, bytes[200];

        int current_byte = 0, idx = 0, number_seq = 0;
        int fileOver = FALSE;

        // CHECK THIS ////////////////////////////////////////
        if (file == NULL)
        {
            printf("\nlog > Error opening the file\n");
            return;
        }
        else
            printf("\nlog > File opened sucessfully\n");

        //////////////////////////////////// SEND FIRST CONTROL PACKET ///////////////////////////////////
        int packet_size = get_controlpacket(filename, TRUE, &packet);

        if (llwrite(packet, packet_size) == -1)
        {
            printf("\nlog > Error sending first control packet\n");
            return;
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////

        while (!fileOver)
        {
            if (!fread(&current_byte, (size_t)1, (size_t)1, file))
            {
                fileOver = TRUE;
                packet_size = get_datapacket(bytes, &packet, number_seq++, idx);

                if (llwrite(packet, packet_size) == -1)
                {
                    printf("\n log > Error sending writting packet\n");
                    return;
                }
            }
            else if (PACKET_MAX_SIZE == idx)
            {

                packet_size = get_datapacket(bytes, &packet, number_seq++, idx);

                if (llwrite(packet, packet_size) == -1)
                {
                    printf("\n log > Error sending writting packet\n");
                    return;
                }

                memset(bytes, 0, sizeof(bytes));
                memset(packet, 0, sizeof(packet));
                idx = 0;
            }

            bytes[idx++] = current_byte;
        }

        fclose(file);

        //////////////////////////////////// SEND END CONTROL PACKET ///////////////////////////////////
        packet_size = get_controlpacket(filename, FALSE, &packet);

        if (llwrite(packet, packet_size) == -1)
        {
            printf("\nlog > Error sending end control packet\n");
            return;
        }
        //////////////////////////////////////////////////////////////////////////////////////////////////
    }
    else if (connection.role == LlRx)
    {
        FILE *dest_file;

        char readBytes = 1;

        while (TRUE)
        {

            unsigned char packet[PACKET_MAX_SIZE] = {0};
            int packet_size = 0, idx = 0;

            if (llread(&packet, &packet_size) == -1)
            {
                continue;
            }
            if (packet[0] == C_END)
            {
                printf("\nlog > Destination file closed\n");
                fclose(dest_file);
                break;
            }
            else if (packet[0] == C_START)
            {
                printf("\nlog > Destination file was open\n");
                dest_file = fopen(filename, "wb");
            }
            else
            {
                // write everything except control packets
                for (int i = 4; i < packet_size; i++)
                {
                    fputc(packet[i], dest_file);
                }
            }
        }
    }

    llclose(0); // close connection
}
