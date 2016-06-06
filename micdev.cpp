#define LOG_TAG "camdev"

// ����ͷ�ļ�
#include <stdlib.h>
#include <utils/Log.h>
#include "micdev.h"

// �ڲ���������
#define DEF_PCM_CARD      0
#define DEF_PCM_DEVICE    0
#define DEF_PCM_CHANNEL   1
#define DEF_PCM_SAMPRATE  44100
#define DEF_PCM_FORMAT    PCM_FORMAT_S16_LE
#define DEF_PCM_BUF_SIZE  2048
#define DEF_PCM_BUF_COUNT 3

// �ڲ�����ʵ��
static void* micdev_capture_thread_proc(void *param)
{
    MICDEV *dev = (MICDEV*)param;

    while (1) {
        if (dev->thread_state & MICDEV_TS_EXIT) {
            break;
        }

        if (dev->thread_state & MICDEV_TS_PAUSE) {
            usleep(33*1000);
            continue;
        }
    }

    return NULL;
}

// ����ʵ��
void* micdev_init(int samprate, int sampsize, int h)
{
    MICDEV *dev = (MICDEV*)malloc(sizeof(MICDEV));
    if (!dev) {
        ALOGE("failed to allocate micdev context !\n");
        goto failed;
    }
    else memset(dev, 0, sizeof(MICDEV));

    dev->config.channels          = DEF_PCM_CHANNEL;
    dev->config.rate              = DEF_PCM_SAMPRATE;
    dev->config.period_size       = DEF_PCM_BUF_SIZE;
    dev->config.period_count      = DEF_PCM_BUF_COUNT;
    dev->config.format            = DEF_PCM_FORMAT;
    dev->config.start_threshold   = 0;
    dev->config.stop_threshold    = 0;
    dev->config.silence_threshold = 0;
    dev->pcm = pcm_open(DEF_PCM_CARD, DEF_PCM_DEVICE, PCM_IN, &dev->config);
    if (!dev->pcm) {
        ALOGE("pcm_open failed !\n");
        goto failed;
    }

    dev->buflen = pcm_frames_to_bytes(dev->pcm, pcm_get_buffer_size(dev->pcm));
    dev->buffer = (uint8_t*)malloc(dev->buflen);
    if (!dev->buffer) {
        ALOGE("unable to allocate %d bytes buffer !\n", dev->buflen);
        goto failed;
    }

    pthread_create(&dev->thread_id, NULL, micdev_capture_thread_proc, dev);

    return dev;

failed:
    if (dev) {
        if (dev->buffer) free(dev->buffer);
        if (dev->pcm   ) pcm_close(dev->pcm);
        free(dev);
    }
    return NULL;
}

void micdev_close(MICDEV *dev)
{
    if (!dev) return;

    // wait thread safely exited
    dev->thread_state |= MICDEV_TS_EXIT;
    pthread_join(dev->thread_id, NULL);

    if (dev->buffer) free(dev->buffer);
    if (dev->pcm   ) pcm_close(dev->pcm);
    free(dev);
}

void micdev_start_capture(MICDEV *dev) {}
void micdev_stop_capture (MICDEV *dev) {}
void micdev_set_encoder  (MICDEV *dev, void *encoder) {}
