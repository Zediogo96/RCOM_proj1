#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

int sm_process_states(unsigned char byte, int fd, int *state, unsigned char *saved_buffer, int *stop);
int llclose_state_machine(unsigned char byte, int fd);

#endif // _STATE_MACHINE_H_