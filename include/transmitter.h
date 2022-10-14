int fd;
int nRetransmissions;

int sendSET();

int transmitter_ctrl_receive();

void transmitter_alarm_handler(int signal);

int transmitter_start(int fd_, int nRetransmissions_, int timeout);
