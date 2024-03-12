#ifndef FFPLAYER_DECODER_H_
#define FFPLAYER_DECODER_H_

#include <memory>
#include "context.h"
#include "thread_base.h"

class Decoder: public ThreadBase {
public:
    Decoder(std::shared_ptr<Context> ctx, AVMediaType media_type);
    virtual ~Decoder();
    int open();
    int close();
    int decode(AVCodecContext *codec_ctx, AVFrame *frame);
protected:
    virtual int _open(int stream_index, AVCodecContext *codec_ctx);
    virtual int _close();
    virtual void run() override {};
protected:
    std::shared_ptr<Context> m_ctx = nullptr;
    AVMediaType m_media_type = AVMEDIA_TYPE_UNKNOWN;

    int m_finished = 0;
    int m_pkt_serial = -1;
    int m_packet_pending = 0;
    int m_recoder_pts = -1;

    AVPacket *m_pkt = nullptr;
    PacketQueue *m_queue = nullptr;
    
    int64_t m_start_pts = AV_NOPTS_VALUE;
    AVRational m_start_pts_tb;
    int64_t m_next_pts = AV_NOPTS_VALUE;
    AVRational m_next_pts_tb;
};

#endif