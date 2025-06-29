#ifndef JXPLAYER_ONNX_INFER_H
#define JXPLAYER_ONNX_INFER_H

#include <onnxruntime_cxx_api.h>
#include <memory>
#include <string>
#include <vector>

namespace jxplayer {
namespace sr {

/**
 * @brief OnnxInfer类用于封装onnxruntime的推理功能，主要用于视频超分辨率处理
 */
class OnnxInfer {
 public:
  /**
   * @brief 构造函数
   */
  OnnxInfer();

  /**
   * @brief 析构函数
   */
  ~OnnxInfer();

  /**
   * @brief 初始化推理环境和模型
   * @param model_path 模型文件路径
   * @param use_gpu 是否使用GPU加速
   * @return 是否初始化成功
   */
  bool Initialize(const std::string& model_path, bool use_gpu = false);

  /**
   * @brief 执行推理
   * @param input_tensor 输入张量数据
   * @param input_shape 输入张量形状
   * @param output_tensor 输出张量数据
   * @param output_shape 输出张量形状
   * @return 是否推理成功
   */
  bool Inference(const std::vector<float>& input_tensor,
                 const std::vector<int64_t>& input_shape,
                 std::vector<float>& output_tensor,
                 std::vector<int64_t>& output_shape);

  /**
   * @brief 获取当前模型支持的输入尺寸
   * @return 输入尺寸 (width, height)
   */
  std::pair<int, int> GetInputSize() const;

  /**
   * @brief 获取当前模型的输出尺寸
   * @return 输出尺寸 (width, height)
   */
  std::pair<int, int> GetOutputSize() const;

  /**
   * @brief 检查模型是否已加载
   * @return 是否已加载
   */
  bool IsModelLoaded() const;

 private:
  // ONNX Runtime环境
  Ort::Env env_;
  // 会话选项
  Ort::SessionOptions session_options_;
  // 推理会话
  std::unique_ptr<Ort::Session> session_;
  // 内存分配器
  Ort::AllocatorWithDefaultOptions allocator_;
  // 输入节点名
  std::vector<const char*> input_node_names_;
  // 输出节点名
  std::vector<const char*> output_node_names_;
  // 存储字符串，防止内存释放
  std::vector<std::string> input_node_strings_;
  std::vector<std::string> output_node_strings_;
  // 输入尺寸 (可能包含动态维度，值为-1)
  std::vector<int64_t> input_dims_;
  // 输出尺寸 (可能包含动态维度，值为-1)
  std::vector<int64_t> output_dims_;
  // 模型是否已加载
  bool model_loaded_;
};

}  // namespace sr
}  // namespace jxplayer

#endif  // JXPLAYER_ONNX_INFER_H