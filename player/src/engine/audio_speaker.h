//
// Created by 刘继玺 on 25-3-3.
//

#ifndef AUDIO_SPEAKER_H
#define AUDIO_SPEAKER_H

#include <deque>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <list>
#include <mutex>

#include "../interface/Iaudio_speaker.h"

namespace jxplay {
class AudioSpeaker : public IAudioSpeaker, QIODevice{
public:
    AudioSpeaker(unsigned int channels, unsigned int sample_rate);
    ~AudioSpeaker() override;

    void PlayAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) override;
    void Stop() override;

private:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
private:
    QAudioSink *auto_sink_ptr_ = nullptr;
    QMediaDevices *output_devices_ptr_ = nullptr;
    QAudioDevice output_device_;
    QAudioSink *audio_sink_output_ptr_ = nullptr;
    IAudioSamples audio_samples_;

    std::mutex audio_samples_list_mutex_;
    std::list<std::shared_ptr<IAudioSamples>> audio_samples_list_;
};
}

#endif //AUDIO_SPEAKER_H
