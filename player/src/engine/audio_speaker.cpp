//
// Created by 刘继玺 on 25-3-3.
//

#include "audio_speaker.h"

#include <stdint.h>
#include <stdio.h>

#include <iostream>

namespace jxplay {
IAudioSpeaker *IAudioSpeaker::Create(unsigned int channels, unsigned int sample_rate) {
    return new AudioSpeaker(channels, sample_rate);
}

AudioSpeaker::AudioSpeaker(unsigned int channels, unsigned int sample_rate) {

    output_devices_ptr_= new QMediaDevices(nullptr);
    output_device_ = output_devices_ptr_->defaultAudioOutput();
    QAudioFormat format = output_device_.preferredFormat();

    // 确保格式与输入音频一致
    format.setChannelCount(channels);
    format.setSampleRate(sample_rate);
    format.setSampleFormat(QAudioFormat::Int16);  // 使用 16 位的音频数据格式

    std::cout << "sampleRate: " << format.sampleRate() << ", channelCount: " << format.channelCount()
              << ", sampleFormat: " << format.sampleFormat() << std::endl;

    // 初始化 QAudioSink
    audio_sink_output_ptr_ = new QAudioSink(output_device_, format);

    // 增大缓冲区大小，避免频繁的数据耗尽问题
    audio_sink_output_ptr_->setBufferSize(16384);  // 调整缓冲区大小为 16 KB

    // 启动音频设备之前确保读取音频流
    open(QIODevice::ReadOnly);
    audio_sink_output_ptr_->start(this);  // 启动音频输出
}

AudioSpeaker::~AudioSpeaker() {
    close();
    audio_sink_output_ptr_->stop();  // 停止音频播放
}

void AudioSpeaker::PlayAudioSamples(std::shared_ptr<IAudioSamples> audioSamples) {
    if (!audioSamples) return;

    std::lock_guard<std::mutex> lock(audio_samples_list_mutex_);
    audio_samples_list_.push_back(audioSamples);
}

void AudioSpeaker::Stop() {
    std::lock_guard<std::mutex> lock(audio_samples_list_mutex_);
    audio_samples_list_.clear();
}

qint64 AudioSpeaker::readData(char *data, qint64 maxlen) {
     if (audio_samples_.pcm_data.empty() || audio_samples_.offset >= audio_samples_.pcm_data.size()) {
         std::lock_guard<std::mutex> lock(audio_samples_list_mutex_);
         if (audio_samples_list_.empty()) return 0;
         audio_samples_ = *audio_samples_list_.front();
         audio_samples_list_.pop_front();
     }

     // 计算实际可以读取的字节数，避免超出 buffer
     size_t bytes_to_read = std::min(static_cast<size_t>(maxlen),
                                   (audio_samples_.pcm_data.size() - audio_samples_.offset) * sizeof(int16_t));
     std::memcpy(data, audio_samples_.pcm_data.data() + audio_samples_.offset, bytes_to_read);

     // 更新音频样本的偏移量，如果音频样本的数据全部读完，则移出队列
     audio_samples_.offset += bytes_to_read / sizeof(int16_t);

     return bytes_to_read;  // 返回读取的字节数
}

qint64 AudioSpeaker::writeData(const char *data, qint64 len) { return 0; }

}