// ����ͷ�ļ�
#include "micdev.h"
#include "camdev.h"
#include "ffencoder.h"
#include "ffrecorder.h"

extern "C" {
#include <libavformat/avformat.h>
}

// �ڲ����Ͷ���
typedef struct
{
    FFRECORDER_PARAMS params;
    void             *micdev [MAX_MICDEV_NUM ];
    void             *camdev [MAX_CAMDEV_NUM ];
    void             *encoder[MAX_ENCODER_NUM];
    void             *enclose[MAX_ENCODER_NUM];
    #define FRF_RECORDING   (1 << 1 )
    #define FRF_RECORD_REQ  (1 << 2 )
    #define FRF_RECORD_MACK (1 << 16)
    #define FRF_RECORD_CACK (1 << 17)
    int               state;

    int audio_source[MAX_ENCODER_NUM];
    int video_source[MAX_ENCODER_NUM];
} FFRECORDER;

// �ڲ�ȫ�ֱ�������
static FFRECORDER_PARAMS DEF_FFRECORDER_PARAMS =
{
    // micdev input params
    44100,                      // mic_sample_rate
    2,                          // mic_channel_num

    // camdev input params
    (char*)"/dev/video0",       // cam_dev_name_0
    0,                          // cam_sub_src_0
    1280,                       // cam_frame_width_0
    720,                        // cam_frame_height_0
    25,                         // cam_frame_rate_0

    // camdev input params
    (char*)"/dev/video1",       // cam_dev_name_1
    0,                          // cam_sub_src_1
    1280,                       // cam_frame_width_1
    720,                        // cam_frame_height_1
    25,                         // cam_frame_rate_1

    // ffencoder output
    16000,                      // out_audio_bitrate_0
    AV_CH_LAYOUT_MONO,          // out_audio_chlayout_0
    16000,                      // out_audio_samprate_0
    384000,                     // out_video_bitrate_0
    320,                        // out_video_width_0
    200,                        // out_video_height_0
    15,                         // out_video_frate_0

    // ffencoder output
    16000,                      // out_audio_bitrate_1
    AV_CH_LAYOUT_MONO,          // out_audio_chlayout_1
    16000,                      // out_audio_samprate_1
    384000,                     // out_video_bitrate_1
    320,                        // out_video_width_1
    200,                        // out_video_height_1
    15,                         // out_video_frate_1

    // ffencoder output
    16000,                      // out_audio_bitrate_2
    AV_CH_LAYOUT_MONO,          // out_audio_chlayout_2
    16000,                      // out_audio_samprate_2
    384000,                     // out_video_bitrate_2
    320,                        // out_video_width_2
    200,                        // out_video_height_2
    15,                         // out_video_frate_2
};

// �ڲ�����ʵ��
// micdev capture callback
int micdev0_capture_callback_proc(void *r, void *data[8], int nbsample)
{
    FFRECORDER *recorder = (FFRECORDER*)r;
    if (recorder->state & FRF_RECORDING) {
        if (!(recorder->state & FRF_RECORD_REQ)) {
            recorder->state |= FRF_RECORD_MACK;
        }
        for (int i=0; i<MAX_ENCODER_NUM; i++) {
            if (recorder->audio_source[i] == 0) {
                ffencoder_audio(recorder->encoder[i], data, nbsample);
            }
        }
        recorder->state &= ~FRF_RECORD_MACK;
    }
    return 0;
}

// camdev capture callback
int camdev0_capture_callback_proc(void *r, void *data[8], int linesize[8])
{
    FFRECORDER *recorder = (FFRECORDER*)r;
    if (recorder->state & FRF_RECORDING) {
        if (!(recorder->state & FRF_RECORD_REQ)) {
            recorder->state |= FRF_RECORD_CACK;
        }
        for (int i=0; i<MAX_ENCODER_NUM; i++) {
            if (recorder->video_source[i] == 0) {
                ffencoder_video(recorder->encoder[i], data, linesize);
            }
        }
        recorder->state &= ~FRF_RECORD_CACK;
    }
    return 0;
}

