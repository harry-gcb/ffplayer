#include "packet_queue.h"

int PacketQueue::put(AVPacket *pkt) {

    AVPacket *pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt);
        return -1;
    }
    // 将pkt中的内容拷贝到pkt1，并将pkt重置
    av_packet_move_ref(pkt1, pkt);
    // 将pkt1 put进队列
    int ret = 0;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        ret = _put(pkt1);
    }
    if (ret < 0) {
        av_packet_free(&pkt);
    }
    return ret;
}

int PacketQueue::_put(AVPacket *pkt) {
    if (m_abort_request) {
        return -1;
    }
    Packet packet;
    packet.pkt = pkt;
    packet.serial = m_serial;
    m_size += packet.pkt->size + sizeof(packet);
    m_duration += packet.pkt->duration;
    m_queue.push(packet);
    m_cond.notify_one();
    return 0;
}

int PacketQueue::get(AVPacket *pkt, int block, int &serial) {
    int ret = 0;
    Packet packet;
    std::unique_lock<std::mutex> lock{m_mutex};
    for (;;) {
        if (m_abort_request || (!block && m_queue.empty())) {
            ret = -1;
            break;
        }
        while (m_abort_request || m_queue.empty()) {
            m_cond.wait(lock);
        }
        if (m_abort_request) {
            ret = -1;
            break;
        }
        packet = m_queue.front();
        m_size -= packet.pkt->size + sizeof(packet);
        m_duration -= packet.pkt->duration;
        serial = packet.serial;
        av_packet_move_ref(pkt, packet.pkt);
        av_packet_free(&packet.pkt);
        m_queue.pop();
        ret = 1;
        break;
    }
    return ret;
}

int PacketQueue::count() const {
    return m_queue.size();
}

int PacketQueue::size() const {
    return m_size;
}

void PacketQueue::flush() {
    Packet packet;
    std::unique_lock<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        packet = m_queue.front();
        av_packet_free(&packet.pkt);
        m_queue.pop();
    }
    m_serial++;
    m_size = 0;
    m_duration = 0;
}

void PacketQueue::destroy() {
    flush();
}

int PacketQueue::serial() {
    return m_serial;
}

bool PacketQueue::request_aborted() {
    return m_abort_request;
}