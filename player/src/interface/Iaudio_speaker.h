//
// Created by 刘继玺 on 25-3-3.
//

#ifndef IAUDIO_SPEAKER_H
#define IAUDIO_SPEAKER_H
#include <memory>

#include "../define/Iaudio_samples.h"

namespace jxplay {

struct IAudioSpeaker{
public:
  virtual void PlayAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) = 0;
  virtual void Stop() = 0;

  virtual ~IAudioSpeaker() = default;

  static IAudioSpeaker* Create(unsigned int channels, unsigned int sample_rate);
};

}

#endif //IAUDIO_SPEAKER_H