int camdev1_capture_callback_proc(void *r, void *data[8], int linesize[8])
{
    FFRECORDER *recorder = (FFRECORDER*)r;
    if (recorder->state & FRF_RECORDING) {
        if (!(recorder->state & FRF_RECORD_REQ)) {
            recorder->state |= FRF_RECORD_CACK;
        }
        for (int i=0; i<MAX_ENCODER_NUM; i++) {
            if (recorder->video_source[i] == 1) {
                ffencoder_video(recorder->encoder[i], data, linesize);
            }
        }
        recorder->state &= ~FRF_RECORD_CACK;
    }
    return 0;
}

// ����ʵ��
void *ffrecorder_init(FFRECORDER_PARAMS *params)
{
    FFRECORDER *recorder = (FFRECORDER*)malloc(sizeof(FFRECORDER));
    if (recorder) memset(recorder, 0, sizeof(FFRECORDER));
    else return NULL;

    // using default params if not set
    if (!params                      ) params                       = &DEF_FFRECORDER_PARAMS;
    if (!params->mic_sample_rate     ) params->mic_sample_rate      = DEF_FFRECORDER_PARAMS.mic_sample_rate;
    if (!params->mic_sample_rate     ) params->mic_channel_num      = DEF_FFRECORDER_PARAMS.mic_channel_num;
    if (!params->cam_dev_name_0      ) params->cam_dev_name_0       = DEF_FFRECORDER_PARAMS.cam_dev_name_0;
    if (!params->cam_sub_src_0       ) params->cam_sub_src_0        = DEF_FFRECORDER_PARAMS.cam_sub_src_0;
    if (!params->cam_frame_width_0   ) params->cam_frame_width_0    = DEF_FFRECORDER_PARAMS.cam_frame_width_0;
    if (!params->cam_frame_height_0  ) params->cam_frame_height_0   = DEF_FFRECORDER_PARAMS.cam_frame_height_0;
    if (!params->cam_frame_rate_0    ) params->cam_frame_rate_0     = DEF_FFRECORDER_PARAMS.cam_frame_rate_0;
    if (!params->cam_dev_name_1      ) params->cam_dev_name_1       = DEF_FFRECORDER_PARAMS.cam_dev_name_1;
    if (!params->cam_sub_src_1       ) params->cam_sub_src_1        = DEF_FFRECORDER_PARAMS.cam_sub_src_1;
    if (!params->cam_frame_width_1   ) params->cam_frame_width_1    = DEF_FFRECORDER_PARAMS.cam_frame_width_1;
    if (!params->cam_frame_height_1  ) params->cam_frame_height_1   = DEF_FFRECORDER_PARAMS.cam_frame_height_1;
    if (!params->cam_frame_rate_1    ) params->cam_frame_rate_1     = DEF_FFRECORDER_PARAMS.cam_frame_rate_1;
    if (!params->out_audio_bitrate_0 ) params->out_audio_bitrate_0  = DEF_FFRECORDER_PARAMS.out_audio_bitrate_0;
    if (!params->out_audio_chlayout_0) params->out_audio_chlayout_0 = DEF_FFRECORDER_PARAMS.out_audio_chlayout_0;
    if (!params->out_audio_samprate_0) params->out_audio_samprate_0 = DEF_FFRECORDER_PARAMS.out_audio_samprate_0;
    if (!params->out_video_bitrate_0 ) params->out_video_bitrate_0  = DEF_FFRECORDER_PARAMS.out_video_bitrate_0;
    if (!params->out_video_width_0   ) params->out_video_width_0    = DEF_FFRECORDER_PARAMS.out_video_width_0;
    if (!params->out_video_height_0  ) params->out_video_height_0   = DEF_FFRECORDER_PARAMS.out_video_height_0;
    if (!params->out_video_frate_0   ) params->out_video_frate_0    = DEF_FFRECORDER_PARAMS.out_video_frate_0;
    if (!params->out_audio_bitrate_1 ) params->out_audio_bitrate_1  = DEF_FFRECORDER_PARAMS.out_audio_bitrate_1;
    if (!params->out_audio_chlayout_1) params->out_audio_chlayout_1 = DEF_FFRECORDER_PARAMS.out_audio_chlayout_1;
    if (!params->out_audio_samprate_1) params->out_audio_samprate_1 = DEF_FFRECORDER_PARAMS.out_audio_samprate_1;
    if (!params->out_video_bitrate_1 ) params->out_video_bitrate_1  = DEF_FFRECORDER_PARAMS.out_video_bitrate_1;
    if (!params->out_video_width_1   ) params->out_video_width_1    = DEF_FFRECORDER_PARAMS.out_video_width_1;
    if (!params->out_video_height_1  ) params->out_video_height_1   = DEF_FFRECORDER_PARAMS.out_video_height_1;
    if (!params->out_video_frate_1   ) params->out_video_frate_1    = DEF_FFRECORDER_PARAMS.out_video_frate_1;
    if (!params->out_audio_bitrate_2 ) params->out_audio_bitrate_2  = DEF_FFRECORDER_PARAMS.out_audio_bitrate_2;
    if (!params->out_audio_chlayout_2) params->out_audio_chlayout_2 = DEF_FFRECORDER_PARAMS.out_audio_chlayout_2;
    if (!params->out_audio_samprate_2) params->out_audio_samprate_2 = DEF_FFRECORDER_PARAMS.out_audio_samprate_2;
    if (!params->out_video_bitrate_2 ) params->out_video_bitrate_2  = DEF_FFRECORDER_PARAMS.out_video_bitrate_2;
    if (!params->out_video_width_2   ) params->out_video_width_2    = DEF_FFRECORDER_PARAMS.out_video_width_2;
    if (!params->out_video_height_2  ) params->out_video_height_2   = DEF_FFRECORDER_PARAMS.out_video_height_2;
    if (!params->out_video_frate_2   ) params->out_video_frate_2    = DEF_FFRECORDER_PARAMS.out_video_frate_2;
    memcpy(&recorder->params, params, sizeof(FFRECORDER_PARAMS));

    recorder->audio_source[0] = 0;
    recorder->audio_source[1] = 0;
    recorder->audio_source[2] =-1;
    recorder->video_source[0] = 0;
    recorder->video_source[1] = 1;
    recorder->video_source[2] =-1;

    recorder->micdev[0] = micdev_init(params->mic_sample_rate, params->mic_channel_num, NULL);
    if (!recorder->micdev[0]) {
        printf("failed to init micdev !\n");
    }

    recorder->camdev[0] = camdev_init(params->cam_dev_name_0, params->cam_sub_src_0,
        params->cam_frame_width_0, params->cam_frame_height_0, params->cam_frame_rate_0);
    if (!recorder->camdev[0]) {
        printf("failed to init camdev0 !\n");
    }

    recorder->camdev[1] = camdev_init(params->cam_dev_name_1, params->cam_sub_src_1,
        params->cam_frame_width_1, params->cam_frame_height_1, params->cam_frame_rate_1);
    if (!recorder->camdev[1]) {
        printf("failed to init camdev1 !\n");
    }

    // start micdev capture
    micdev_start_capture(recorder->micdev[0]);

    // start camdev capture
    camdev_capture_start(recorder->camdev[0]);
    camdev_capture_start(recorder->camdev[1]);

    // set callback
    micdev_set_callback(recorder->micdev[0], (void*)micdev0_capture_callback_proc, recorder);
    camdev_set_callback(recorder->camdev[0], (void*)camdev0_capture_callback_proc, recorder);
    camdev_set_callback(recorder->camdev[1], (void*)camdev1_capture_callback_proc, recorder);

    return recorder;
}

