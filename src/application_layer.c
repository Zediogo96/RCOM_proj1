// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

int stats = 1; // for now, always show statistics
// change for macros
// statistics will be the number of packets sent

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connection;

    // snprintf(connection.serialPort, sizeof(connection.serialPort), "%s\n", serialPort);  //copy serial port number to link layer struct

    // swap snprintf connection.serialPort to serialPort
    strncpy(connection.serialPort, serialPort, sizeof(serialPort) + 3);

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

    if (llopen(connection) == (-1))
    {
        printf("log > Error in llopen, aborting...\n");
        // llclose
        return;
    }
    else printf("\n Connection was sucessfully established \n");

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

        // open file with filename
        FILE *file = fopen(filename, "r");
        unsigned int file_size;

        if (file == NULL)
        {
            printf("Error opening file");
            // llclose
            return;
        }
        ///////// GET FILE SIZE /////////
        fseek(file, 0, SEEK_END); // seek to end of file
        file_size = ftell(file);  // get current file pointer
        rewind(file);             // seek back to beginning of file
        /////////////////////////////////

        unsigned char buffer[PACKET_MAX_SIZE] = { 0 };
        unsigned int bytes_to_send = get_controlpacket(filename, file_size, TRUE, buffer);

        unsigned counter = 0;
        int bytes_sent = 0;

        while ((bytes_to_send = read(file, buffer, PACKET_MAX_SIZE)) > 0)
        {
            bytes_sent += bytes_to_send;
            bytes_to_send = get_data_packet(&buffer, bytes_to_send, counter);
            
            if (llwrite(buffer, bytes_to_send) == -1)
            {
                printf("Error sending data packet\n");
                // llclose
                return -1;
            }
        }

        // send end control packet
    }
    else if (connection.role == LlRx)
    {
        // receive stuff
        // use llread to read from the buffer
    }

    // close connection

    if (llclose(stats) == (-1))
    {
        printf("\nError: Close failed!\n");
        return;
    }
}
