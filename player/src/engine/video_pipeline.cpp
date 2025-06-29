//
// Created by 刘继玺 on 25-3-2.
//

#include "video_pipeline.h"

#include <iostream>

namespace jxplay {
IVideoPipeline* IVideoPipeline::Create(GLContext& gl_context) {
  return new VideoPipeline(gl_context);
}

VideoPipeline::VideoPipeline(GLContext& gl_context)
    : shared_gl_context_(gl_context), thread_(std::make_shared<std::thread>(&VideoPipeline::ThreadLoop, this)) {}

VideoPipeline::~VideoPipeline() {
  Stop();
}
void VideoPipeline::SetListener(Listener* listener) {
  std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
  listener_ = listener;
}

std::shared_ptr<IVideoFilter> VideoPipeline::AddVideoFilter(VideoFilterType type) {
  std::lock_guard<std::mutex> lock(video_filter_mutex_);
  std::cout << "AVideoPipeline::VideoFilter" << std::endl;
  // 如果已经存在相同类型的滤镜，则不再添加
  for (auto& filter : video_filter_list_) {
    if (filter->GetType() == type) {
      return filter;
    }
  }

  // 实例化对应的filter添加到video_filter_list
  auto filter = std::shared_ptr<VideoFilter>(VideoFilter::Create(type));
  if (filter) {
    video_filter_list_.push_back(filter);
  }
  return filter;
}

void VideoPipeline::RemoveVideoFilter(VideoFilterType type) {
  std::lock_guard<std::mutex> lock(video_filter_mutex_);
  std::cout << "AVideoPipeline::RemoveVideoFilter" << std::endl;
  for (auto it = video_filter_list_.begin(); it != video_filter_list_.end(); ++it) {
    if ((*it)->GetType() == type) {
      removed_video_filter_list_.push_back(*it);
      // 防止迭代器失效
      it = video_filter_list_.erase(it);
      break;
    }
  }
}

void VideoPipeline::NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  frame_queue_.push(video_frame);
  queue_condition_var_.notify_all();
}
void VideoPipeline::NotifyVideoFinished() {
  std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
  if (listener_) {
    listener_->OnVideoPipelineNotifyVideoFinished();
  }
}

void VideoPipeline::Stop() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    abort_ = true;
    queue_condition_var_.notify_all();
  }
  if (thread_->joinable()) {
    thread_->join();
  }
}

void VideoPipeline::PrepareTempTexture(int width, int height) {
  if (temp_texture_info_.id == 0) {
    glGenTextures(1, &temp_texture_info_.id);
    glBindTexture(GL_TEXTURE_2D, temp_texture_info_.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    temp_texture_info_.width = width;
    temp_texture_info_.height = height;
  } else if (temp_texture_info_.width != width || temp_texture_info_.height != height) {
    glBindTexture(GL_TEXTURE_2D, temp_texture_info_.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    temp_texture_info_.width = width;
    temp_texture_info_.height = height;
  }
}

void VideoPipeline::PrepareVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
  glGenTextures(1, &video_frame->texture_id);
  glBindTexture(GL_TEXTURE_2D, video_frame->texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, video_frame->width, video_frame->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               video_frame->data.get());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // 垂直翻转画面
  if (!flip_vertical_filter_) {
    flip_vertical_filter_ = std::shared_ptr<VideoFilter>(VideoFilter::Create(VideoFilterType::kFlipVertical));
  }
  flip_vertical_filter_->Render(video_frame, temp_texture_info_.id);

  std::swap(video_frame->texture_id, temp_texture_info_.id);
}

void VideoPipeline::RenderVideoFilter(std::shared_ptr<IVideoFrame> frame) {
  PrepareTempTexture(frame->width, frame->height);

  std::lock_guard<std::mutex> lock(video_filter_mutex_);
  removed_video_filter_list_.clear();
  if (video_filter_list_.empty())
    return;

  auto inputTexture = frame->texture_id;
  auto outputTexture = temp_texture_info_.id;

  auto filterRenderCount = 0;
  for (auto& filter : video_filter_list_) {
    if (filter->Render(frame, outputTexture)) {
      ++filterRenderCount;
      std::swap(inputTexture, outputTexture);
    }
  }

  if (filterRenderCount % 2 == 1)
    std::swap(frame->texture_id, temp_texture_info_.id);
}

void VideoPipeline::EnableSuperResolution(bool enabled) {
  std::lock_guard<std::mutex> lock(sr_mutex_);
  sr_enabled_ = enabled;
  std::cout << "超分辨率功能已" << (enabled ? "启用" : "禁用") << std::endl;
}

void VideoPipeline::SetSuperResolutionModel(const std::string& model_name) {
  std::lock_guard<std::mutex> lock(sr_mutex_);
  sr_model_name_ = model_name;
  std::cout << "设置超分辨率模型: " << model_name << std::endl;
}

std::vector<std::string> VideoPipeline::GetAvailableSuperResolutionModels() const {
  return jxplayer::sr::SRModelFactory::GetAvailableModels();
}

std::shared_ptr<IVideoFrame> VideoPipeline::ApplySuperResolution(const std::shared_ptr<IVideoFrame>& frame) {
  if (!frame || !sr_enabled_) {
    return frame;
  }

  std::lock_guard<std::mutex> lock(sr_mutex_);

  // 获取超分辨率模型
  auto sr_model = jxplayer::sr::SRModelFactory::CreateModel(sr_model_name_, use_gpu_);
  if (!sr_model) {
    std::cerr << "获取超分辨率模型失败: " << sr_model_name_ << std::endl;
    return frame;
  }

  // 执行超分辨率处理
  auto sr_frame = sr_model->SuperResolve(frame);
  if (sr_frame) {
    std::cout << "超分辨率处理成功: " << frame->width << "x" << frame->height << " -> " << sr_frame->width << "x"
              << sr_frame->height << std::endl;
    return sr_frame;
  } else {
    std::cerr << "超分辨率处理失败" << std::endl;
    return frame;
  }
}

void VideoPipeline::ThreadLoop() {
  shared_gl_context_.Initialize();
  shared_gl_context_.MakeCurrent();

  unsigned int fbo = 0;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  while (true) {
    std::shared_ptr<IVideoFrame> frame;
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      queue_condition_var_.wait(lock, [this]() { return abort_ || !frame_queue_.empty(); });
      if (abort_) {
        break;
      }
      frame = frame_queue_.front();
      frame_queue_.pop();
    }
    if (frame) {
      // 应用超分辨率处理
      if (sr_enabled_) {
        frame = ApplySuperResolution(frame);
      }

      PrepareVideoFrame(frame);
      RenderVideoFilter(frame);
      glFinish();

      std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
      if (listener_) {
        listener_->OnVideoPipelineNotifyVideoFrame(frame);
      }
    }
  }

  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    CleanFrameQueue();
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
  shared_gl_context_.DoneCurrent();
  shared_gl_context_.Destroy();
}

void VideoPipeline::CleanFrameQueue() {
  std::queue<std::shared_ptr<IVideoFrame>> temp_queue;
  std::swap(frame_queue_, temp_queue);
}

}  // namespace jxplay
