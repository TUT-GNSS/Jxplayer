//
// Created by 刘继玺 on 25-3-4.
//

#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>
#include <player/src/filter/video_filter.h>

#include "../../include/Igl_context.h"
#include "interface/Ivideo_encoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace jxplay {

class VideoEncoder : public IVideoEncoder {
public:
    explicit VideoEncoder(GLContext& gl_context);
    ~VideoEncoder();
    void SetListener(Listener *listener) override;
    bool Configure(FileWriterParameters &parameters, int flags) override;
    void NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) override;

private:
    void ThreadLoop();
    void StopThread();
    void FlipVideoFrame(std::shared_ptr<IVideoFrame> video_frame);
    void ConvertTextureToFrame(unsigned int texture_id, int width, int height);
    void PrepareAndEncodeVideoFrame(std::shared_ptr<IVideoFrame> video_frame);
    void EncodeVideoFrame(const AVFrame* frame);
    void CleanPacketsQueue();
private:
    GLContext shared_gl_context_;

    Listener *listener_;
    std::mutex listener_mutex_;

    // AVPacket 队列
    std::mutex video_frame_queue_mutex_;
    std::condition_variable condition_var_;
    std::queue<std::shared_ptr<IVideoFrame>> video_frame_queue_;

    std::thread thread_;
    std::atomic<bool> abort_ = false;

    std::shared_ptr<VideoFilter> flip_vertical_filter_; // 垂直翻转滤镜
    unsigned int texture_id_ = 0;

    AVCodecContext *encode_context_ = nullptr;
    SwsContext *sws_context_ = nullptr;
    AVFrame *av_frame_ = nullptr;
    AVPacket *av_packet_ = nullptr;
    std::vector<uint8_t> rgba_data_;

    int frame_index_ = 0;  // 初始化为0
};

}



#endif //VIDEO_ENCODER_H
