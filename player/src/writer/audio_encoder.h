//
// Created by 刘继玺 on 25-3-4.
//

#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#include <atomic>
#include <condition_variable>
#include <queue>
#include <memory>
#include <mutex>
#include <thread>

#include "interface/Iaudio_encoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace jxplay  {

class AudioEncoder : public IAudioEncoder{
public:
    AudioEncoder();
    ~AudioEncoder();

    // 继承自IAudioEncoder
    void SetListener(Listener *listener) override;
    bool Configure(FileWriterParameters& parameters, int flags);
    void NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) override;

private:
    void ThreadLoop();
    void StopThread();
    void PrepareEncodeAudioSamples(std::shared_ptr<IAudioSamples> audio_samples);
    void EncodeAudioSamples(const AVFrame* audio_frame);
    void CleanPacketsQueue();

private:
    std::mutex listener_mutex_;
    Listener *listener_ = nullptr;

    // AVPacket 队列
    std::mutex audio_samples_queue_mutex_;
    std::condition_variable condition_var_;
    std::queue<std::shared_ptr<IAudioSamples>> audio_samples_queue_;

    std::thread thread_;
    std::atomic<bool> abort_ = false;

    // 编码和重采样
    AVCodecContext* encoder_context_ = nullptr;
    SwrContext* swr_context_ = nullptr;

    // AVPacket 和 AVFrame
    AVFrame* av_frame_ = nullptr;
    AVPacket* av_packet_ = nullptr;

    int64_t pts_ = 0;

};

}


#endif //AUDIO_ENCODER_H
