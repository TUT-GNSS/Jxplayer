#pragma once

#include <memory>
#include <utility>
#include "../interface/Ivideo_display_view.h"

namespace jxplayer {
namespace sr {

/**
 * @brief 超分辨率基类，定义通用接口
 */
class BaseSR {
 public:
  BaseSR() = default;
  virtual ~BaseSR() = default;

  /**
   * @brief 对视频帧进行超分辨率处理
   * @param frame 输入视频帧
   * @return 超分辨率处理后的视频帧
   */
  virtual std::shared_ptr<jxplay::IVideoFrame> SuperResolve(const std::shared_ptr<jxplay::IVideoFrame>& frame) = 0;

  /**
   * @brief 获取模型支持的输入尺寸
   * @return 输入尺寸 (width, height)
   */
  virtual std::pair<int, int> GetModelInputSize() const = 0;

  /**
   * @brief 获取模型输出尺寸
   * @return 输出尺寸 (width, height)
   */
  virtual std::pair<int, int> GetModelOutputSize() const = 0;

  /**
   * @brief 获取模型支持的放大倍数
   * @return 放大倍数
   */
  virtual float GetScaleFactor() const = 0;
};

}  // namespace sr
}  // namespace jxplayer