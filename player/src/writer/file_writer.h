//
// Created by 刘继玺 on 25-3-5.
//

#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include "audio_encoder.h"
#include "video_encoder.h"
#include "muxer.h"
#include "../interface/Ifile_writer.h"

namespace jxplay {
class FileWriter:public IFileWriter, AudioEncoder::Listener, VideoEncoder::Listener {
public:
    explicit FileWriter(GLContext& gl_context);
    ~FileWriter() override;

    bool StartWriter(const std::string &output_file_path, FileWriterParameters &parameters, int flags) override;
    void StopWriter() override;

    void NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) override;
    void NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) override;

    void NotifyAudioFinished() override;
    void NotifyVideoFinished() override;
private:
    // 继承自 AudioEncoder::Listener
    void OnAudioEncoderNotifyPacket(std::shared_ptr<IAVPacket>) override;
    void OnAudioEncoderNotifyFinished() override;

    // 继承自 VideoEncoder::Listener
    void OnVideoEncoderNotifyPacket(std::shared_ptr<IAVPacket>) override;
    void OnVideoEncoderNotifyFinished() override;
private:
    std::shared_ptr<IAudioEncoder> audio_encoder_;
    std::shared_ptr<IVideoEncoder> video_encoder_;
    std::shared_ptr<IMuxer> muxer_;

};
}



#endif //FILE_WRITER_H
