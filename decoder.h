#ifndef FFPLAYER_DECODER_H_
#define FFPLAYER_DECODER_H_

#include <memory>
#include "context.h"

class Decoder {
public:
    Decoder(std::shared_ptr<Context> ctx, AVMediaType media_type);
    int open();
protected:
    virtual int _open(int stream_index, AVCodecContext *codec_ctx);
protected:
    std::shared_ptr<Context> m_ctx;
    AVMediaType m_media_type;
};

#endif