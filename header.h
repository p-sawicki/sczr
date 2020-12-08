#ifndef HEADER_H
#define HEADER_H

#include <alsa/asoundlib.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>

#define ALSA_PCM_NEW_HW_PARAMS_API

#define BUFFER_SIZE 2400
#define BUFFER_BYTES 2400 * 4
#define BUFFERS_IN_MEM 16
#define START_MARKER 420

#define Fs 48000
#define FILTERS 31
#define PCM_BLOCKING 0
#define CHANNELS 1
#define LATENCY 200000
#define RESAMPLE 1
#define DEVICE_NAME "default"

#define MQ_MAX_MSG 10
#define MQ_MAX_MSG_SIZE 4
#define MQ_INPUT "/input"
#define MQ_OUTPUT "/output"
#define MQ_MSG_PRIO 0

#define PERMISSIONS 0777
#define FILE_INPUT "/tmp/input"
#define FILE_OUTPUT "/tmp/output"
#define FILE_SIZE 4 * BUFFER_SIZE * BUFFERS_IN_MEM
#define FILE_END "\0"
#define MMAP_OFFSET 0

#endif // HEADER_H