void ffrecorder_free(void *ctxt)
{
    FFRECORDER *recorder = (FFRECORDER*)ctxt;
    if (!recorder) return;

    // stop micdev capture
    micdev_stop_capture(recorder->micdev[0]);

    // stop camdev capture
    camdev_capture_stop(recorder->camdev[0]);
    camdev_capture_stop(recorder->camdev[1]);

    // free camdev & micdev
    micdev_close(recorder->micdev[0]);
    camdev_close(recorder->camdev[0]);
    camdev_close(recorder->camdev[1]);

    // free context
    free(recorder);
}

void ffrecorder_reset_camdev(void *ctxt, int camidx, int w, int h, int frate)
{
    FFRECORDER *recorder = (FFRECORDER*)ctxt;
    char       *dev_name = NULL;
    int         sub_src  = 0;
    if (!recorder || camidx < 0 || camidx >= MAX_CAMDEV_NUM) return;

    camdev_capture_stop(recorder->camdev[camidx]);
    camdev_close(recorder->camdev[camidx]);

    switch (camidx) {
    case 0:
        w     = (w     == -1) ? recorder->params.cam_frame_width_0  : w;
        h     = (h     == -1) ? recorder->params.cam_frame_height_0 : h;
        frate = (frate == -1) ? recorder->params.cam_frame_rate_0   : frate;
        dev_name = recorder->params.cam_dev_name_0;
        sub_src  = recorder->params.cam_sub_src_0;
        recorder->params.cam_frame_width_0  = w;
        recorder->params.cam_frame_height_0 = h;
        recorder->params.cam_frame_rate_0   = frate;
        break;
    case 1:
        w     = (w     == -1) ? recorder->params.cam_frame_width_1  : w;
        h     = (h     == -1) ? recorder->params.cam_frame_height_1 : h;
        frate = (frate == -1) ? recorder->params.cam_frame_rate_1   : frate;
        dev_name = recorder->params.cam_dev_name_1;
        sub_src  = recorder->params.cam_sub_src_1;
        recorder->params.cam_frame_width_1  = w;
        recorder->params.cam_frame_height_1 = h;
        recorder->params.cam_frame_rate_1   = frate;
        break;
    }

    recorder->camdev[camidx] = camdev_init(dev_name, sub_src, w, h, frate);
    camdev_capture_start(recorder->camdev[camidx]);
    camdev_set_callback(recorder->camdev[camidx], (void*)(camidx ? camdev1_capture_callback_proc : camdev0_capture_callback_proc), recorder);
}

