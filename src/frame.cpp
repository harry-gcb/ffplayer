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