#include <iostream>
#include "fsrcnn_sr.h"
#include "sr_model_factory.h"

// 如果RESOURCE_DIR没有定义，提供一个默认值
#ifndef RESOURCE_DIR
#define RESOURCE_DIR "./resource"
#endif

namespace jxplayer {
namespace sr {

// 初始化注册的模型列表
std::vector<std::string> SRModelFactory::registered_models_ = {"fsrcnn"};

std::unique_ptr<BaseSR> SRModelFactory::CreateModel(const std::string& model_name, bool use_gpu) {
  // 资源目录路径
  std::string resource_dir = RESOURCE_DIR;
  std::string model_dir = resource_dir + "/model/";

  if (model_name == "fsrcnn") {
    return std::make_unique<FSRCNN_SR>(model_dir + "fsrcnn_x2.onnx", use_gpu);
  }

  // 如果模型名称不匹配，返回nullptr
  std::cerr << "模型名称不存在: " << model_name << std::endl;
  return nullptr;
}

std::vector<std::string> SRModelFactory::GetAvailableModels() {
  return registered_models_;
}

std::shared_ptr<jxplay::IVideoFrame> SRModelFactory::ApplySuperResolution(
    const std::string& model_name,
    const std::shared_ptr<jxplay::IVideoFrame>& frame,
    bool use_gpu) {
  // 如果输入帧为空，直接返回
  if (!frame) {
    return frame;
  }

  // 创建模型实例
  auto model = CreateModel(model_name, use_gpu);
  if (!model) {
    std::cerr << "创建超分辨率模型失败: " << model_name << std::endl;
    return frame;  // 返回原始帧
  }

  // 执行超分辨率处理
  auto result = model->SuperResolve(frame);

  // 如果处理失败，返回原始帧
  if (!result) {
    std::cerr << "超分辨率处理失败" << std::endl;
    return frame;
  }

  return result;
}

}  // namespace sr
}  // namespace jxplayer