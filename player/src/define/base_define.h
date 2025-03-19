//
// Created by 刘继玺 on 25-2-26.
//

#ifndef BASE_DEFINE_H
#define BASE_DEFINE_H

namespace jxplay {
enum class AVFrameFlag {
    kKeyFrame = 1 << 0,  // 关键帧
    kFlush = 1 << 1,     // 刷新
    kEOS = 1 << 2,       // 结束
};
}


#endif //BASE_DEFINE_H
