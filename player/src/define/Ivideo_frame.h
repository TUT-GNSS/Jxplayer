//
// Created by 刘继玺 on 25-2-26.
//

#ifndef IVIDEO_FRAME_H
#define IVIDEO_FRAME_H

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "base_define.h"
#include "../../include/Igl_context.h"

namespace jxplay {
struct IVideoFrame {
    int flags = 0;                   // 标志位
        unsigned int width = 0;      // 视频帧的宽度
        unsigned int height = 0;     // 视频帧的高度
        int64_t pts = 0;                 // 显示时间戳
        int64_t duration = 0;            // 持续时间
        int32_t timebase_num = 1;         // 时间基数的分子
        int32_t timebase_den = 1;         // 时间基数的分母
        std::shared_ptr<uint8_t> data;  // RGBA 数据，为了简化代码，统一使用 RGBA 数据
        unsigned int texture_id = 0;      // OpenGL 纹理 ID

        std::weak_ptr<std::function<void()>> release_callback;  // 释放的回调函数

        float GetTimeStamp() const { return pts * 1.0f * timebase_num / timebase_den; }  // 获取时间戳
        virtual ~IVideoFrame() {
            if (texture_id > 0) glDeleteTextures(1, &texture_id);
            if (auto locked_ptr = release_callback.lock()) {
                (*locked_ptr)();
            }
        }
    };
}
#endif //IVIDEO_FRAME_H
