//
// Created by 刘继玺 on 25-2-26.
//

#include "demuxer.h"

#include <iostream>

namespace jxplay {

IDeMuxer* IDeMuxer::Create() { return new DeMuxer(); }

DeMuxer::DeMuxer() {
    audio_stream_info_.pipeline_release_callback = std::make_shared<std::function<void()>>([&](){ReleaseAudioPipelineResource();});
    video_stream_info_.pipeline_release_callback = std::make_shared<std::function<void()>>([&](){ReleaseVideoPipelineResource();});
    thread_ = std::thread(&DeMuxer::ThreadPool,this);
}

DeMuxer::~DeMuxer() {
    Stop();
    if (thread_.joinable()) {
        thread_.join();
    }
    std::lock_guard<std::mutex> lock(format_mutex_);
    if (format_context_) {
        avformat_close_input(&format_context_);
    }
}

void DeMuxer::SetListener(Listener *listener) {
    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    listener_ = listener;
}

bool DeMuxer::Open(const std::string &url) {
    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    if (avformat_open_input(&format_context_, url.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Could not open url " << url << std::endl;
        return false;
    }
    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
       std::cerr << "Could not find stream information" << std::endl;
        return false;
    }
    for (int i = 0; i < format_context_->nb_streams; ++i) {
        if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_info_.stream_index = i;
            std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
            if (listener_) {
                listener_->OnNotifyAudioStream(format_context_->streams[i]);
            }
        }else if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_info_.stream_index = i;
            std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
            if (listener_) {
                listener_->OnNotifyVideoStream(format_context_->streams[i]);
            }
        }
    }
    return true;
}

void DeMuxer::Start() {
    pause_ = false;
    notifier_.Notify();
}

void DeMuxer::Pause() {
    pause_ = true;
}


void DeMuxer::Stop() {
    abort_ = true;
    notifier_.Notify();
}

void DeMuxer::SeekTo(float position) {
    seek_progress_ = position;
    seek_ = true;
    notifier_.Notify();
}

float DeMuxer::GetDuration() {
    std::lock_guard<std::mutex> lock(format_mutex_);
    if (!format_context_) {
        return 0;
    }
    return format_context_->duration / static_cast<float>(AV_TIME_BASE);
}

void DeMuxer::ThreadPool() {
    while (true) {
        notifier_.Wait(100);
        if (abort_) {break;}
        if (seek_) {
            ProcessSeek();
        }
        if (!pause_ && (audio_stream_info_.pipeline_resource_count>0||video_stream_info_.pipeline_resource_count>0)) {
            if (!ReadAndSendPacket()) {
                break;
            }
        }
    }
}

void DeMuxer::ProcessSeek() {
    std::lock_guard<std::mutex> lock(format_mutex_);
    if (!format_context_) {
        return;
    }
    int64_t time_stamp = static_cast<int64_t> (seek_progress_ * format_context_->duration);
    if (av_seek_frame(format_context_, -1, time_stamp, AVSEEK_FLAG_BACKWARD) < 0) {
        std::cerr << "Could not seek to seek end" << std::endl;
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
        auto audio_packet = std::make_shared<IAVPacket>(nullptr);
        audio_packet->flags |= static_cast<int>(AVFrameFlag::kFlush);
        listener_->OnNotifyAudioPacket(audio_packet);

        auto video_packet = std::make_shared<IAVPacket>(nullptr);
        video_packet->flags |= static_cast<int>(AVFrameFlag::kFlush);
        listener_->OnNotifyVideoPacket(video_packet);
    }
    seek_progress_  = -1.0f;
    seek_ = false;
}

bool DeMuxer::ReadAndSendPacket() {
    std::lock_guard<std::mutex> lock(format_mutex_);
    if (!format_context_) {
        pause_ = true;
        return true;
    }
    AVPacket packet;
    auto ret = av_read_frame(format_context_, &packet);
    if (ret>=0) {
        std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
        if (listener_) {
            if (packet.stream_index == audio_stream_info_.stream_index) {
                --audio_stream_info_.pipeline_resource_count;
                auto shared_audio_packet = std::make_shared<IAVPacket>(av_packet_clone(&packet));
                shared_audio_packet->release_callback = audio_stream_info_.pipeline_release_callback;
                listener_->OnNotifyAudioPacket(shared_audio_packet);
            }else if (packet.stream_index == video_stream_info_.stream_index) {
                --video_stream_info_.pipeline_resource_count;
                auto shared_video_packet = std::make_shared<IAVPacket>(av_packet_clone(&packet));
                shared_video_packet->release_callback = video_stream_info_.pipeline_release_callback;
                listener_->OnNotifyVideoPacket(shared_video_packet);
            }
        }
        av_packet_unref(&packet);
    }else if (ret == AVERROR_EOF) {
        pause_ = true;
    }else {
        return false;
    }
    return true;
}

void DeMuxer::ReleaseAudioPipelineResource() {
    ++audio_stream_info_.pipeline_resource_count;
    notifier_.Notify();
}

void DeMuxer::ReleaseVideoPipelineResource() {
    ++video_stream_info_.pipeline_resource_count;
    notifier_.Notify();
}



} // jxplay