void ffrecorder_preview_window(void *ctxt, int camidx, const sp<ANativeWindow> win)
{
    FFRECORDER *recorder = (FFRECORDER*)ctxt;
    if (!recorder || camidx < 0 || camidx >= MAX_CAMDEV_NUM) return;
    camdev_set_preview_window(recorder->camdev[camidx], win);
}

void ffrecorder_preview_target(void *ctxt, int camidx, const sp<IGraphicBufferProducer>& gbp)
{
    FFRECORDER *recorder = (FFRECORDER*)ctxt;
    if (!recorder || camidx < 0 || camidx >= MAX_CAMDEV_NUM) return;
    camdev_set_preview_target(recorder->camdev[camidx], gbp);
}

void ffrecorder_preview_start(void *ctxt, int camidx)
{
    FFRECORDER *recorder = (FFRECORDER*)ctxt;
    if (!recorder || camidx < 0 || camidx >= MAX_CAMDEV_NUM) return;
    camdev_preview_start(recorder->camdev[camidx]);
}

void ffrecorder_preview_stop(void *ctxt, int camidx)
{
    FFRECORDER *recorder = (FFRECORDER*)ctxt;
    if (!recorder || camidx < 0 || camidx >= MAX_CAMDEV_NUM) return;
    camdev_preview_stop(recorder->camdev[camidx]);
}

