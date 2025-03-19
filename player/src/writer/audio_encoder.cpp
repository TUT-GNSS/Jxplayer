//
// Created by 刘继玺 on 25-3-4.
//

#include "audio_encoder.h"

#include <iostream>

namespace jxplay {

AudioEncoder::AudioEncoder() : thread_(&AudioEncoder::ThreadLoop, this) {}

AudioEncoder::~AudioEncoder() {
    StopThread();

    if (av_frame_) {
        av_frame_free(&av_frame_);
    }
    if (av_packet_) {
        av_packet_free(&av_packet_);
    }
    if (encoder_context_) {
        avcodec_free_context(&encoder_context_);
    }
    if (swr_context_) {
        swr_free(&swr_context_);
    }
}

void AudioEncoder::SetListener(Listener *listener) {
    std::lock_guard<std::mutex> lock(listener_mutex_);
    listener_ = listener;
}

bool AudioEncoder::Configure(FileWriterParameters& parameters, int flags) {
    // 找到编码器
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);

    if (!codec) {
        return false;
    }
    encoder_context_ = avcodec_alloc_context3(codec);
    if (!encoder_context_) {
        std::cerr<<"Failed to allocate audio codec context"<<std::endl;
        return false;
    }

    //配置编码器
    encoder_context_->sample_rate = parameters.sample_rate;
    av_channel_layout_default(&encoder_context_->ch_layout,parameters.channels);
    encoder_context_->sample_fmt = AV_SAMPLE_FMT_FLTP;
    encoder_context_->bit_rate = 128000;
    encoder_context_->frame_size = 1024;

    // 打开编码器
    if (avcodec_open2(encoder_context_, codec, nullptr) < 0) {
        std::cerr<<"Failed to open audio codec"<<std::endl;
        return false;
    }

    av_packet_ = av_packet_alloc();
    if (!av_packet_) {
        std::cerr<<"Failed to allocate audio packet"<<std::endl;
        return false;
    }
    av_frame_ = av_frame_alloc();
    if (!av_frame_) {
        std::cerr<<"Failed to allocate audio frame"<<std::endl;
        return false;
    }

    // 配置AVFrame
    av_frame_->nb_samples = encoder_context_->frame_size;
    av_frame_->format = encoder_context_->sample_fmt;
    av_frame_->ch_layout = encoder_context_->ch_layout;
    av_frame_->sample_rate = encoder_context_->sample_rate;
    int ret = av_frame_get_buffer(av_frame_, 0);
    if (ret < 0) {
        std::cerr << "Could not allocate audio frame buffer" << std::endl;
        return false;
    }

    // 配置重采样器
    // swr_context_ = swr_alloc();
    // av_opt_set_int(swr_context_,"in_channel_layout",AV_CH_LAYOUT_STEREO,0);
    // av_opt_set_int(swr_context_, "in_sample_rate", parameters.sample_rate, 0);
    // av_opt_set_sample_fmt(swr_context_, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    // av_opt_set_int(swr_context_, "out_channel_layout", encoder_context_->ch_layout.u.mask, 0);
    // av_opt_set_int(swr_context_, "out_sample_rate", encoder_context_->sample_rate, 0);
    // av_opt_set_sample_fmt(swr_context_, "out_sample_fmt", encoder_context_->sample_fmt, 0);


    // 配置重采样器
    // 初始化输入声道布局（立体声）
    AVChannelLayout in_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    ret = swr_alloc_set_opts2(
        &swr_context_, // 自动分配新上下文
        &encoder_context_->ch_layout, // 输出声道布局
        encoder_context_->sample_fmt,       // 输出采样格式
        encoder_context_->sample_rate,      // 输出采样率
        &in_ch_layout,                // 输入声道布局（假设输入为立体声）
        AV_SAMPLE_FMT_S16,                  // 输入采样格式
        parameters.sample_rate,             // 输入采样率
        0, nullptr                          // 日志参数
    );

    av_channel_layout_uninit(&in_ch_layout);

    if (ret<0 || swr_init(swr_context_) < 0) {
        std::cerr << "Could not initialize swr context" << std::endl;
        return false;
    }

