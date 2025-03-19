//
// Created by 刘继玺 on 25-3-3.
//

#ifndef IVIDEO_FILTER_H
#define IVIDEO_FILTER_H

#include <string>

namespace jxplay {

enum class VideoFilterType {
    kNone = 0,
    kFlipVertical,  // 垂直翻转
    kGray,          // 灰度
    kInvert,        // 反色
    kSticker,       // 贴纸
    kPause,         // 暂停
};

class IVideoFilter {
public:
    virtual VideoFilterType GetType() const = 0;

    virtual void SetFloat(const std::string& name, float value) = 0;
    virtual float GetFloat(const std::string& name) = 0;

    virtual void SetInt(const std::string& name, int value) = 0;
    virtual int GetInt(const std::string& name) = 0;

    virtual void SetString(const std::string& name, const std::string& value) = 0;
    virtual std::string GetString(const std::string& name) = 0;

    virtual ~IVideoFilter() = default;
};

}  // namespace jxplay


#endif //IVIDEO_FILTER_H