void ffrecorder_record_start(void *ctxt, int encidx, char *filename)
{
    FFRECORDER *recorder  = (FFRECORDER*)ctxt;
    int         vidsrc    = 0;
    void       *camdev    = NULL;
    int         abitrate  = 0;
    int         achlayout = 0;
    int         asamprate = 0;
    int         vbitrate  = 0;
    int         vwidth    = 0;
    int         vheight   = 0;
    int         vfrate    = 0;
    if (!recorder || encidx < -1 || encidx >= MAX_ENCODER_NUM) return;

    if (encidx == -1) {
        for (int i=0; i<MAX_ENCODER_NUM; i++) {
            ffencoder_free(recorder->enclose[i]);
            recorder->enclose[i] = NULL;
        }
        return;
    }

    vidsrc = recorder->video_source[encidx];
    if (vidsrc >= 0 && vidsrc < MAX_CAMDEV_NUM) {
        camdev = recorder->camdev[vidsrc];
    }

    switch (encidx) {
    case 0:
        abitrate  = recorder->params.out_audio_bitrate_0;
        achlayout = recorder->params.out_audio_chlayout_0;
        asamprate = recorder->params.out_audio_samprate_0;
        vbitrate  = recorder->params.out_video_bitrate_0;
        vwidth    = recorder->params.out_video_width_0;
        vheight   = recorder->params.out_video_height_0;
        vfrate    = recorder->params.out_video_frate_0;
        break;
    case 1:
        abitrate  = recorder->params.out_audio_bitrate_1;
        achlayout = recorder->params.out_audio_chlayout_1;
        asamprate = recorder->params.out_audio_samprate_1;
        vbitrate  = recorder->params.out_video_bitrate_1;
        vwidth    = recorder->params.out_video_width_1;
        vheight   = recorder->params.out_video_height_1;
        vfrate    = recorder->params.out_video_frate_1;
        break;
    case 2:
        abitrate  = recorder->params.out_audio_bitrate_2;
        achlayout = recorder->params.out_audio_chlayout_2;
        asamprate = recorder->params.out_audio_samprate_2;
        vbitrate  = recorder->params.out_video_bitrate_2;
        vwidth    = recorder->params.out_video_width_2;
        vheight   = recorder->params.out_video_height_2;
        vfrate    = recorder->params.out_video_frate_2;
        break;
    }

    // switch to a new ffencoder for recording
    FFENCODER_PARAMS encoder_params;
    encoder_params.in_audio_channel_layout = AV_CH_LAYOUT_STEREO;
    encoder_params.in_audio_sample_fmt     = AV_SAMPLE_FMT_S16;
    encoder_params.in_audio_sample_rate    = recorder->params.mic_sample_rate;
    encoder_params.in_video_width          = camdev_get_param(camdev, CAMDEV_PARAM_VIDEO_WIDTH );
    encoder_params.in_video_height         = camdev_get_param(camdev, CAMDEV_PARAM_VIDEO_HEIGHT);
    encoder_params.in_video_pixfmt         = v4l2dev_pixfmt_to_ffmpeg_pixfmt(camdev_get_param(camdev, CAMDEV_PARAM_VIDEO_PIXFMT));
    encoder_params.in_video_frame_rate     = camdev_get_param(camdev, CAMDEV_PARAM_VIDEO_FRATE );
    encoder_params.out_filename            = filename;
    encoder_params.out_audio_bitrate       = abitrate;
    encoder_params.out_audio_channel_layout= achlayout;
    encoder_params.out_audio_sample_rate   = asamprate;
    encoder_params.out_video_bitrate       = vbitrate;
    encoder_params.out_video_width         = vwidth;
    encoder_params.out_video_height        = vheight;
    encoder_params.out_video_frame_rate    = vfrate;
    encoder_params.start_apts              = 0;
    encoder_params.start_vpts              = 0;
    encoder_params.scale_flags             = 0; // use default
    encoder_params.audio_buffer_number     = 0; // use default
    encoder_params.video_buffer_number     = 0; // use default
    recorder->enclose[encidx] = recorder->encoder[encidx];
    recorder->encoder[encidx] = ffencoder_init(&encoder_params);
    if (!recorder->encoder[encidx]) {
        printf("failed to init encoder !\n");
    }

    // set request recording flag and wait switch done
    recorder->state |= (FRF_RECORD_REQ|FRF_RECORDING);
    while (recorder->state & (FRF_RECORD_MACK|FRF_RECORD_CACK)) usleep(10*1000);
    recorder->state &=~(FRF_RECORD_REQ);
}

void ffrecorder_record_stop(void *ctxt, int encidx)
{
    FFRECORDER *recorder = (FFRECORDER*)ctxt;
    if (!recorder || encidx < -1 || encidx >= MAX_ENCODER_NUM) return;

    if (encidx == -1) {
        for (int i=0; i<MAX_ENCODER_NUM; i++) {
            ffencoder_free(recorder->enclose[i]);
            recorder->enclose[i] = NULL;
        }
        return;
    }

    recorder->enclose[encidx] = recorder->encoder[encidx];
    recorder->encoder[encidx] = NULL;

    recorder->state |= (FRF_RECORD_REQ|FRF_RECORDING);
    while (recorder->state & (FRF_RECORD_MACK|FRF_RECORD_CACK)) usleep(10*1000);
    recorder->state &=~(FRF_RECORD_REQ|FRF_RECORDING);
}

void ffrecorder_record_audio_source(void *ctxt, int encidx, int source)
{
    FFRECORDER *recorder = (FFRECORDER*)ctxt;
    if (!recorder || encidx < 0 || encidx >= MAX_ENCODER_NUM) return;
    recorder->audio_source[encidx] = source;
}

void ffrecorder_record_video_source(void *ctxt, int encidx, int source)
{
    FFRECORDER *recorder = (FFRECORDER*)ctxt;
    if (!recorder || encidx < 0 || encidx >= MAX_ENCODER_NUM) return;
    recorder->video_source[encidx] = source;
}
