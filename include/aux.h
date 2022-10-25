#ifndef _AUX_H_
#define _AUX_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

int stuffing(unsigned char *data, int data_size)
{
    unsigned char new_data[PACKET_MAX_SIZE*2+2] = {0};

    // stores the last position where new_data was updated

    int new_data_pos = 0;

    for (int i = 0; i < data_size; i++) 
    {
        if (data[i] == FLAG)
        {
            new_data[new_data_pos++] = ESCAPE_OCTET;
            new_data[new_data_pos++] = FLAG_OCTET_SUB;
        }
        else if (data[i] == ESCAPE_OCTET) 
        {  
            new_data[new_data_pos++] = ESCAPE_OCTET;
            new_data[new_data_pos++] = ESCAPE_OCTET_SUB;

        }
        else new_data[new_data_pos++] = data[i];
    }

    // copy new_data to data,
    // not sure if 3 argument can be new_data_pos
    memcpy(data, new_data, sizeof(new_data));


    return sizeof(new_data);
}

int destuffing(unsigned char *data, int data_size)
{
    unsigned char new_data[PACKET_MAX_SIZE*2+2] = {0};

    // stores the last position where new_data was updated

    int new_data_pos = 0;

    for (int i = 0; i < data_size; i++) 
    {
        if (data[i] == ESCAPE_OCTET && data[i+1] == FLAG_OCTET_SUB)
        {
            new_data[new_data_pos++] = FLAG;
            i++;
        }
        else if (data[i] == ESCAPE_OCTET && data[i+1] == ESCAPE_OCTET_SUB) 
        {  
            new_data[new_data_pos++] = ESCAPE_OCTET;
            i++;
        }
        else new_data[new_data_pos++] = data[i];
    }
    
    memcpy(data, new_data, sizeof(new_data));

    return sizeof(new_data);
}


int checkBCC2(unsigned char *message, int sizeMessage)
{
  int i = 1;
  unsigned char BCC2 = message[0];
  for (; i < sizeMessage - 1; i++)
  {
    BCC2 ^= message[i];
  }
  if (BCC2 == message[sizeMessage - 1])
  {
    return TRUE;
  }
  else
    return FALSE;
}

#endif // _AUX_H_