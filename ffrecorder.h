#ifndef __FFRECORDER_H__
#define __FFRECORDER_H__

// ����ͷ�ļ�
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <ui/DisplayInfo.h>

using namespace android;

// ��������
#define MAX_MICDEV_NUM     1
#define MAX_CAMDEV_NUM     2
#define MAX_ENCODER_NUM    3

// ���Ͷ���
typedef struct
{
    // micdev input params
    int   mic_sample_rate;
    int   mic_channel_num;

    // camdev input params
    char *cam_dev_name_0;
    int   cam_sub_src_0;
    int   cam_frame_width_0;
    int   cam_frame_height_0;
    int   cam_frame_rate_0;

    char *cam_dev_name_1;
    int   cam_sub_src_1;
    int   cam_frame_width_1;
    int   cam_frame_height_1;
    int   cam_frame_rate_1;

    // ffencoder output
    int   out_audio_bitrate_0;
    int   out_audio_chlayout_0;
    int   out_audio_samprate_0;
    int   out_video_bitrate_0;
    int   out_video_width_0;
    int   out_video_height_0;
    int   out_video_frate_0;

    int   out_audio_bitrate_1;
    int   out_audio_chlayout_1;
    int   out_audio_samprate_1;
    int   out_video_bitrate_1;
    int   out_video_width_1;
    int   out_video_height_1;
    int   out_video_frate_1;

    int   out_audio_bitrate_2;
    int   out_audio_chlayout_2;
    int   out_audio_samprate_2;
    int   out_video_bitrate_2;
    int   out_video_width_2;
    int   out_video_height_2;
    int   out_video_frate_2;
} FFRECORDER_PARAMS;

// ��������
void *ffrecorder_init(FFRECORDER_PARAMS *params);
void  ffrecorder_free(void *ctxt);
void  ffrecorder_reset_camdev  (void *ctxt, int camidx, int w, int h, int frate);
void  ffrecorder_preview_window(void *ctxt, int camidx, const sp<ANativeWindow> win);
void  ffrecorder_preview_target(void *ctxt, int camidx, const sp<IGraphicBufferProducer>& gbp);
void  ffrecorder_preview_start (void *ctxt, int camidx);
void  ffrecorder_preview_stop  (void *ctxt, int camidx);
void  ffrecorder_record_start  (void *ctxt, int encidx, char *filename);
void  ffrecorder_record_stop   (void *ctxt, int encidx);
void  ffrecorder_record_audio_source(void *ctxt, int encidx, int source);
void  ffrecorder_record_video_source(void *ctxt, int encidx, int source);

#endif

