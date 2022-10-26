#ifndef _PACKET_H_
#define _PACKET_H_

unsigned int get_controlpacket(unsigned char *filename, int start, unsigned char packet[]);

unsigned int get_datapacket(unsigned char *bytes, unsigned char *packet, int nSequence, int count_bytes);

unsigned int handle_packet(unsigned char *packet, unsigned int *size);


#endif // _PACKET_H_