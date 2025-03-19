//
// Created by 刘继玺 on 25-2-27.
//
#include "../../define/Iav_packet.h"
#include "../../define/Iaudio_samples.h"

#ifndef IAUDIO_DECODER_H
#define IAUDIO_DECODER_H
struct AVStream;

namespace jxplay {

class IAudioDecoder{
public:
  struct Listener{
    virtual void OnNotifyAudioSamples(std::shared_ptr<IAudioSamples> samples) = 0;
    virtual ~Listener() = default;
  };
  virtual void SetListener(Listener* listener) = 0;
  virtual void SetStream(AVStream* stream) = 0;
  virtual void Decode(std::shared_ptr<IAVPacket> packet) = 0;
  virtual void Start() = 0;
  virtual void Stop() = 0;
  virtual void Pause() = 0;

  virtual ~IAudioDecoder() = default;

  static IAudioDecoder* Create(unsigned int channels, unsigned int sample_rate);

};
}
#endif //IAUDIO_DECODER_H
