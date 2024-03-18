#ifndef FFPLAYER_OPTS_
#define FFPLAYER_OPTS_

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

extern AVDictionary *format_opts;
extern int framedrop;

// frame队列参数
#define VIDEO_FRAME_QUEUE_SIZE 3
#define SUBTITLE_FRAME_QUEUE_SIZE 16
#define AUDIO_FRAME_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(AUDIO_FRAME_QUEUE_SIZE, FFMAX(VIDEO_FRAME_QUEUE_SIZE, SUBTITLE_FRAME_QUEUE_SIZE))

/* no AV sync correction is done if below the minimum AV sync threshold */
// 如果低于最小 AV 同步阈值，则不进行 AV 同步校正
#define AV_SYNC_THRESHOLD_MIN 0.04

/* AV sync correction is done if above the maximum AV sync threshold */
// 如果高于最大 AV 同步阈值，则完成 AV 同步校正
#define AV_SYNC_THRESHOLD_MAX 0.1

/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
// 如果帧持续时间长于此，则不会复制该帧以补偿 AV 同步
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

/* no AV correction is done if too big error */
// 如果误差太大，则不进行 AV 校正
#define AV_NOSYNC_THRESHOLD 10.0

/* Minimum SDL audio buffer size, in samples. */
// 最小 SDL 音频缓冲区大小（以样本为单位）
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
// 计算实际缓冲区大小，记住不要导致过于频繁的音频回调
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

#endif