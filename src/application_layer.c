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
        ///////////////////////////////// RESUMO STEPS /////////////////////////////////        ///////////////////////////////// RESUMO STEPS /////////////////////////////////

        // send stuff
        // represent file
        // get control packet
        // write packets to buffer
        // use llwrite to transmit the buffer

        ///////////////////////////////// /////////// /////////////////////////////////        ///////////////////////////////// RESUMO STEPS /////////////////////////////////

        printf("\ndebug 1\n");

        printf("before open file\n"); // DEBUGGING
        // open file with filename

        FILE *fileptr;
    
        // int nBytes = 200, curByte=0, index=0, nSequence = 0;
        
        fileptr = fopen(filename, "rb");        // Open the file in binary mode
        if(fileptr == NULL){
            printf("Couldn't find a file with that name, sorry :(\n");
            return;
        }
        else
            printf("\nlog > File opened sucessfully\n");

        unsigned char buffer[PACKET_MAX_SIZE] = {0};
        unsigned int bytes_to_send = get_controlpacket(filename, TRUE, buffer);

        if (llwrite(buffer, bytes_to_send) < 0)
        {
            printf("Failed to send information frame\n");
            llclose(0);
            return -1;
        }

        /* unsigned counter = 0;
        int bytes_sent = 0;

        while ((bytes_to_send = read(file, buffer, PACKET_MAX_SIZE - 4)) > 0)
        {
            bytes_sent += bytes_to_send;
            counter++;
            bytes_to_send = get_datapacket(&buffer, bytes_to_send, counter);

            printf("----------- SENDING DATA PACKET %d -----------\n", counter);
            if (llwrite(buffer, bytes_to_send) == -1)
            {
                printf("\nlog > Error sending data packet\n");
                llclose(0);
                return;
            }
        }
 */
        // send end control packet
        bytes_to_send = get_controlpacket(filename, FALSE, buffer);

        if (llwrite(buffer, bytes_to_send) == -1)
        {
            printf("\nlog > Error sending end control packet\n");
            llclose(0);
            return;
        }
    }
    else if (connection.role == LlRx)
    {
        int *destination_file;
        int received_DISC = FALSE; // flag to check if DISC was received
        unsigned int packet_size = 0;

        unsigned char *buffer[PACKET_MAX_SIZE] = {0}; // buffer for received packets
        while (!received_DISC)
        {

            if (llread(buffer) == -1)
            {
                printf("\nlog > Error occurred during llread\n");
                llclose(0);
                return;
            }

            switch (handle_packet(&buffer, &packet_size))
            {
            case 0:
                printf("\nlog > Error Handling the packet (source: handle_packet())\n");
                llclose(0);
                return;
                break;
            case 1:
                printf("Inserting data: %d\n", packet_size);
                if (destination_file == -1)
                {
                    printf("\nlog > No file was initialized, aborting...\n");
                    return;
                }
                else
                {
                    for (int i = 4; i < packet_size + 4; i++)
                    {
                        write(destination_file, &buffer[i], 1);
                    }
                    printf("\n");   
                }
                break;
            case 2:
                printf("log > Start Control Packet received\n");
                destination_file = fopen(filename, "w+");
                break;
            case 3:
                printf("log > End Control Packet received\n");
                fclose(destination_file);
                received_DISC = TRUE;
                break;
            }
        }
    }

    if (llclose(stats) < 0)
    {
        printf("\nlog > Error: llclose failed!\n");
        return;
    }
}
