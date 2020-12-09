#include "header.h"

int main(int argc, char** argv){
    if (argc < 3) {
        printf("Not enough arguments!\n");
        exit(1);
    } 
    mqd_t mq_settings = mq_open(MQ_SETTINGS, O_WRONLY);
    if (mq_settings == -1) {
        printf("Could not open message queue!\n");
        exit(1);
    }
    int buf[2];
    buf[0] = atoi(argv[1]);
    buf[1] = atoi(argv[2]);
    mq_send(mq_settings, (char*) buf, MQ_SETTINGS_MAX_MSG_SIZE, MQ_MSG_PRIO);
    mq_close(mq_settings);
}