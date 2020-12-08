#include "header.h"

int main() {
  struct mq_attr attr;
  attr.mq_maxmsg = MQ_MAX_MSG;
  attr.mq_msgsize = MQ_MAX_MSG_SIZE;
  mqd_t mq_descriptor = mq_open(MQ_INPUT, O_RDWR | O_CREAT, PERMISSIONS, &attr);

  int fd = open(FILE_INPUT, O_RDWR | O_CREAT, PERMISSIONS);
  lseek(fd, FILE_SIZE, SEEK_SET);
  write(fd, FILE_END, sizeof(FILE_END));

  float* input_addr = (float*) mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MMAP_OFFSET);
  int buffer_offset = 0;
  close(fd);

  int start_marker = START_MARKER;
  mq_send(mq_descriptor, (char*) &start_marker, MQ_MAX_MSG_SIZE, MQ_MSG_PRIO);

  snd_pcm_t *handle;
  snd_pcm_open(&handle, DEVICE_NAME, SND_PCM_STREAM_CAPTURE, PCM_BLOCKING);
  snd_pcm_set_params(handle, SND_PCM_FORMAT_FLOAT, SND_PCM_ACCESS_RW_INTERLEAVED, CHANNELS, Fs, RESAMPLE, LATENCY);

  while (1) {
    float* addr = input_addr + buffer_offset * BUFFER_SIZE;
    if (snd_pcm_readi(handle, addr, BUFFER_SIZE) == -1)
      printf("[capture] ERROR");
    msync(addr, BUFFER_BYTES, MS_SYNC);

    mq_send(mq_descriptor, (char*) &buffer_offset, MQ_MAX_MSG_SIZE, MQ_MSG_PRIO);
    //printf("[capture] sent %d\n", buffer_offset);
    buffer_offset = (buffer_offset + 1) % BUFFERS_IN_MEM;
  }

  munmap(input_addr, FILE_SIZE);
  mq_close(mq_descriptor);
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  return 0;
}