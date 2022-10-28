#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

int sm_process_states(unsigned char byte, int fd, int *state, unsigned char *saved_buffer, int *stop);

int data_state_machine(unsigned char byte, int *state, unsigned char *info_frame, int *stop, int *sizeInfo);


#endif // _STATE_MACHINE_H_