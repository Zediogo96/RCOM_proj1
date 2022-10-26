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
        ///////////////////////////////// RESUMO STEPS /////////////////////////////////

        // send stuff
        // represent file
        // get control packet
        // write packets to buffer
        // use llwrite to transmit the buffer
        // repeat

        ///////////////////////////////// /////////// /////////////////////////////////

        //  printf("before open file\n"); // DEBUGGING

        // open file with filename in binary mode
        FILE * file = fopen(filename, "rb");

        int number_bytes = 200, current_byte = 0, idx = 0, number_sequence = 0;

        unsigned char buffer[PACKET_MAX_SIZE] = {0}, bytes[200];

        if(file == NULL){
            printf("\nlog > Error opening the file\n");
            return;
        }
        else
            printf("\nlog > File opened sucessfully\n");

        //////////////////////////////////// SEND FIRST CONTROL PACKET ///////////////////////////////////
        int packet_size = get_controlpacket(filename, TRUE, &buffer);

        if (llwrite(buffer, packet_size) < 0)
        {
            printf("\nlog > Error sending first control packet\n");
            llclose(0);
            return;
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////
        
        while (fread(&current_byte, (size_t) 1, (size_t) 1, file))
        {
            packet_size = get_datapacket(bytes, &buffer, number_sequence++, index);
            
            for (int i = 0; i < sizeof(buffer); i++)
            {
                printf("%x ", buffer[i]);
            }

            {
                if (llwrite(buffer, packet_size) == -1)
                {   
                    printf("\n log > Error sending writting packet\n");
                    return;
                }
            }
        }



        fclose(file);

        //////////////////////////////////// SEND END CONTROL PACKET ///////////////////////////////////
        packet_size = get_controlpacket(filename, FALSE, buffer);

        if (llwrite(buffer, packet_size) == -1)
        {
            printf("\nlog > Error sending end control packet\n");
            llclose(0);
            return;
        }
        //////////////////////////////////////////////////////////////////////////////////////////////////
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
