#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "macros.h"
#include "packet.h"

#include <sys/stat.h>

// INTENDER TO BE PACKETS GENERATED BY THE APPLICATION
unsigned int get_controlpacket(unsigned char *filename, int start, unsigned char packet[])
{
    int size_packet = 0;

    if (strlen(filename) > 255)
    {
        printf("Size of filename couldn't fit in one byte: %d\n, 2");
        return -1;
    }

    unsigned char hex_string[255];

    struct stat file;
    stat(filename, &file);

    // Reads string directly as hexadecimal
    sprintf(hex_string, "%02lx", file.st_size);

    int idx = 3;
    int file_size_bytes = strlen(hex_string) / 2;
    int file_size = file.st_size;

    printf("\n File Size: %d\n");

    if (file_size_bytes > 256)
    {
        printf("size of file couldn't fit into 1 Byte\n");
    }


    printf("packet: %02lx\n", file_size_bytes);

    (start == TRUE) ? (packet[0] = C_START) : (packet[0] = C_END);
    /* if (start == TRUE)
        packet[index++] = C_START;
    else if (start == FALSE)
        packet[index++] = C_END; */

    // APPENDING T_SIZE TO PACKET
    packet[1] = T_SIZE;
    packet[2] = file_size_bytes;
    for (int i= file_size_bytes-1, j = 0; i>-1; i--, j++) {
        packet[3+j]=file_size >> (8*i);
    
    }
    // NOT SURE IF THE ORDER T_NAME 1ST & T_SIZE 2ND OR VICE-VERSA MATTERS
    packet[idx+file_size_bytes] = T_NAME;
    packet[idx+file_size_bytes+1] = strlen(filename);

    for (int i = 0; i < strlen(filename); i++)
    {
        packet[idx+2+file_size_bytes+i] = filename[i];
    }

    size_packet = 5+file_size_bytes+strlen(filename);

    for (int i = 0; i < size_packet; i++) {
        printf("%c\n",packet[i]);
    }
    return size_packet;
}

unsigned int get_datapacket(unsigned char *bytes, unsigned char *packet, int nSequence, int count_bytes)
{
    int l1 = div(count_bytes, 256).rem;
    int l2 = div(count_bytes, 256).quot;

    packet[0] = 0x01;
    packet[1] = div(nSequence, 255).rem;
    packet[2] = l2;
    packet[3] = l1;

    for (int i = 0; i < count_bytes; i++)
    {
        packet[i + 4] = bytes[i];
    }

    return count_bytes + 4; // for the initial data packet
}

unsigned int handle_packet(unsigned char *packet, unsigned int *size)
{
    unsigned int count = 0;
    unsigned int name_size = 0;
    unsigned int real_size = 0;

    switch (packet[0])
    {
    case C_START:
        if (packet[1] == T_NAME)
        {
            unsigned int new_size = 0;
            name_size = packet[2];

            real_size = packet[4 + name_size];
            for (int i = 0; i < real_size; i++)
            {
                new_size += packet[5 + name_size + i] << (8 * i);
            }
            *size = new_size; // size of file

            for (int i = 0; i < name_size; i++)
            {
                packet[i] = packet[3 + i];
                printf("%c", packet[3 + i]);
            }
            packet[name_size] = '\0'; // end of string
        }
        else if (packet[1] == T_SIZE)
        {
            real_size = packet[2]; // 1st byte of size
            unsigned int new_size = 0;

            int i;

            for (i = 0; i < real_size; i++)
            {
                new_size += packet[3 + i] << (8 * i);
            }
            *size = new_size; // size of file
            i += 3;

            if (packet[i] == T_NAME)
            {
                name_size = packet[i + 1];
                for (int j = 0; j < name_size; j++)
                {
                    packet[j] = packet[i + 2 + j];
                    printf("%c", packet[i + 2 + j]);
                }
                packet[name_size] = '\0'; // end of string
            }
            else
            {
                printf("log > Error in packet, aborting...\n");
                return 0;
            }
        }
        return 2;
    /* case C_END: NOT SURE IF ANY HANDLING IS NEEDED FOR THIS */
    case C_DATA:
        count = packet[1];
        *size = packet[2] * 256 + packet[3]; // size of data
        return 1;
    case C_DISC:
        return 4;
    default:
        return 0;
    }
}