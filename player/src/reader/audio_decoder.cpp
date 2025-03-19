//
// Created by 刘继玺 on 25-2-27.
//

#include "audio_decoder.h"

#include <iostream>

namespace jxplay {
IAudioDecoder* IAudioDecoder::Create(unsigned int channels, unsigned int sample_rate) {
    return new AudioDecoder(channels, sample_rate);
}
AudioDecoder::AudioDecoder(unsigned int channels, unsigned int sample_rate)
        : traget_channels_(channels), traget_sample_rate_(sample_rate), sound_touch_() {
    pipeline_release_callback_ = std::make_shared<std::function<void()>>([&]() { ReleaseAudioPipelineResource(); });
    thread_ = std::thread(&AudioDecoder::ThreadLoop, this);
    // sound_touch_.setSampleRate(traget_sample_rate_);     // 目标采样率
    // sound_touch_.setChannels(traget_channels_);          // 目标声道数
    // sound_touch_.setTempo(playback_speed_);              // 初始播放速度
    // sound_touch_.setRate(playback_speed_);
    // sound_touch_.setSetting(SETTING_USE_QUICKSEEK, 0);   // 关闭快速模式保证音质
}

AudioDecoder::~AudioDecoder() {
    Stop();
    {
        std::lock_guard<std::mutex> lock(codec_context_mutex_);
        CleanupContext();
    }
}

void AudioDecoder::SetListener(IAudioDecoder::Listener* listener) {
    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    listener_ = listener;
}

void AudioDecoder::SetStream(AVStream* stream) {
    if (!stream) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(packets_queue_mutex_);
        CleanPacketsQueue();
    }
    std::lock_guard<std::mutex> lock(codec_context_mutex_);
    CleanupContext();
    // 1.创建编解码上下文
    codec_context_ = avcodec_alloc_context3(nullptr);
    if (avcodec_parameters_to_context(codec_context_, stream->codecpar) < 0) {
        std::cerr << "Failed to allocate AVCodecParameters" << std::endl;
        return;
    }

    // 2.查找解码器
    const AVCodec* av_codec = avcodec_find_decoder(codec_context_->codec_id);
    if (!av_codec) {
        std::cerr << "Failed to find AVCodec" << std::endl;
        return;
    }
    // 3.打开解码器
    if (avcodec_open2(codec_context_, av_codec, nullptr) < 0) {
        std::cerr << "Failed to open AVCodec" << std::endl;
        return;
    }
    // 4.初始化重采样上下文
    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO; // 输出为立体声
    AVChannelLayout in_ch_layout;
    av_channel_layout_copy(&in_ch_layout, &codec_context_->ch_layout);

    int ret = swr_alloc_set_opts2(
        &swr_context_,
        &out_ch_layout,  // 输出声道布局
        AV_SAMPLE_FMT_S16,                                 // 输出采样格式
        traget_sample_rate_,                                // 输出采样率
        &in_ch_layout, // 输入声道布局
        // static_cast<AVSampleFormat>(stream->codecpar->format),  // 输入采样格式
        codec_context_->sample_fmt,                        // 输入采样格式
        codec_context_->sample_rate,                       // 输入采样率
        0,                                                // 日志偏移
        nullptr                                           // 附加参数
    );
    if (ret < 0 || !swr_context_) {
        std::cerr << "Failed to create resampler" << std::endl;
        return;
    }
    if (swr_init(swr_context_) < 0) {
        std::cerr << "Failed to initialize resampler." << std::endl;
        return;
    }

    time_base_ = stream->time_base;
    // 5. 释放声道布局资源
    av_channel_layout_uninit(&in_ch_layout);
    av_channel_layout_uninit(&out_ch_layout);
}

void AudioDecoder::Decode(std::shared_ptr<IAVPacket> packet) {
    if (!packet) {return;}
    {
        std::lock_guard<std::mutex> lock(packets_queue_mutex_);
        if (packet->flags & static_cast<int>(AVFrameFlag::kFlush)) {
            CleanPacketsQueue();
        }
        packets_queue_.push(packet);
        notifier_.Notify();
    }
}

