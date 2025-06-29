#include <algorithm>
#include <cstring>
#include <iostream>
#include "onnx_infer.h"

namespace jxplayer {
namespace sr {

OnnxInfer::OnnxInfer() : env_(ORT_LOGGING_LEVEL_WARNING, "OnnxInfer"), model_loaded_(false) {}

OnnxInfer::~OnnxInfer() {
  // 清理资源
  input_node_names_.clear();
  output_node_names_.clear();
  input_node_strings_.clear();
  output_node_strings_.clear();
  session_.reset();
}

bool OnnxInfer::Initialize(const std::string& model_path, bool use_gpu) {
  // 配置会话选项
  session_options_.SetIntraOpNumThreads(1);
  session_options_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

  // 仅使用CPU执行推理
  if (use_gpu) {
    std::cerr << "当前仅支持CPU推理" << std::endl;
  }

  // 使用CPU执行提供程序
  session_options_.SetIntraOpNumThreads(4);  // 使用4个线程以提高性能

  // 创建会话
  try {
    session_ = std::make_unique<Ort::Session>(env_, model_path.c_str(), session_options_);
  } catch (...) {
    std::cerr << "创建ONNX会话失败" << std::endl;
    return false;
  }

  if (!session_) {
    std::cerr << "创建ONNX会话失败" << std::endl;
    return false;
  }

  // 获取模型信息
  size_t num_input_nodes = 0;
  size_t num_output_nodes = 0;

  try {
    num_input_nodes = session_->GetInputCount();
    num_output_nodes = session_->GetOutputCount();
  } catch (...) {
    std::cerr << "获取模型信息失败" << std::endl;
    return false;
  }

  // 获取输入输出节点名称
  try {
    // 清空之前的名称
    input_node_names_.clear();
    output_node_names_.clear();
    input_node_strings_.clear();
    output_node_strings_.clear();

    for (size_t i = 0; i < num_input_nodes; i++) {
      Ort::AllocatedStringPtr input_name = session_->GetInputNameAllocated(i, allocator_);
      std::string name_str(input_name.get());
      input_node_strings_.push_back(name_str);
      input_node_names_.push_back(input_node_strings_.back().c_str());
      std::cout << "输入节点名: " << input_node_strings_.back() << std::endl;
    }

    for (size_t i = 0; i < num_output_nodes; i++) {
      Ort::AllocatedStringPtr output_name = session_->GetOutputNameAllocated(i, allocator_);
      std::string name_str(output_name.get());
      output_node_strings_.push_back(name_str);
      output_node_names_.push_back(output_node_strings_.back().c_str());
      std::cout << "输出节点名: " << output_node_strings_.back() << std::endl;
    }

    // 获取输入形状
    Ort::TypeInfo type_info = session_->GetInputTypeInfo(0);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    input_dims_ = tensor_info.GetShape();

    // 获取输出形状
    type_info = session_->GetOutputTypeInfo(0);
    tensor_info = type_info.GetTensorTypeAndShapeInfo();
    output_dims_ = tensor_info.GetShape();

    // 打印动态维度信息
    std::cout << "模型输入维度: [";
    for (auto dim : input_dims_) {
      std::cout << dim << " ";
    }
    std::cout << "]" << std::endl;

    std::cout << "模型输出维度: [";
    for (auto dim : output_dims_) {
      std::cout << dim << " ";
    }
    std::cout << "]" << std::endl;

  } catch (...) {
    std::cerr << "获取模型节点信息失败" << std::endl;
    return false;
  }

  // 检查维度是否有效 - 对于动态维度，可能有-1值
  if (input_dims_.size() != 4 || output_dims_.size() != 4) {
    std::cerr << "模型输入或输出维度不符合要求，应为4维 [batch, channels, height, width]" << std::endl;
    return false;
  }

  model_loaded_ = true;
  return true;
}

bool OnnxInfer::Inference(const std::vector<float>& input_tensor,
                          const std::vector<int64_t>& input_shape,
                          std::vector<float>& output_tensor,
                          std::vector<int64_t>& output_shape) {
  if (!model_loaded_ || input_tensor.empty() || input_shape.empty()) {
    return false;
  }

  try {
    // 创建输入tensor
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value input_tensor_value =
        Ort::Value::CreateTensor<float>(memory_info, const_cast<float*>(input_tensor.data()), input_tensor.size(),
                                        input_shape.data(), input_shape.size());

    // 执行推理
    std::vector<Ort::Value> output_tensors =
        session_->Run(Ort::RunOptions{nullptr}, input_node_names_.data(), &input_tensor_value, 1,
                      output_node_names_.data(), output_node_names_.size());

    // 获取输出数据
    const float* output_data = output_tensors[0].GetTensorData<float>();

    // 获取实际输出形状
    auto output_tensor_info = output_tensors[0].GetTensorTypeAndShapeInfo();
    auto actual_output_shape = output_tensor_info.GetShape();

    // 计算输出大小
    size_t output_size = 1;
    for (auto dim : actual_output_shape) {
      output_size *= dim;
    }

    // 复制输出数据
    output_tensor.assign(output_data, output_data + output_size);
    output_shape = actual_output_shape;

    // 打印实际输出尺寸
    std::cout << "实际输出尺寸: [";
    for (auto dim : actual_output_shape) {
      std::cout << dim << " ";
    }
    std::cout << "]" << std::endl;

    return true;
  } catch (const Ort::Exception& e) {
    std::cerr << "推理错误: " << e.what() << std::endl;
    return false;
  } catch (const std::exception& e) {
    std::cerr << "推理错误: " << e.what() << std::endl;
    return false;
  }
}

std::pair<int, int> OnnxInfer::GetInputSize() const {
  if (!model_loaded_ || input_dims_.size() < 4) {
    return {0, 0};
  }

  // 对于动态尺寸，可能会返回负值
  return {static_cast<int>(input_dims_[3]), static_cast<int>(input_dims_[2])};
}

std::pair<int, int> OnnxInfer::GetOutputSize() const {
  if (!model_loaded_ || output_dims_.size() < 4) {
    return {0, 0};
  }

  // 对于动态尺寸，可能会返回负值
  return {static_cast<int>(output_dims_[3]), static_cast<int>(output_dims_[2])};
}

bool OnnxInfer::IsModelLoaded() const {
  return model_loaded_;
}

}  // namespace sr
}  // namespace jxplayer