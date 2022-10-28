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

    // CHECK IF FILENAME DOESN'T FIT INTO ONE BYTE
    if (strlen(filename) > 255)
    {
        printf("Size of filename couldn't fit in one byte: %d\n, 2");
        return -1;
    }

    unsigned char hex_string[255];

    struct stat file;
    stat(filename, &file);

    // READS STRING IN HEXADECIMAL
    sprintf(hex_string, "%02x", file.st_size);

    int idx = 3;
    int file_size_bytes = strlen(hex_string) / 2;
    int file_size = file.st_size;

    printf("\nFile Size: %d\n", file_size);

    // CHECK IF FILESIZE NUMBER DOESN'T FIT INTO ONE BYTE
    if (file_size_bytes > 256)
    {
        printf("Size of file couldn't fit into 1 Byte\n");
    }

    printf("Packet size in Bytes: %02x\n", file_size_bytes);

    // APPENDS CONTROL DEPENDING IF IT'S THE START CONTROL PACKET OR END ON
    (start == TRUE) ? (packet[0] = C_START) : (packet[0] = C_END);

    // APPENDING T_SIZE TO PACKET
    packet[1] = T_SIZE;
    packet[2] = file_size_bytes;
    for (int i = file_size_bytes - 1, j = 0; i > -1; i--, j++)
        packet[3 + j] = file_size >> (8 * i);

    // NOT SURE IF THE ORDER T_NAME 1ST & T_SIZE 2ND OR VICE-VERSA MATTERS
    packet[idx + file_size_bytes] = T_NAME;
    packet[idx + file_size_bytes + 1] = strlen(filename);

    // APPENDS FILENAME BYTES INTO THE PACKET
    for (int i = 0; i < strlen(filename); i++)
        packet[idx + 2 + file_size_bytes + i] = filename[i];

    size_packet = 5 + file_size_bytes + strlen(filename);

    for (int i = 0; i < size_packet; i++)
        printf("%02x ", packet[i]);

    printf("\n");
    return size_packet;
}

unsigned int get_datapacket(unsigned char *bytes, unsigned char *packet, int nSequence, int count_bytes)
{

    // CALCULATIONS FOR FIRST PART OF DATA PACKET
    int l1 = count_bytes % 256;
    int l2 = count_bytes / 256;

    // APPENDS CONTROL, L1, L2 TO DATAPACKET
    packet[0] = C_DATA;
    packet[1] = nSequence % 256;
    packet[2] = l2;
    packet[3] = l1;

    // APPENDS DATA INTO DATAPACKET
    for (int i = 0; i < count_bytes; i++)
    {
        packet[i + 4] = bytes[i];
    }

    // +4 BECAUSE OF INITIAL CONTROL PACKET
    return count_bytes + 4;
}
