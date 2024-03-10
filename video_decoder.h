#ifndef FFPLAYER_VIDEO_DECODER_H_
#define FFPLAYER_VIDEO_DECODER_H_

#include "decoder.h"

class VideoDecoder: public Decoder {
public:
    VideoDecoder(std::shared_ptr<Context> ctx);
protected:
    virtual int _open(int stream_index, AVCodecContext *codec_ctx) override;
};

#endif