//
// Created by 刘继玺 on 25-2-26.
//

#ifndef IAUDIO_SAMPLES_H
#define IAUDIO_SAMPLES_H

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "base_define.h"

namespace jxplay {
struct IAudioSamples {
    int flags = 0; // 标志位
    unsigned int channels;// 通道数
    unsigned int sample_rate; // 采样率

    int64_t pts; // 显示时间戳
    int64_t duration; // 持续时间
    int32_t timebase_num; // 时间基数分子
    int32_t timebase_den; // 时间基数分母

    std::vector<int16_t> pcm_data; // 存储PCM数据的数组
    size_t offset = 0; // 当前PCM数据的偏移量
    std::weak_ptr<std::function<void()>> release_callback; // 释放的回调函数
    float GetTimeStamp() const { return pts * 1.0f * timebase_num / timebase_den; } // 获取时间戳

    virtual ~IAudioSamples() {
        if (auto locked_ptr = release_callback.lock()) {
            (*locked_ptr)();
        }
    }
};
}

#endif //IAUDIO_SAMPLES_H
