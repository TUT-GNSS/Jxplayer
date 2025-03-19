//
// Created by 刘继玺 on 25-3-1.
//

#include "av_synchronizer.h"

#include "../reader/audio_decoder.h"

namespace jxplay {

AVSynchronizer::AVSynchronizer(GLContext& gl_context) : gl_context_(gl_context) {
    Start();
}

AVSynchronizer::~AVSynchronizer() {
    Stop();
}

void AVSynchronizer::SetListener(Listener *listener) {
    std::lock_guard<std::recursive_mutex> listener_lock(listener_mutex_);
    listener_ = listener;
}

void AVSynchronizer::Start() {
    abort_ = false;
    audio_stream_info_.Reset();
    video_stream_info_.Reset();
    sync_thread_ = std::thread(&AVSynchronizer::ThreadLoop, this);
}

void AVSynchronizer::Stop() {
    abort_ = true;
    notifier_.Notify();
    if (sync_thread_.joinable()) {sync_thread_.join();}
}

void AVSynchronizer::Reset() {
    reset_ = true;
    notifier_.Notify();
}

void AVSynchronizer::NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) {
    if (!audio_samples) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (audio_samples->flags & static_cast<int>(AVFrameFlag::kFlush)) {
            CleanAudioQueue();
        }
        audio_queue_.push(audio_samples);
    }
    notifier_.Notify();
}

void AVSynchronizer::NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
    if (!video_frame) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (video_frame->flags & static_cast<int>(AVFrameFlag::kFlush)) {
            CleanVideoQueue();
        }
        video_queue_.push(video_frame);
    }
    notifier_.Notify();
}

void AVSynchronizer::NotifyAudioFinished() {
    auto audio_samples = std::make_shared<IAudioSamples>();
    audio_samples->flags |= static_cast<int>(AVFrameFlag::kEOS);
    NotifyAudioSamples(audio_samples);
}

void AVSynchronizer::NotifyVideoFinished() {
    auto video_samples = std::make_shared<IVideoFrame>();
    video_samples->flags |= static_cast<int>(AVFrameFlag::kEOS);
    NotifyVideoFrame(video_samples);
}

void AVSynchronizer::ThreadLoop() {
    gl_context_.Initialize();
    gl_context_.MakeCurrent();

    while (true) {
        notifier_.Wait(100);
        if (abort_) {break;}
        if (reset_) {
            CleanAudioQueue();
            CleanVideoQueue();
            reset_ = false;
        }
        Synchronize();
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        CleanVideoQueue();
    }
    gl_context_.DoneCurrent();
    gl_context_.Destroy();
}

void AVSynchronizer::Synchronize() {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // 处理音频帧
    while (!audio_queue_.empty()) {
        auto audio_samples = audio_queue_.front();
        if (audio_samples->flags & static_cast<int>(AVFrameFlag::kEOS)) {
            audio_stream_info_.is_finished = true;
            audio_queue_.pop();
            std::lock_guard<std::recursive_mutex> listen_lock(listener_mutex_);
            if (listener_) {
                listener_->OnAVSynchronizerNotifyAudioFinished();
            }
        }else if (audio_samples->flags & static_cast<int>(AVFrameFlag::kFlush)) {
            CleanAudioQueue();
            CleanVideoQueue();
        }else {
            audio_stream_info_.current_time_stamp = audio_samples->GetTimeStamp();
            audio_queue_.pop();
            std::lock_guard<std::recursive_mutex> listen_lock(listener_mutex_);
            if (listener_) {
                listener_->OnAVSynchronizerNotifyAudioSamples(audio_samples);
            }
        }
    }
    // 处理视频帧
    while (!video_queue_.empty()) {
        auto video_samples = video_queue_.front();
        if (video_samples->flags & static_cast<int>(AVFrameFlag::kEOS)) {
            video_stream_info_.is_finished = true;
            video_queue_.pop();
            std::lock_guard<std::recursive_mutex> listener_lock(listener_mutex_);
            if (listener_) {
                listener_->OnAVSynchronizerNotifyVideoFinished();
            }
            continue;
        }else if (video_samples->flags & static_cast<int>(AVFrameFlag::kFlush)) {
            CleanVideoQueue();
            CleanAudioQueue();
            continue;
        }else {
            // 比较音频和视频时间戳 同步音视频
            auto time_diff = audio_stream_info_.current_time_stamp - video_samples->GetTimeStamp();
            // 视频落后太多，丢弃这一帧(或者快速播放)
            if (time_diff > sync_threshold) {
                video_stream_info_.current_time_stamp = video_samples->GetTimeStamp();
                video_queue_.pop();
                std::lock_guard<std::recursive_mutex> listener_lock(listener_mutex_);
                if (listener_) {
                    listener_->OnAVSynchronizerNotifyVideoFrame(video_samples);
                }
                continue;
            }
            else if (time_diff < -sync_threshold) {
                // 视频帧太快 等待下一帧
                break;
            }else {
                // 视频和音频在容忍范围内同步
                video_stream_info_.current_time_stamp = video_samples->GetTimeStamp();
                video_queue_.pop();
                std::lock_guard<std::recursive_mutex> listener_lock(listener_mutex_);
                if (listener_) {
                    listener_->OnAVSynchronizerNotifyVideoFrame(video_samples);
                }
                break;
            }
        }
    }
}

void AVSynchronizer::CleanAudioQueue() {
    std::queue<std::shared_ptr<IAudioSamples>> tem_queue;
    std::swap(audio_queue_, tem_queue);
}

void AVSynchronizer::CleanVideoQueue() {
    std::queue<std::shared_ptr<IVideoFrame>> tem_queue;
    std::swap(video_queue_, tem_queue);
}

}
