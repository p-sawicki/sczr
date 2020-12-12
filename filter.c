#include "header.h"

static float freq[] = {20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 
                        1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000};
static float b0[FILTERS], a1[FILTERS], sin_omega[FILTERS];

static float gains[FILTERS];
static float** buffers = NULL;
static float* input = NULL;
mqd_t mq_settings;

void init() {
    for (int id = 0; id < FILTERS; ++id) {
        float omega = 2 * M_PI * freq[id] / Fs;
        sin_omega[id] = sin(omega);

        b0[id] = sin_omega[id] / 2;
        a1[id] = -2 * cos(omega);
    }
}

float from_db(float gain) {
    return pow(10, gain / 20);
}

void* filter(void* freq_id) {
    u_int64_t id = (u_int64_t) freq_id;
    float alpha = sin_omega[id] / 2 * from_db(gains[id]);

    float a0 = 1 + alpha;
    float a2 = 1 - alpha;

    float x0 = b0[id] / a0;
    float x1 = 0;
    float x2 = -x0;
    float y1 = a1[id] / a0;
    float y2 = a2 / a0;

    float* output = buffers[id];
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        output[i] = x0 * input[i];
        if (i > 0)
            output[i] += x1 * input[i - 1] - y1 * input[i - 1];
        if (i > 1)
            output[i] += x2 * input[i - 2] - y2 * input[i - 2];
    }
}

void *settings(void *p) {
    int buf[2];
    while (1) {
        int i = mq_receive(mq_settings, (char*) buf, MQ_SETTINGS_MAX_MSG_SIZE, NULL);
        int err = errno;
        printf("[filter] received setting: set gain %d for freq %d\n", buf[1], (int) freq[buf[0]]);
        gains[buf[0]] = buf[1];
    }
}

int main(int argc, char** argv) {
    printf("[filter] start\n");
    set_sched();

    struct mq_attr settings_attr;
    settings_attr.mq_maxmsg = MQ_MAX_MSG;
    settings_attr.mq_msgsize = MQ_SETTINGS_MAX_MSG_SIZE;
    mq_settings = mq_open(MQ_SETTINGS, O_RDWR | O_CREAT, PERMISSIONS, &settings_attr);
    pthread_t settings_thread;
    int i = pthread_create(&settings_thread, NULL, settings, NULL);
    int err = errno;

    mqd_t mq_input = -1;
    while (mq_input == -1)
        mq_input = mq_open(MQ_INPUT, O_RDONLY);

    int marker = 0;
    while (marker != START_MARKER)
        mq_receive(mq_input, (char*) &marker, MQ_MAX_MSG_SIZE, NULL);

    struct mq_attr attr;
    attr.mq_maxmsg = MQ_MAX_MSG;
    attr.mq_msgsize = MQ_MAX_MSG_SIZE;
    mqd_t mq_output = mq_open(MQ_OUTPUT, O_RDWR | O_CREAT, PERMISSIONS, &attr);

    int fd = open(FILE_INPUT, O_RDONLY);
    float* input_addr = (float*) mmap(NULL, FILE_SIZE, PROT_READ, MAP_SHARED, fd, MMAP_OFFSET);
    close(fd);
    int input_offset = 0;

    fd = open(FILE_OUTPUT, O_RDWR | O_CREAT, PERMISSIONS);
    lseek(fd, FILE_SIZE, SEEK_SET);
    write(fd, FILE_END, sizeof(FILE_END));
    float* output_addr = (float*) mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MMAP_OFFSET);
    close(fd);
    int output_offset = 0;

    FILE* log = fopen(LOG_FILTER, "w");
    int sample_id = 0;
    int samples = __INT_MAX__;
    if (argc > 1)
        samples = atoi(argv[1]);

    marker = START_MARKER;
    mq_send(mq_output, (char*) &marker, sizeof(marker), MQ_MSG_PRIO);

    init();

    buffers = malloc(sizeof(float*) * FILTERS);
    for (int i = 0; i < FILTERS; ++i) {
        buffers[i] = malloc(BUFFER_BYTES);
    }

    while (sample_id < samples) {
        mq_receive(mq_input, (char*) &input_offset, MQ_MAX_MSG_SIZE, NULL);
        input = input_addr + BUFFER_SIZE * input_offset;

        float* output = output_addr + BUFFER_SIZE * output_offset;
        int started = 0;
        pthread_t threads[FILTERS];
        for (u_int64_t i = 0; i < FILTERS; ++i) {
            if (gains[i] != 0)
                pthread_create(&threads[i], NULL, filter, (void*)i);
        }
        for (int i = 0; i < BUFFER_SIZE; ++i)
            output[i] = 0;
        for (int i = 0; i < FILTERS; ++i) {
            if (gains[i] != 0) {
                pthread_join(threads[i], NULL);
                for (int j = 0; j < BUFFER_SIZE; ++j)
                    output[j] += buffers[i][j];
                ++started;
            }
        }
        if (started != 0) {
            for (int i = 0; i < BUFFER_SIZE; ++i)
                output[i] /= started;
        } else
            memcpy(output, input, BUFFER_BYTES);
        msync(output, BUFFER_BYTES, MS_SYNC);

        fprintf(log, "%d %lld\n", sample_id, time_ms());
        mq_send(mq_output, (char*) &output_offset, MQ_MAX_MSG_SIZE, MQ_MSG_PRIO);
        output_offset = (output_offset + 1) % BUFFERS_IN_MEM;
        ++sample_id;
    }

    pthread_cancel(settings_thread);
    fclose(log);
    munmap(output_addr, FILE_SIZE);
    mq_close(mq_input);
    mq_close(mq_output);
    for (int i = 0; i < FILTERS; ++i)
        free(buffers[i]);
    free(buffers);
    printf("[filter] end\n");
    return 0;
}