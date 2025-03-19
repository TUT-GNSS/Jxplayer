//
// Created by 刘继玺 on 25-3-4.
//

#ifndef IAUDIO_ENCODER_H
#define IAUDIO_ENCODER_H
#include "../../define/file_writer_parameters.h"
#include "../../define/Iaudio_samples.h"
#include "../../define/Iav_packet.h"

namespace jxplay{

class IAudioEncoder{
public:
  struct Listener{
    virtual void OnAudioEncoderNotifyPacket(std::shared_ptr<IAVPacket>) = 0;
    virtual void OnAudioEncoderNotifyFinished() = 0;
    virtual ~Listener() = default;
  };
  virtual void SetListener(Listener* listener) = 0;
  virtual bool Configure(FileWriterParameters& parameters, int flags) = 0;
  virtual void NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) = 0;
};

}

#endif //IAUDIO_ENCODER_H
