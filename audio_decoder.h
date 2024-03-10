#ifndef FFPLAYER_AUDIO_DECODER_H_
#define FFPLAYER_AUDIO_DECODER_H_

#include "decoder.h"

class AudioDecoder: public Decoder {
public:
    AudioDecoder(std::shared_ptr<Context> ctx);
protected:
    virtual int _open(int stream_index, AVCodecContext *codec_ctx) override;
};

#endif