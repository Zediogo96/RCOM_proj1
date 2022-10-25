#ifndef _MACROS_H_
#define _MACROS_H_

#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1

#define BUFFER_SIZE 256

#define DISC 0x0B

#define NUM_MAX_TRIES 5

/* ************** CHECKAR MELHOR ************** */

#define ADDRESS_T_to_R 0x03
#define ADDRESS_R_to_T 0x01

#define FLAG 0x7E

/* ************** FIELDS ************** */

#define A 0x03                     // ADDRESS
#define C_SET 0x03                 // SET UP
#define C_UA 0x07                  // UNUMBERED ACKNOWLEDGEMENT
#define C_DISC 0x0B                // DISCONNECT
#define C_RR(n)(0x06 | (n >> 7))  // RECEIVER READY / POSITIVE ACKNOWLEDGMENT
#define C_REJ(n)(0x01 | (n >> 7)) // RECEIVER REJECTED / NEGATIVE ACKNOWLEDGEMENT

/* ************** INFORMATION FRAME ************** */

#define C_ZERO 0x00
#define C_ONE 0x40

/* ************** PACKETS ************** */

#define C_DATA 0x01  // DATA

// TO BE USED IN GETCONTROLPACKET
#define C_START 0x02 // CONTROL
#define C_END 0x03   // CONTROL


#define T_SIZE 0x00  // CONTROL
#define T_NAME 0x01  // CONTROL

#define PACKET_MAX_SIZE 128

/** 
 * @brief DON'T FORGET BCC1 = HEADER, BCC2 = DATA
*/

/* ************** FOR STUFFING USE ************** */

#define ESCAPE_OCTET 0x7D
#define FLAG_OCTET_SUB 0x5E // this is the result of 0x7E (octet to be subbed) XOR (^) with 0x20
#define ESCAPE_OCTET_SUB 0x5D // this is the result of 0x7D (octet to be subbed) XOR (^) with 0x20

#endif // _MACROS_H_
