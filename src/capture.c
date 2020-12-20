#include <alsa/asoundlib.h>
#include <mqueue.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "common.h"


int main(int argc, char* argv[]) {
    mqd_t mq_input;
    sczr_shared_t* shm_input;
    FILE* log;

    set_sched();
    
    pcm_device_t device = {
        .name = CAPTURE_DEVICE_NAME,
        .stream = SND_PCM_STREAM_CAPTURE,
        .access = SND_PCM_ACCESS_RW_INTERLEAVED,
        .format = SND_PCM_FORMAT_S16_LE,
        .rate = Fs,
        .channels = CAPTURE_CHANNELS,
        .buffer_time = BUFFER_TIME,
        .period_time = PERIOD_TIME
    };

    if(init_pcm_device(&device) < 0) {
        fprintf(stderr, "[capture] could not init capture device\n");
        return 1;
    }
    
    if(make_queue(MQ_INPUT, &mq_input) != 0) {
        snd_pcm_close(device.handle);
        fprintf(stderr, "[capture] could not init input queue\n");
        return 1;
    }

    if(make_shared(SHM_INPUT, device.channels * device.period_size * sizeof(short), &shm_input) != 0) {
        snd_pcm_close(device.handle);
        mq_close(mq_input);
        fprintf(stderr, "[capture] could not init shared memory\n");
        return 1;
    }

    snd_pcm_prepare(device.handle);

    if(snd_pcm_start(device.handle) < 0) {
        destroy_shared(shm_input);
        snd_pcm_close(device.handle);
        mq_close(mq_input);
        fprintf(stderr, "[capture] could not start capture device\n");
        return 1;
    }

    if((log = fopen(LOG_CAPTURE, "w")) == NULL) {
        destroy_shared(shm_input);
        snd_pcm_close(device.handle);
        mq_close(mq_input);
        fprintf(stderr, "[capture] could not open log file\n");
        return 1;
    }

    const int start_marker = START_MARKER;
    mq_send(mq_input, (char*)&start_marker, MQ_MAX_MSG_SIZE, MQ_MSG_PRIO);
    mq_send(mq_input, (char*)&device.period_size, MQ_MAX_MSG_SIZE, MQ_MSG_PRIO);

    printf("[capture] running\n");

    unsigned int buffer_index = 0;
    unsigned int block_id = 1;
    int error;
    int periods = INT32_MAX;;

    if(argc > 1) {
        periods = atoi(argv[1]);
    }
    
    while(periods > 0) {
        if((error = snd_pcm_avail(device.handle)) < 0) {
            snd_pcm_recover(device.handle, error, 1);
            continue;
        }
        else if(error < device.period_size) {
            continue;
        }

        shm_input->buffers[buffer_index].block_id = block_id;
        int16_t* buffer_addr = (void*)shm_input + shm_input->buffers[buffer_index].offset;
        
        semaphore_wait(shm_input->semaphores_id, buffer_index);

        if((error = snd_pcm_readi(device.handle, buffer_addr, device.period_size)) < 0) {
            if(error == -EPIPE) {
                snd_pcm_prepare(device.handle);
            }
            else {
                printf("[capture] error: %s\n", snd_strerror(error));
            }
        }
        else if(error < device.period_size) {
            printf("[capture] read: %i\n", error);
        }

        fprintf(log, "%u %lld\n", block_id, time_us());
        semaphore_signal(shm_input->semaphores_id, buffer_index); 
        mq_send(mq_input, (char*)&buffer_index, sizeof(buffer_index), MQ_MSG_PRIO);
        buffer_index = (buffer_index + 1) % BUFFERS_IN_MEM;
        block_id++;
        periods--;
    }

    close_shared(shm_input);
    mq_close(mq_input);
    snd_pcm_drain(device.handle);
    snd_pcm_close(device.handle);
    fclose(log);
    return 0;
}
