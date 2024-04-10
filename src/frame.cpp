#include "frame.h"

double Frame::vf_duration(Frame *nextvp, double max_frame_duration) {
    if (!nextvp) {
        return 0.0;
    }
    if (nextvp->serial != serial) {
        return 0.0;
    }
    double duration = nextvp->pts - pts;
    if (isnan(duration) || duration <= 0 || duration > max_frame_duration) {
        return this->duration;
    }
    return duration;
}

void Frame::reset() {
    av_frame_unref(frame);
    serial = 0; // 该帧的序列号，用于seek操作
    pts = 0.0; // 该帧的显示时间戳（Presentation Time Stamp），表示该帧应该在播放时显示的时间
    duration = 0.0; // 该帧的持续时间，表示该帧的显示时长
    uploaded = 0; // 这一帧是否已经上传到渲染器中了
    pos = 0; // 该帧在文件中的位置，用于定位该帧的来源 /* byte position of the frame in the input file */
}