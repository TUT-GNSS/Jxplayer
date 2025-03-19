//
// Created by 刘继玺 on 25-3-2.
//

#ifndef IAUDIO_PIPELINE_H
#define IAUDIO_PIPELINE_H
#include <memory>

#include "../define/Iaudio_samples.h"

namespace jxplay {
class IAudioPipeline {
public:
    struct Listener {
        virtual void OnAudioPipelineNotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) = 0;
        virtual void OnAudioPipelineNotifyAudioFinished() = 0;
        virtual ~Listener() = default;
    };

    virtual void SetListener(Listener* listener) = 0;
    virtual void NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) = 0;
    virtual void NotifyAudioFinished() = 0;

    virtual void NotifyPlaybackSpeed(float speed) = 0;

    virtual ~IAudioPipeline() = default;
    static IAudioPipeline* Create(unsigned int channels, unsigned int sample_rate);
};
}
#endif //IAUDIO_PIPELINE_H
