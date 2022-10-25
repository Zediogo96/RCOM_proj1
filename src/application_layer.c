// Application layer protocol implementation

#include "application_layer.h"
#include <sys/stat.h> // for stat()
#include <fcntl.h> // for open() and O_RDONLY

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
    else printf("\nlog > Connection was sucessfully established \n");

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
        // open file with filename
        
        struct stat file;
        stat(filename, &file);

        int file_fd = 0;

        if ((file_fd = open(filename, O_RDONLY)) < 0)
        {
            printf("\nlog > Error opening file, aborting...\n");
            // llclose
            return;
        }
        else printf("\nlog > File opened sucessfully\n");

        unsigned char buffer[PACKET_MAX_SIZE] = { 0 };
        unsigned int bytes_to_send = get_controlpacket(filename, file.st_size, TRUE, buffer);

        if (llwrite(buffer, bytes_to_send) < 0) {
            printf("Failed to send information frame\n");
            llclose(0);
            return -1;
        }

        unsigned counter = 0;
        int bytes_sent = 0;

        while ((bytes_to_send = read(file, buffer, PACKET_MAX_SIZE - 4)) > 0)
        {
            bytes_sent += bytes_to_send;
            counter++;
            bytes_to_send = get_datapacket(&buffer, bytes_to_send, counter);
            
            if (llwrite(buffer, bytes_to_send) == -1)
            {
                printf("\nlog > Error sending data packet\n");
                llclose(0);
                return;
            }
        }

        // send end control packet
        bytes_to_send = get_controlpacket(filename, file.st_size, FALSE, buffer);

        if (llwrite(buffer, bytes_to_send) == -1)
        {
            printf("\nlog > Error sending end control packet\n");
            llclose(0);
            return;
        }
    }
    else if (connection.role == LlRx)
    {
        // receive stuff
        // use llread to read from the buffer
    }

    // close connection

    if (llclose(stats) == (-1))
    {
        printf("\nlog > Error: llclose failed!\n");
        return;
    }
}
