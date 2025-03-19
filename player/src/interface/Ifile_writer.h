//
// Created by 刘继玺 on 25-3-5.
//

#ifndef IFILE_WRITER_H
#define IFILE_WRITER_H

#include "../define/file_writer_parameters.h"
#include "../define/Iaudio_samples.h"
#include "../define/Ivideo_frame.h"
#include "../../include/Igl_context.h"

namespace jxplay{
class IFileWriter{
public:
    virtual bool StartWriter(const std::string& output_file_path, FileWriterParameters& parameters, int flags) = 0;
    virtual void StopWriter() = 0;

    virtual void NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) = 0;
    virtual void NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) = 0;

    virtual void NotifyAudioFinished() = 0;
    virtual void NotifyVideoFinished() = 0;

    virtual ~IFileWriter() = default;

    static IFileWriter* Create(GLContext& gl_context);
};

};
#endif //IFILE_WRITER_H
