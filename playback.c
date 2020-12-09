#include "header.h"

int main() {
  mqd_t mq_output = -1;
  while (mq_output == -1)
    mq_output = mq_open(MQ_OUTPUT, O_RDONLY);

  int marker = 0;
  while (marker != START_MARKER)
    mq_receive(mq_output, (char*) &marker, MQ_MAX_MSG_SIZE, NULL);

  int fd = open(FILE_OUTPUT, O_RDONLY);
  float* output_addr = (float*) mmap(NULL, FILE_SIZE, PROT_READ, MAP_SHARED, fd, MQ_MSG_PRIO);
  close(fd);

  snd_pcm_t *handle;
  snd_pcm_open(&handle, DEVICE_NAME, SND_PCM_STREAM_PLAYBACK, PCM_BLOCKING);
  snd_pcm_set_params(handle, SND_PCM_FORMAT_FLOAT, SND_PCM_ACCESS_RW_INTERLEAVED, CHANNELS, Fs, RESAMPLE, LATENCY);
  sleep(3);

  while (1) {
    int offset = 0;
    mq_receive(mq_output, (char*) &offset, MQ_MAX_MSG_SIZE, NULL);
    //printf("[playback] received %d\n", offset);
    float *addr = output_addr + offset * BUFFER_SIZE;
    if(snd_pcm_writei(handle, addr, BUFFER_SIZE) == -1)
      printf("[playback] ERROR");
  }

  mq_close(mq_output);
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  return 0;
}