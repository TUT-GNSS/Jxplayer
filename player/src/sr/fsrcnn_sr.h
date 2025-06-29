#pragma once

extern "C" {
#include <libswscale/swscale.h>
}
#include <memory>
#include <string>
#include <vector>
#include "base_sr.h"
#include "onnx_infer.h"

namespace jxplayer {
namespace sr {

/**
 * @brief FSRCNN超分辨率实现类
 *
 * FSRCNN (Fast Super-Resolution Convolutional Neural Network)
 * 是一种轻量级的超分辨率神经网络，适合实时处理
 */
class FSRCNN_SR : public BaseSR {
 public:
  /**
   * @brief 构造函数
   * @param model_path 模型文件路径
   * @param use_gpu 是否使用GPU加速
   */
  explicit FSRCNN_SR(const std::string& model_path, bool use_gpu = false);

  /**
   * @brief 析构函数
   */
  ~FSRCNN_SR() override;

  /**
   * @brief 对视频帧进行超分辨率处理
   * @param frame 输入视频帧
   * @return 超分辨率处理后的视频帧
   */
  std::shared_ptr<jxplay::IVideoFrame> SuperResolve(const std::shared_ptr<jxplay::IVideoFrame>& frame) override;

  /**
   * @brief 获取模型支持的输入尺寸
   * @return 输入尺寸 (width, height)
   */
  std::pair<int, int> GetModelInputSize() const override;

  /**
   * @brief 获取模型输出尺寸
   * @return 输出尺寸 (width, height)
   */
  std::pair<int, int> GetModelOutputSize() const override;

  /**
   * @brief 获取模型支持的放大倍数
   * @return 放大倍数，FSRCNN通常为2x
   */
  float GetScaleFactor() const override;

 private:
  /**
   * @brief 创建输出帧
   * @param input_frame 输入帧
   * @param width 输出宽度
   * @param height 输出高度
   * @return 创建的输出帧
   */
  std::shared_ptr<jxplay::IVideoFrame> CreateOutputFrame(const std::shared_ptr<jxplay::IVideoFrame>& input_frame,
                                                         int width,
                                                         int height);

  /**
   * @brief 预处理帧数据
   * @param frame 输入视频帧
   * @param input_tensor 输出预处理后的张量数据
   * @return 是否预处理成功
   */
  bool PreprocessFrame(const std::shared_ptr<jxplay::IVideoFrame>& frame, std::vector<float>& input_tensor);

  /**
   * @brief 后处理张量数据
   * @param output_tensor 模型输出张量
   * @param output_shape 输出张量形状
   * @param output_frame 输出视频帧
   * @return 是否后处理成功
   */
  bool PostprocessFrame(const std::vector<float>& output_tensor,
                        const std::vector<int64_t>& output_shape,
                        std::shared_ptr<jxplay::IVideoFrame>& output_frame);

 private:
  // ONNX推理器
  std::unique_ptr<OnnxInfer> onnx_infer_;
  // 模型路径
  std::string model_path_;
  // 是否使用GPU
  bool use_gpu_;
  // 模型缩放因子（通常FSRCNN为2x）
  float scale_factor_;
  // swscale上下文缓存
  SwsContext* ctx_rgba2yuv_ = nullptr;
  SwsContext* ctx_u_ = nullptr;
  SwsContext* ctx_v_ = nullptr;
  SwsContext* ctx_yuv2rgba_ = nullptr;
  int last_in_w_ = 0, last_in_h_ = 0, last_out_w_ = 0, last_out_h_ = 0;
};

}  // namespace sr
}  // namespace jxplayer