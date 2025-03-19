//
// Created by 刘继玺 on 25-3-2.
//

#ifndef VIDEO_PIPELINE_H
#define VIDEO_PIPELINE_H

#include <queue>
#include <thread>
#include <condition_variable>

#include "../interface/Ivideo_pipeline.h"
#include "../core/sync_notifier.h"
#include "../../include/Ivideo_filter.h"
#include "player/src/filter/video_filter.h"

namespace jxplay {
class VideoPipeline : public IVideoPipeline{
public:
    explicit VideoPipeline(GLContext& gl_context);
    ~VideoPipeline() override;

    std::shared_ptr<IVideoFilter> AddVideoFilter(VideoFilterType type) override;
    void RemoveVideoFilter(VideoFilterType type) override;

    void SetListener(Listener* listener) override;
    void NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) override;
    void NotifyVideoFinished() override;
    void Stop() override;

private:
    void PrepareVideoFrame(std::shared_ptr<IVideoFrame> video_frame);
    void PrepareTempTexture(int width, int height);
    void RenderVideoFilter(std::shared_ptr<IVideoFrame> videoFrame);

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

};
}

#endif //VIDEO_PIPELINE_H
