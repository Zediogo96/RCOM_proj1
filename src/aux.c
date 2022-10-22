#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

unsigned char *stuffing(unsigned char *data, unsigned int *size)
{
    unsigned char *new_data;
    unsigned int swaps = 0, new_size = 0;
    int offset = 0;

    // THIS CALCULATES THE NUMBER OF SWAPS IT WILL BE DONE, TO DO A MALLOC BEFOREHAND,
    // I THINK IT COULD BE DONE ANOTHER WAY // TODO: LOOK INTO IT LATER
    for (size_t i = 0; i < *size; i++)
    {
        if (data[i] == FLAG || data[i] == ESCAPE_OCTET)
            swaps++;
    }

    new_size = *size + swaps;
    new_data = malloc(new_size);

    for (size_t i = 0; i < *size; i++)
    {
        if (data[i] == FLAG)
        {
            new_data[i + offset] = ESCAPE_OCTET;
            new_data[i + offset + 1] = FLAG_OCTET_SUB;
            offset++;
        }
        else if (data[i] == ESCAPE_OCTET)
        {
            new_data[i + offset] = ESCAPE_OCTET;
            new_data[i + offset + 1] = ESCAPE_OCTET_SUB;
            offset++;
        }
        else
        {
            new_data[i + offset] = data[i];
        }
    }
    *size = new_size;
    return new_data;
}