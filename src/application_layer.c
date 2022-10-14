// Application layer protocol implementation

#include "application_layer.h"

int stats = 1;  //for now, always show statistics
//change for macros
//statistics will be the number of packets sent

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer connection;

    snprintf(connection.serialPort, sizeof(connection.serialPort), %s, serialPort);  //copy serial port number to link layer struct

    //see what is the role of the device running the application

    if (strcmp(role, "tx")==0) {
        connection.role = LlTx;  //transmitter
    }
    else if (strcmp(role, "rx")==0) {
        connection.role = LlRx  //receiver
    }
    else {
        return;
    }

    //set link layer properties
    connection.baudRate = baudRate;
    connection.nRetransmissions = nTries;
    connection.timeout = timeout;

    if (llopen_read == (-1)) {
        //error
        //close connection
        llclose(stats);
        return;
    }

    //if transmitter, send data

    if (connection.role = LlTx) {
        //send stuff
        //represent file
        //get control packet
        //write packets to buffer
        //use llwrite to transmit the buffer
    }
    else if (connection.role = LlRx) {
        //receive stuff
        //use llread to read from the buffer
    }

    //close connection

    if (llclose(stats) == (-1)) {
        //error
        return;
    }
}