void AudioDecoder::Start() {
    pause_ = false;
    notifier_.Notify();
}

void AudioDecoder::Pause() {
    pause_ = true;
}

void AudioDecoder::Stop() {
    abort_ = true;
    notifier_.Notify();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void AudioDecoder::ThreadLoop() {
    while (true) {
        notifier_.Wait(100);
        if (abort_) {break;}
        CheckFlushPacket();
        if (!pause_ && pipeline_resource_count_ >0) {
            DecodeAVPacket();
        }
    }
    {
        std::lock_guard<std::mutex> lock(packets_queue_mutex_);
        CleanPacketsQueue();
    }
}

void AudioDecoder::CheckFlushPacket() {
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
        auto audio_samples = std::make_shared<IAudioSamples>();
        audio_samples->flags |= static_cast<int>(AVFrameFlag::kFlush);
        std::lock_guard<std::recursive_mutex> listener_lock(listener_mutex_); // 递归锁保护监听器
        if (listener_) {
            listener_->OnNotifyAudioSamples(audio_samples); // 通知监听器刷新事件
        }
    }
}

void AudioDecoder::DecodeAVPacket() {
    std::shared_ptr<IAVPacket> packet;
    {
        std::lock_guard<std::mutex> lock(packets_queue_mutex_);
        if (packets_queue_.empty()) {
            return;
        }
        packet = packets_queue_.front();
        packets_queue_.pop();
    }
    // 发送AVPacket到编解码器
    if (packet->av_packet && avcodec_send_packet(codec_context_, packet->av_packet) < 0) {
        std::cerr << "Error sending audio packet for decoding." << std::endl;
        return;
    }
    // 分配AVFrame用于存储解码结果
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Failed to allocate audio frame." << std::endl;
        return;
    }
    // 接收解码后的音频帧
    while (avcodec_receive_frame(codec_context_, frame) >= 0) {
        // 计算目标采样率下的延迟补偿样本数
        int dst_nb_samples =
            av_rescale_rnd(swr_get_delay(swr_context_, codec_context_->sample_rate) + frame->nb_samples,
                           traget_sample_rate_, codec_context_->sample_rate, AV_ROUND_UP);
        // 计算输出缓冲区大小
        int buffer_size = av_samples_get_buffer_size(nullptr, traget_channels_, dst_nb_samples, AV_SAMPLE_FMT_S16, 1);
        // 分配重采样缓冲区
        uint8_t* buffer = (uint8_t*)av_malloc(buffer_size);
        // 执行音量重采样
        int ret = swr_convert(swr_context_, &buffer, dst_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
        if (ret < 0) {
            std::cerr << "Error while converting." << std::endl;
            av_free(buffer);
            av_frame_free(&frame);
            return;
        }
        // 创建音频样本数据结构
        std::shared_ptr<IAudioSamples> samples = std::make_shared<IAudioSamples>();
        samples->channels = traget_channels_;
        samples->sample_rate = traget_sample_rate_;
        samples->pts = frame->pts;
        samples->duration = frame->duration;
        samples->timebase_num= time_base_.num;
        samples->timebase_den= time_base_.den;
        samples->pcm_data.assign((int16_t*)buffer, (int16_t*)(buffer + buffer_size));
        samples->release_callback= pipeline_release_callback_;
        av_free(buffer);

        --pipeline_resource_count_;

        std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
        if (listener_) listener_->OnNotifyAudioSamples(samples);
    }

    av_frame_free(&frame);
}

