//
// Created by 刘继玺 on 25-2-28.
//

#include "video_decoder.h"

#include <functional>
#include <iostream>

extern "C" {
#include <libavutil/imgutils.h>  // Include this header for av_image_get_buffer_size and av_image_fill_arrays
}

namespace jxplay {

IVideoDecoder *IVideoDecoder::Create() {
    return new VideoDecoder();
}


VideoDecoder::VideoDecoder() {
    pipeline_release_callback_ = std::make_shared<std::function<void()>>([&](){ReleaseVideoPipelineResource();} );
    thread_ = std::thread(&VideoDecoder::ThreadLoop, this);
}

VideoDecoder::~VideoDecoder() {
    Stop();
    std::lock_guard<std::mutex> lock(codec_context_mutex_);
    CleanupContext();
}

void VideoDecoder::SetListener(Listener* listener) {
    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    listener_ = listener;
}
void VideoDecoder::SetStream(AVStream* stream) {
    if (!stream) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(packets_queue_mutex_);
        CleanPacketsQueue();
    }

    std::lock_guard<std::mutex> lock(codec_context_mutex_);
    CleanupContext();
    //分配解码上下文
    codec_context_ = avcodec_alloc_context3(nullptr);
    if (avcodec_parameters_to_context(codec_context_, stream->codecpar) < 0) {
        std::cerr << "Failed to allocate AVCodecParameters" << std::endl;
        avcodec_free_context(&codec_context_);
        return;
    }

    // 找到对应的解码器
    const AVCodec* av_codec = avcodec_find_decoder(codec_context_->codec_id);
    if (!av_codec) {
        std::cerr << "Failed to find AVCodec" << std::endl;
        avcodec_free_context(&codec_context_);
        return;
    }
    // 打开解码器
    if (avcodec_open2(codec_context_, av_codec, nullptr) < 0) {
        std::cerr << "Failed to open AVCodec" << std::endl;
        avcodec_free_context(&codec_context_);
        return;
    }
    time_base_ = stream->time_base;
}

void VideoDecoder::Decode(std::shared_ptr<IAVPacket> packet) {
    std::lock_guard<std::mutex> lock(packets_queue_mutex_);
    if (packet->flags & static_cast<int>(AVFrameFlag::kFlush)) {
        CleanPacketsQueue();
    }
    packets_queue_.push(packet);
    notifier_.Notify();
}

void VideoDecoder::Start() {
    paused_ = false;
    notifier_.Notify();
}
void VideoDecoder::Pause() {
    paused_ = true;
}
void VideoDecoder::Stop() {
    abort_ = true;
    notifier_.Notify();
    if (thread_.joinable()) {
        thread_.join();
    }
}
int VideoDecoder::GetVideoWidth() {
    std::lock_guard<std::mutex> lock(codec_context_mutex_);
    return codec_context_ ? codec_context_->width : 0;
}
int VideoDecoder::GetVideoHeight() {
    std::lock_guard<std::mutex> lock(codec_context_mutex_);
    return codec_context_ ? codec_context_->height : 0;
}

void VideoDecoder::ThreadLoop() {
    while (true) {
        notifier_.Wait(100);
        if (abort_) {break;}
        CheckFlushPacket();
        if (!paused_ && pipeline_resource_count_> 0) {
            DecodeAVPacket();
        }
    }
    {
        std::lock_guard<std::mutex> lock(packets_queue_mutex_);
        CleanPacketsQueue();
    }
}

void VideoDecoder::CheckFlushPacket() {
    std::lock_guard<std::mutex> lock(packets_queue_mutex_);
    if (packets_queue_.empty()) {
        return;
    }
    auto packet = packets_queue_.front();
    // 检查刷新标志
    if (packet->flags & static_cast<int>(AVFrameFlag::kFlush)) {
        packets_queue_.pop();
        avcodec_flush_buffers(codec_context_); // 清空编解码器缓冲区

        // 创建刷新事件通知
        auto video_samples = std::make_shared<IVideoFrame>();
        video_samples->flags |= static_cast<int>(AVFrameFlag::kFlush);
        std::lock_guard<std::recursive_mutex> listener_lock(listener_mutex_); // 递归锁保护监听器
        if (listener_) {
            listener_->OnNotifyVideoFrame(video_samples); // 通知监听器刷新事件
        }
    }
}

void VideoDecoder::DecodeAVPacket() {
    std::shared_ptr<IAVPacket> packet;
    {
        std::lock_guard<std::mutex> lock(packets_queue_mutex_);
        if (packets_queue_.empty()) {
            return;
        }
        packet = packets_queue_.front();
        packets_queue_.pop();
    }

    std::lock_guard<std::mutex> lock(codec_context_mutex_);
    if (packet->av_packet && avcodec_send_packet(codec_context_, packet->av_packet) < 0) {
        std::cerr << "Error sending video packet for decoding." << std::endl;
        return;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Could not allocate video frame." << std::endl;
        return;
    }

    while (true) {
        int ret = avcodec_receive_frame(codec_context_, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            std::cerr << "Error during decoding." << std::endl;
            av_frame_free(&frame);
            return;
        }

        if (!sws_context_) {
            sws_context_ = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format), frame->width,
                                          frame->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!sws_context_) {
                av_frame_free(&frame);
                return;
            }
        }

        AVFrame* rgbFrame = av_frame_alloc();
        if (!rgbFrame) {
            av_frame_free(&frame);
            return;
        }

        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, frame->width, frame->height, 1);
        std::shared_ptr<uint8_t> buffer(new uint8_t[numBytes], std::default_delete<uint8_t[]>());
        if (av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer.get(), AV_PIX_FMT_RGBA, frame->width,
                                 frame->height, 1) < 0) {
            av_frame_free(&frame);
            av_frame_free(&rgbFrame);
            return;
        }

        sws_scale(sws_context_, frame->data, frame->linesize, 0, frame->height, rgbFrame->data, rgbFrame->linesize);

        auto videoFrame = std::make_shared<IVideoFrame>();
        videoFrame->width = frame->width;
        videoFrame->height = frame->height;
        videoFrame->data = std::move(buffer);
        videoFrame->pts = frame->pts / (int)playback_speed_ ;
        videoFrame->duration = frame->duration / (int)playback_speed_ ;
        videoFrame->timebase_num = time_base_.num;
        videoFrame->timebase_den = time_base_.den;
        videoFrame->release_callback = pipeline_release_callback_;
        av_frame_free(&rgbFrame);

        --pipeline_resource_count_;

        {
            std::lock_guard<std::recursive_mutex> listener_lock(listener_mutex_);
            if (listener_) {
                listener_->OnNotifyVideoFrame(videoFrame);
            }
        }
    }

    av_frame_free(&frame);
}

void VideoDecoder::ReleaseVideoPipelineResource() {
    ++pipeline_resource_count_;
    notifier_.Notify();
}

void VideoDecoder::CleanupContext() {
    if (codec_context_) avcodec_free_context(&codec_context_);
    if (sws_context_) sws_freeContext(sws_context_);
}

void VideoDecoder::CleanPacketsQueue() {
    std::queue<std::shared_ptr<IAVPacket>> tem_queue;
    std::swap(packets_queue_, tem_queue);
}

}