//
// Created by 刘继玺 on 25-3-2.
//

#include "audio_pipeline.h"

namespace jxplay {
IAudioPipeline *IAudioPipeline::Create(unsigned int channels, unsigned int sample_rate) {
    return new AudioPipeline(channels, sample_rate);
}
AudioPipeline::AudioPipeline(unsigned int channels, unsigned int sample_rate):channels_(channels), sample_rate_(sample_rate),sound_touch_() {
    sound_touch_.setSampleRate(sample_rate_);     // 目标采样率
    sound_touch_.setChannels(channels_);          // 目标声道数
    sound_touch_.setTempo(playback_speed_);              // 初始播放速度
    sound_touch_.setSetting(SETTING_USE_QUICKSEEK, 0);   // 关闭快速模式保证音质
}


void AudioPipeline::SetListener(Listener *listener) {
    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    listener_ = listener;
}
void AudioPipeline::NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) {

    if (playback_speed_ != 1.0f) {
        // SoundTouch处理
        const int16_t* pcm_data = audio_samples->pcm_data.data();
        int num_samples = audio_samples->pcm_data.size() / channels_;

        // 将原始音频样本送入SoundTouch进行变速处理
        sound_touch_.putSamples(pcm_data, num_samples);

        static std::vector<int16_t> processed_pcm;
        processed_pcm.clear();
        // 从SoundTouch接收变速后的音频样本
        while (true) {
            int16_t temp_buffer[audio_samples->pcm_data.size()];
            // int received = sound_touch_.receiveSamples(temp_buffer, 4096 / channels_);
            int received = sound_touch_.receiveSamples(temp_buffer, num_samples);

            if (received <= 0) {
                break; // 没有更多数据时退出循环
            }
            // 将接收到的音频数据添加到processed_pcm向量中
            processed_pcm.insert(processed_pcm.end(), temp_buffer, temp_buffer + received * audio_samples->channels);
        }

        // 重新赋值对象
        audio_samples->pcm_data.swap(processed_pcm);

        // 更新时间戳和持续时间
        // audio_samples->pts = audio_samples->pts / playback_speed_; // 更新时间戳
        audio_samples->duration = audio_samples->duration / playback_speed_; // 更新持续时间
    }

    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    if (listener_) {
       listener_->OnAudioPipelineNotifyAudioSamples(audio_samples);
    }
}

void AudioPipeline::NotifyAudioFinished() {
    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    if (listener_) {
        listener_->OnAudioPipelineNotifyAudioFinished();
    }
}

void AudioPipeline::NotifyPlaybackSpeed(float speed) {
    playback_speed_ = speed;
    sound_touch_.setTempo(playback_speed_);
}
}