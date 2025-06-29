//
// Created by 刘继玺 on 25-3-2.
//

#ifndef VIDEO_PIPELINE_H
#define VIDEO_PIPELINE_H

#include <condition_variable>
#include <queue>
#include <thread>

#include "../../include/Ivideo_filter.h"
#include "../core/sync_notifier.h"
#include "../interface/Ivideo_pipeline.h"
#include "player/src/filter/video_filter.h"
#include "player/src/sr/sr_model_factory.h"

namespace jxplay {
class VideoPipeline : public IVideoPipeline {
 public:
  explicit VideoPipeline(GLContext& gl_context);
  ~VideoPipeline() override;

  std::shared_ptr<IVideoFilter> AddVideoFilter(VideoFilterType type) override;
  void RemoveVideoFilter(VideoFilterType type) override;

  void SetListener(Listener* listener) override;
  void NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) override;
  void NotifyVideoFinished() override;
  void Stop() override;

  // 超分辨率相关方法
  /**
   * @brief 启用/禁用超分辨率
   * @param enabled 是否启用
   */
  void EnableSuperResolution(bool enabled);

  /**
   * @brief 设置超分辨率模型
   * @param model_name 模型名称
   */
  void SetSuperResolutionModel(const std::string& model_name);

  /**
   * @brief 获取所有可用的超分辨率模型
   * @return 模型名称列表
   */
  std::vector<std::string> GetAvailableSuperResolutionModels() const;

 private:
  void PrepareVideoFrame(std::shared_ptr<IVideoFrame> video_frame);
  void PrepareTempTexture(int width, int height);
  void RenderVideoFilter(std::shared_ptr<IVideoFrame> videoFrame);

  // 超分辨率处理
  std::shared_ptr<IVideoFrame> ApplySuperResolution(const std::shared_ptr<IVideoFrame>& frame);

  void ThreadLoop();
  void CleanFrameQueue();

 private:
  struct TextureInfo {
    unsigned int id = 0;
    int width = 0;
    int height = 0;
  };

  GLContext shared_gl_context_;

  std::recursive_mutex listener_mutex_;
  IVideoPipeline::Listener* listener_ = nullptr;

  std::mutex queue_mutex_;
  std::queue<std::shared_ptr<IVideoFrame>> frame_queue_;

  std::shared_ptr<VideoFilter> flip_vertical_filter_;

  std::list<std::shared_ptr<VideoFilter>> video_filter_list_;
  std::list<std::shared_ptr<VideoFilter>> removed_video_filter_list_;
  std::mutex video_filter_mutex_;
  TextureInfo temp_texture_info_;

  std::condition_variable queue_condition_var_;
  // SyncNotifier notifier_;
  std::shared_ptr<std::thread> thread_;
  std::atomic<bool> abort_ = false;

  // 超分辨率相关
  bool sr_enabled_ = false;
  std::string sr_model_name_ = "fsrcnn";
  const bool use_gpu_ = false;  // 固定使用CPU执行
  std::mutex sr_mutex_;
};
}  // namespace jxplay

#endif  // VIDEO_PIPELINE_H
