#pragma once

#include <memory>
#include <string>
#include <vector>
#include "base_sr.h"

namespace jxplayer {
namespace sr {

/**
 * @brief 超分辨率模型工厂类，负责创建不同类型的超分辨率模型
 */
class SRModelFactory {
 public:
  /**
   * @brief 创建超分辨率模型实例
   * @param model_name 模型名称
   * @param use_gpu 是否使用GPU加速
   * @return 超分辨率模型实例
   */
  static std::unique_ptr<BaseSR> CreateModel(const std::string& model_name, bool use_gpu = false);

  /**
   * @brief 获取所有可用模型的名称
   * @return 模型名称列表
   */
  static std::vector<std::string> GetAvailableModels();

  /**
   * @brief 直接使用指定模型对视频帧进行超分辨率处理
   * @param model_name 模型名称
   * @param frame 输入视频帧
   * @param use_gpu 是否使用GPU加速
   * @return 处理后的视频帧，如果处理失败则返回原始帧
   */
  static std::shared_ptr<jxplay::IVideoFrame> ApplySuperResolution(const std::string& model_name,
                                                                   const std::shared_ptr<jxplay::IVideoFrame>& frame,
                                                                   bool use_gpu = false);

 private:
  // 所有可用的模型名称列表
  static std::vector<std::string> registered_models_;
};

}  // namespace sr
}  // namespace jxplayer