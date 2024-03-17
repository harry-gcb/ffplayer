#ifndef FFPLAYER_FRAME_H_
#define FFPLAYER_FRAME_H_

#include <mutex>
#include <condition_variable>
#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/frame.h>

#ifdef __cplusplus
}
#endif
#include "opts.h"
#include "packet_queue.h"

class Frame {
public:
    double vf_duration(Frame *nextvp, double max_frame_duration);

    AVFrame *frame = nullptr; // 指向 FFmpeg 中的 AVFrame 结构体，用于存储视频帧的图像数据
    int serial = 0; // 该帧的序列号，用于seek操作
    double pts = 0.0; // 该帧的显示时间戳（Presentation Time Stamp），表示该帧应该在播放时显示的时间
    double duration = 0.0; // 该帧的持续时间，表示该帧的显示时长
    int uploaded = 0; // 这一帧是否已经上传到渲染器中了
    int64_t pos = 0; // 该帧在文件中的位置，用于定位该帧的来源 /* byte position of the frame in the input file */
};



#endif // FFPLAYER_FRAME_H_