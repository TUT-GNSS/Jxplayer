//
// Created by 刘继玺 on 25-3-2.
//

#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include "../define/Iaudio_samples.h"
#include "../interface/Iaudio_pipeline.h"
#include "../../../third_party/soundtouch/include/SoundTouch.h"

namespace jxplay{
class AudioPipeline : public IAudioPipeline{
public:
    AudioPipeline(unsigned int channels, unsigned int sample_rate);
    ~AudioPipeline() override = default;

    void SetListener(Listener *listener) override;
    void NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) override;
    void NotifyAudioFinished() override;

    void NotifyPlaybackSpeed(float speed) override;

private:
    unsigned int channels_ = 2;
    unsigned int sample_rate_ = 44100;

    std::recursive_mutex listener_mutex_;
    IAudioPipeline::Listener *listener_;

    soundtouch::SoundTouch sound_touch_;
    float playback_speed_ = 1.0f; // 默认正常速度
    std::vector<int16_t> soundtouch_buffer_;  // 输入缓冲队列
};
}


#endif //AUDIO_PIPELINE_H