// void AudioDecoder::DecodeAVPacket() {
//     std::shared_ptr<IAVPacket> packet;
//     {
//         std::lock_guard<std::mutex> lock(packets_queue_mutex_);
//         if (packets_queue_.empty()) return;
//         packet = packets_queue_.front();
//         packets_queue_.pop();
//     }
//
//     // 发送数据包到解码器
//     if (packet->av_packet && avcodec_send_packet(codec_context_, packet->av_packet) < 0) {
//         std::cerr << "Error sending audio packet for decoding." << std::endl;
//         return;
//     }
//
//     AVFrame* frame = av_frame_alloc();
//     if (!frame) {
//         std::cerr << "Failed to allocate audio frame." << std::endl;
//         return;
//     }
//
//     while (avcodec_receive_frame(codec_context_, frame) >= 0) {
//         // 重采样处理
//         int dst_nb_samples = av_rescale_rnd(
//             swr_get_delay(swr_context_, codec_context_->sample_rate) + frame->nb_samples,
//             traget_sample_rate_, codec_context_->sample_rate, AV_ROUND_UP);
//
//         uint8_t* buffer = nullptr;
//         int buffer_size = av_samples_get_buffer_size(
//             nullptr, traget_channels_, dst_nb_samples, AV_SAMPLE_FMT_S16, 1);
//
//         buffer = static_cast<uint8_t*>(av_malloc(buffer_size));
//         int ret = swr_convert(swr_context_, &buffer, dst_nb_samples,
//                             (const uint8_t**)frame->data, frame->nb_samples);
//         if (ret < 0) {
//             av_free(buffer);
//             av_frame_free(&frame);
//             return;
//         }
//
//         // SoundTouch处理
//         const int16_t* pcm_data = reinterpret_cast<int16_t*>(buffer);
//         const int num_samples = ret * traget_channels_;
//
//         // 分块处理防止内存溢出
//         const int chunk_size = 1024 * traget_channels_;
//         soundtouch_buffer_.insert(soundtouch_buffer_.end(), pcm_data, pcm_data + num_samples);
//
//         while (soundtouch_buffer_.size() >= chunk_size) {
//             sound_touch_.putSamples(soundtouch_buffer_.data(), chunk_size / traget_channels_);
//             soundtouch_buffer_.erase(soundtouch_buffer_.begin(), soundtouch_buffer_.begin() + chunk_size);
//         }
//
//         // 获取变速后的数据
//         std::vector<int16_t> processed_pcm;
//         while (true) {
//             int16_t temp_buffer[4096];
//             const int max_samples = sizeof(temp_buffer) / sizeof(int16_t);
//
//             int received = sound_touch_.receiveSamples(temp_buffer,
//                 max_samples / traget_channels_);
//
//             if (received == 0) {break;}
//
//             processed_pcm.insert(processed_pcm.end(),
//                 temp_buffer,temp_buffer + received * traget_channels_);
//
//         }
//
//         // 构建音频样本
//         auto samples = std::make_shared<IAudioSamples>();
//         samples->channels = traget_channels_;
//         samples->sample_rate = traget_sample_rate_;
//         samples->pts = frame->pts / playback_speed_;
//         samples->duration = frame->duration / playback_speed_;
//         samples->timebase_num= time_base_.num;
//         samples->timebase_den= time_base_.den;
//         // std::cout<<"samples->pts = "<<samples->pts<<"samples->duration = "<<samples->duration<<std::endl;
//
//         // 内存优化：直接交换数据所有权
//         samples->pcm_data.swap(processed_pcm);
//         samples->release_callback = pipeline_release_callback_;
//
//         // 通知监听器
//         {
//             std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
//             if (listener_) listener_->OnNotifyAudioSamples(samples);
//         }
//
//         --pipeline_resource_count_;
//         av_free(buffer);
//     }
//     av_frame_free(&frame);
// }

void AudioDecoder::ReleaseAudioPipelineResource() {
    ++pipeline_resource_count_;
    notifier_.Notify();
}

void AudioDecoder::CleanupContext() {
    if (codec_context_) {
        av_freep(&codec_context_);
    }
    if (swr_context_) {
        swr_free(&swr_context_);
    }
}



void AudioDecoder::CleanPacketsQueue() {
    std::queue<std::shared_ptr<IAVPacket>> tem_queue;
    std::swap(packets_queue_, tem_queue);
}

}
