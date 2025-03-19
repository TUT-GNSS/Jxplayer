//
// Created by 刘继玺 on 25-2-27.
//

#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <condition_variable>
#include <queue>
#include <mutex>
#include <thread>

#include "../core/sync_notifier.h"
#include "interface/Iaudio_decoder.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
}
#include "../../../third_party/soundtouch/include/SoundTouch.h"

namespace jxplay {
class AudioDecoder : public IAudioDecoder{
public:
    AudioDecoder(unsigned int channels, unsigned int sample_rate);
    ~AudioDecoder() override;

    void SetListener(Listener* listener) override;
    void SetStream(AVStream* stream) override; // 设置音频流并初始化解码器相关资源
    void Decode(std::shared_ptr<IAVPacket> packet) override;
    void Start() override;
    void Pause() override;
    void Stop() override;

private:
    void ThreadLoop();
    void CheckFlushPacket();
    void DecodeAVPacket(); // 解码AVPacket并输出音频样本
    void ReleaseAudioPipelineResource();
    void CleanupContext();
    void CleanPacketsQueue();
private:
    unsigned int traget_channels_;
    unsigned int traget_sample_rate_;

    // 数据回调
    IAudioDecoder::Listener* listener_;
    std::recursive_mutex listener_mutex_;

    // 解码和重采样
    std::mutex codec_context_mutex_;
    AVCodecContext* codec_context_ = nullptr;
    SwrContext* swr_context_ = nullptr;
    AVRational time_base_ = {1,1};

    // AVPacket队列
    std::queue<std::shared_ptr<IAVPacket>> packets_queue_;
    std::mutex packets_queue_mutex_;

    SyncNotifier notifier_;

    std::thread thread_;
    std::atomic<bool> pause_ = true;
    std::atomic<bool> abort_ = false;

    std::atomic<int> pipeline_resource_count_ = 3;
    std::shared_ptr<std::function<void()>> pipeline_release_callback_;

    soundtouch::SoundTouch sound_touch_;
    float playback_speed_ = 1.0f; // 默认正常速度
    std::vector<int16_t> soundtouch_buffer_;  // 输入缓冲队列
};
}




#endif //AUDIO_DECODER_H