    return true;
}

void AudioEncoder::NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) {
    if (!audio_samples) {
        return;
    }
    std::lock_guard<std::mutex> lock(audio_samples_queue_mutex_);
    audio_samples_queue_.push(audio_samples);
    condition_var_.notify_all();
}

void AudioEncoder::ThreadLoop() {
    while (true) {
        std::shared_ptr<IAudioSamples> audio_samples;
        {
            std::unique_lock<std::mutex> lock(audio_samples_queue_mutex_);
            condition_var_.wait(lock, [this]{return abort_||!audio_samples_queue_.empty();});
            if (abort_) {break;}
            audio_samples = audio_samples_queue_.front();
            audio_samples_queue_.pop();
        }
        if (audio_samples->flags & static_cast<int>(AVFrameFlag::kEOS)) {
            EncodeAudioSamples(nullptr);
        }else {
            PrepareEncodeAudioSamples(audio_samples);
        }
    }
    {
        std::unique_lock<std::mutex> lock(audio_samples_queue_mutex_);
        CleanPacketsQueue();
    }
}
void AudioEncoder::StopThread() {
    abort_ = true;
    condition_var_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void AudioEncoder::PrepareEncodeAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) {
    if (!av_frame_||!av_packet_) {
        return;
    }
    int ret = av_frame_make_writable(av_frame_);
    if (ret < 0) {
        std::cerr << "av_frame can't make writable!" << std::endl;
        return;
    }
    // 重采样时需要为每个声道输入数据
    const uint8_t* in_data[1]; // 输入双声道的数据
    in_data[0] = reinterpret_cast<const uint8_t*>(audio_samples->pcm_data.data()); // 输入交织的pcm数据

    // swr_convert 需要将每个声道数据分离
    uint8_t* out_data[2] = {av_frame_->data[0], av_frame_->data[1]};  // 输出两个声道的FLTP数据

    //计算输入样本数，交织数据的总样本数需要除声道数
    int in_samples = audio_samples->pcm_data.size() / audio_samples->channels;

    // 重采样 将交织的S16数据转换为FLTP格式
    int out_samples = swr_convert(swr_context_, out_data, av_frame_->nb_samples, in_data, in_samples);
    if (out_samples < 0) {
        std::cerr << "Error while resampling" << std::endl;
        return;
    }

    // 设置帧时间戳
    av_frame_->pts = pts_;
    EncodeAudioSamples(av_frame_);
    pts_ += av_frame_->nb_samples;
}

void AudioEncoder::EncodeAudioSamples(const AVFrame* audio_frame) {
    // 发送帧到编码器（允许处理EAGAIN）
    int send_ret = avcodec_send_frame(encoder_context_, audio_frame);
    if (send_ret < 0 && send_ret != AVERROR(EAGAIN)) {
        std::cerr << "Error sending frame: " << av_err2str(send_ret) << std::endl;
        return;
    }

    // 持续接收所有可用数据包
    while (true) {
        int recv_ret = avcodec_receive_packet(encoder_context_, av_packet_);
        if (recv_ret == AVERROR(EAGAIN) || recv_ret == AVERROR_EOF) {
            break; // 正常退出循环
        } else if (recv_ret < 0) {
            std::cerr << "Error receiving packet: " << av_err2str(recv_ret) << std::endl;
            break;
        }

        // 处理有效数据包
        auto av_packet = std::make_shared<IAVPacket>(av_packet_clone(av_packet_));
        av_packet->time_base = encoder_context_->time_base;

        {
            std::lock_guard<std::mutex> lock(listener_mutex_);
            if (listener_) {
                listener_->OnAudioEncoderNotifyPacket(av_packet);
            }
        }
        av_packet_unref(av_packet_);
    }

    // 通知编码完成
    if (!audio_frame) {
        std::lock_guard<std::mutex> lock(listener_mutex_);
        if (listener_) {
            listener_->OnAudioEncoderNotifyFinished();
        }
    }
}


void AudioEncoder::CleanPacketsQueue() {
    std::queue<std::shared_ptr<IAudioSamples>> temp_queue;
    std::swap(audio_samples_queue_, temp_queue);
}

}
