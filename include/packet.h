#ifndef _PACKET_H_
#define _PACKET_H_

unsigned int get_controlpacket(unsigned char *filename, int fileSize, int start, unsigned char packet[]);

unsigned int get_datapacket(unsigned char *file_data, unsigned int data_size, unsigned int count);

unsigned int handle_packet(unsigned char *packet, unsigned int *size);


#endif // _PACKET_H_