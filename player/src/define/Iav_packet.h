//
// Created by 刘继玺 on 25-2-26.
//
#include <functional>
#include <memory>

#include "base_define.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/rational.h>
}

#ifndef IAV_PACKET_H
#define IAV_PACKET_H

namespace jxplay {
struct IAVPacket {
    int flags = 0; // 标志位
    AVPacket *av_packet = nullptr; // 指向FFmpeg av_packet结构体的指针
    AVRational time_base = {0, 0}; // 时间基
    std::weak_ptr<std::function<void()>> release_callback; // 释放的回调
    
    explicit IAVPacket(AVPacket* av_packet) : av_packet(av_packet) {}
    virtual ~IAVPacket() {
        if (av_packet) av_packet_free(&av_packet);  // 释放av_packet
        if (auto lockedPtr = release_callback.lock()) {
            (*lockedPtr)();  // 调用释放回调函数
        }
    }

    float GetTimeStamp() const {
        return av_packet ? av_packet->pts * 1.0f * time_base.num / time_base.den : -1.0f;  // 获取时间戳
    }
};
}

#endif //IAV_PACKET_H
