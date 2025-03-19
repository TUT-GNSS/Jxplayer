//
// Created by 刘继玺 on 25-2-28.
//

#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <queue>
#include <mutex>
#include <thread>

#include "../core/sync_notifier.h"
#include "../define/Iav_packet.h"
#include "interface/Ivideo_decoder.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace jxplay {
class VideoDecoder : public IVideoDecoder{
public:
    VideoDecoder();
    ~VideoDecoder() override;

    void SetListener(Listener* listener) override;
    void SetStream(AVStream* stream) override; // 设置音频流并初始化解码器相关资源
    void Decode(std::shared_ptr<IAVPacket> packet) override;
    void Start() override;
    void Pause() override;
    void Stop() override;

    int GetVideoWidth() override;
    int GetVideoHeight() override;

private:
    void ThreadLoop();
    void CheckFlushPacket();
    void DecodeAVPacket();
    void ReleaseVideoPipelineResource();
    void CleanupContext();
    void CleanPacketsQueue();
private:
    // 数据回调
    IVideoDecoder::Listener* listener_;
    std::recursive_mutex listener_mutex_;

    // 解码 缩放
    std::mutex codec_context_mutex_;
    AVCodecContext* codec_context_ = nullptr; // 编解码器上下文
    SwsContext* sws_context_ = nullptr; // 缩放上下文
    AVRational time_base_ = {1,1};

    // AVPacket队列
    std::mutex packets_queue_mutex_;
    std::queue<std::shared_ptr<IAVPacket>> packets_queue_;

    SyncNotifier notifier_;

    std::thread thread_;
    std::atomic<bool> paused_;
    std::atomic<bool> abort_;

    std::atomic<int> pipeline_resource_count_ = 3;
    std::shared_ptr<std::function<void()>> pipeline_release_callback_;

    float playback_speed_ = 1.0f; // 默认正常速度
};
}




#endif //VIDEO_DECODER_H
