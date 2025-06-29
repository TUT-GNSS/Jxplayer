#include <algorithm>
#include <cmath>
#include <iostream>
#include "fsrcnn_sr.h"
extern "C" {
#include <libswscale/swscale.h>
}

namespace jxplayer {
namespace sr {

FSRCNN_SR::FSRCNN_SR(const std::string& model_path, bool use_gpu)
    : model_path_(model_path), use_gpu_(use_gpu), scale_factor_(2.0f) {
  // 初始化ONNX推理器
  onnx_infer_ = std::make_unique<OnnxInfer>();
  if (!onnx_infer_->Initialize(model_path, use_gpu)) {
    // std::cerr << "Failed to initialize ONNX inference engine for FSRCNN" << std::endl;
    return;
  }

  // 打印模型输入输出形状信息
  // auto input_size = onnx_infer_->GetInputSize();
  // auto output_size = onnx_infer_->GetOutputSize();
  // std::cout << "FSRCNN model input size: " << input_size.first << "x" << input_size.second << std::endl;
  // std::cout << "FSRCNN model output size: " << output_size.first << "x" << output_size.second << std::endl;

  // 动态尺寸模型，使用固定的缩放因子
  // std::cout << "动态模型注意！使用固定缩放因子: " << scale_factor_ << "x" << std::endl;
}

FSRCNN_SR::~FSRCNN_SR() {
  if (ctx_rgba2yuv_)
    sws_freeContext(ctx_rgba2yuv_);
  if (ctx_u_)
    sws_freeContext(ctx_u_);
  if (ctx_v_)
    sws_freeContext(ctx_v_);
  if (ctx_yuv2rgba_)
    sws_freeContext(ctx_yuv2rgba_);
}

std::shared_ptr<jxplay::IVideoFrame> FSRCNN_SR::SuperResolve(const std::shared_ptr<jxplay::IVideoFrame>& frame) {
  if (!frame || !onnx_infer_ || !onnx_infer_->IsModelLoaded()) {
    return nullptr;
  }
  int in_w = frame->width;
  int in_h = frame->height;

  // 1. RGBA -> YUV420P
  AVPixelFormat src_fmt = AV_PIX_FMT_RGBA;
  AVPixelFormat yuv_fmt = AV_PIX_FMT_YUV420P;
  // 分配YUV缓冲区
  int y_size = in_w * in_h;
  int uv_w = (in_w + 1) / 2;
  int uv_h = (in_h + 1) / 2;
  int uv_size = uv_w * uv_h;
  std::vector<uint8_t> ybuf(y_size);
  std::vector<uint8_t> ubuf(uv_size);
  std::vector<uint8_t> vbuf(uv_size);
  uint8_t* yuv_data[3] = {ybuf.data(), ubuf.data(), vbuf.data()};
  int yuv_linesize[3] = {in_w, uv_w, uv_w};

  // 缓存SwsContext，只有分辨率变化时才重建
  if (!ctx_rgba2yuv_ || in_w != last_in_w_ || in_h != last_in_h_) {
    if (ctx_rgba2yuv_)
      sws_freeContext(ctx_rgba2yuv_);
    ctx_rgba2yuv_ =
        sws_getContext(in_w, in_h, src_fmt, in_w, in_h, yuv_fmt, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    last_in_w_ = in_w;
    last_in_h_ = in_h;
  }
  const uint8_t* src_slices[1] = {frame->data.get()};
  int src_linesize[1] = {in_w * 4};
  sws_scale(ctx_rgba2yuv_, src_slices, src_linesize, 0, in_h, yuv_data, yuv_linesize);

  // 2. Y通道超分
  std::vector<float> input_tensor(in_w * in_h);
  for (int i = 0; i < in_w * in_h; ++i)
    input_tensor[i] = ybuf[i] / 255.0f;
  std::vector<int64_t> input_shape = {1, 1, in_h, in_w};
  std::vector<float> output_tensor;
  std::vector<int64_t> output_shape;
  if (!onnx_infer_->Inference(input_tensor, input_shape, output_tensor, output_shape)) {
    // std::cerr << "FSRCNN inference failed" << std::endl;
    return nullptr;
  }
  if (output_shape.size() != 4) {
    // std::cerr << "输出形状维度不正确" << std::endl;
    return nullptr;
  }
  int out_h = static_cast<int>(output_shape[2]);
  int out_w = static_cast<int>(output_shape[3]);

  // 3. UV通道缩放到目标尺寸
  int out_uv_w = (out_w + 1) / 2;
  int out_uv_h = (out_h + 1) / 2;
  std::vector<uint8_t> out_ubuf(out_uv_w * out_uv_h);
  std::vector<uint8_t> out_vbuf(out_uv_w * out_uv_h);
  uint8_t* in_uv[1] = {ubuf.data()};
  int in_uv_linesize[1] = {uv_w};
  uint8_t* out_uv[1] = {out_ubuf.data()};
  int out_uv_linesize[1] = {out_uv_w};
  if (!ctx_u_ || uv_w != last_in_w_ / 2 || uv_h != last_in_h_ / 2 || out_uv_w != last_out_w_ / 2 ||
      out_uv_h != last_out_h_ / 2) {
    if (ctx_u_)
      sws_freeContext(ctx_u_);
    ctx_u_ = sws_getContext(uv_w, uv_h, AV_PIX_FMT_GRAY8, out_uv_w, out_uv_h, AV_PIX_FMT_GRAY8, SWS_FAST_BILINEAR,
                            nullptr, nullptr, nullptr);
  }
  if (!ctx_v_ || uv_w != last_in_w_ / 2 || uv_h != last_in_h_ / 2 || out_uv_w != last_out_w_ / 2 ||
      out_uv_h != last_out_h_ / 2) {
    if (ctx_v_)
      sws_freeContext(ctx_v_);
    ctx_v_ = sws_getContext(uv_w, uv_h, AV_PIX_FMT_GRAY8, out_uv_w, out_uv_h, AV_PIX_FMT_GRAY8, SWS_FAST_BILINEAR,
                            nullptr, nullptr, nullptr);
  }
  sws_scale(ctx_u_, in_uv, in_uv_linesize, 0, uv_h, out_uv, out_uv_linesize);
  in_uv[0] = vbuf.data();
  out_uv[0] = out_vbuf.data();
  sws_scale(ctx_v_, in_uv, in_uv_linesize, 0, uv_h, out_uv, out_uv_linesize);

  // 4. 合成YUV420P
  std::vector<uint8_t> out_ybuf(out_w * out_h);
  for (int i = 0; i < out_w * out_h; ++i) {
    float v = output_tensor[i];
    v = std::max(0.0f, std::min(1.0f, v));
    out_ybuf[i] = static_cast<uint8_t>(v * 255.0f);
  }
  uint8_t* out_yuv_data[3] = {out_ybuf.data(), out_ubuf.data(), out_vbuf.data()};
  int out_yuv_linesize[3] = {out_w, out_uv_w, out_uv_w};

  // 5. YUV420P -> RGBA
  if (!ctx_yuv2rgba_ || out_w != last_out_w_ || out_h != last_out_h_) {
    if (ctx_yuv2rgba_)
      sws_freeContext(ctx_yuv2rgba_);
    ctx_yuv2rgba_ =
        sws_getContext(out_w, out_h, yuv_fmt, out_w, out_h, src_fmt, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    last_out_w_ = out_w;
    last_out_h_ = out_h;
  }
  std::shared_ptr<jxplay::IVideoFrame> output_frame = std::make_shared<jxplay::IVideoFrame>();
  output_frame->width = out_w;
  output_frame->height = out_h;
  size_t buffer_size = out_w * out_h * 4;
  output_frame->data = std::shared_ptr<uint8_t>(new uint8_t[buffer_size], std::default_delete<uint8_t[]>());
  uint8_t* out_rgba[1] = {output_frame->data.get()};
  int out_rgba_linesize[1] = {out_w * 4};
  sws_scale(ctx_yuv2rgba_, out_yuv_data, out_yuv_linesize, 0, out_h, out_rgba, out_rgba_linesize);

  // 时间戳等元数据
  output_frame->pts = frame->pts;
  output_frame->duration = frame->duration;
  output_frame->timebase_num = frame->timebase_num;
  output_frame->timebase_den = frame->timebase_den;

  // std::cout << "超分辨率处理成功: " << in_w << "x" << in_h << " -> " << out_w << "x" << out_h << std::endl;
  return output_frame;
}

std::shared_ptr<jxplay::IVideoFrame>
FSRCNN_SR::CreateOutputFrame(const std::shared_ptr<jxplay::IVideoFrame>& input_frame, int width, int height) {
  auto frame = std::make_shared<jxplay::IVideoFrame>();
  frame->width = width;
  frame->height = height;

  // 分配RGBA数据内存
  size_t buffer_size = width * height * 4;  // RGBA每像素4字节
  frame->data = std::shared_ptr<uint8_t>(new uint8_t[buffer_size], std::default_delete<uint8_t[]>());

  // 初始化为0
  std::memset(frame->data.get(), 0, buffer_size);

  return frame;
}

bool FSRCNN_SR::PreprocessFrame(const std::shared_ptr<jxplay::IVideoFrame>& frame, std::vector<float>& input_tensor) {
  if (!frame || !frame->data || frame->width <= 0 || frame->height <= 0) {
    return false;
  }

  // 根据模型输入维度调整tensor大小
  // 模型输入维度是 [1, 1, -1, -1]，即1通道灰度图像
  input_tensor.resize(frame->height * frame->width);

  // 假设输入为RGBA格式，转换为灰度图像
  const uint8_t* src_data = frame->data.get();
  int src_stride = frame->width * 4;  // RGBA每像素4字节

  // 转换为灰度图像并归一化到[0,1]区间
  for (int h = 0; h < frame->height; ++h) {
    for (int w = 0; w < frame->width; ++w) {
      const uint8_t* pixel = src_data + h * src_stride + w * 4;

      // 使用标准的RGB到灰度转换公式
      float gray_value = (0.299f * pixel[0] + 0.587f * pixel[1] + 0.114f * pixel[2]) / 255.0f;

      // 存储到对应位置
      input_tensor[h * frame->width + w] = gray_value;
    }
  }

  return true;
}

bool FSRCNN_SR::PostprocessFrame(const std::vector<float>& output_tensor,
                                 const std::vector<int64_t>& output_shape,
                                 std::shared_ptr<jxplay::IVideoFrame>& output_frame) {
  if (!output_frame || output_tensor.empty() || output_shape.size() != 4) {
    return false;
  }

  // 获取输出尺寸
  int64_t height = output_shape[2];
  int64_t width = output_shape[3];

  // 从灰度张量转换回RGBA图像
  uint8_t* dst_data = output_frame->data.get();
  int dst_stride = output_frame->width * 4;  // RGBA每像素4字节

  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      uint8_t* pixel = dst_data + h * dst_stride + w * 4;

      // 获取灰度值并转回0-255范围
      float gray_value = output_tensor[h * width + w];
      // 确保值在有效范围内
      gray_value = std::max(0.0f, std::min(1.0f, gray_value));
      uint8_t gray_byte = static_cast<uint8_t>(gray_value * 255.0f);

      // 将灰度值复制到RGB三个通道
      pixel[0] = gray_byte;  // R
      pixel[1] = gray_byte;  // G
      pixel[2] = gray_byte;  // B
      pixel[3] = 255;        // A
    }
  }

  return true;
}

std::pair<int, int> FSRCNN_SR::GetModelInputSize() const {
  // 动态尺寸模型，返回0,0表示动态尺寸
  return {0, 0};
}

std::pair<int, int> FSRCNN_SR::GetModelOutputSize() const {
  // 动态尺寸模型，返回0,0表示动态尺寸
  return {0, 0};
}

float FSRCNN_SR::GetScaleFactor() const {
  return scale_factor_;
}

}  // namespace sr
}  // namespace jxplayer