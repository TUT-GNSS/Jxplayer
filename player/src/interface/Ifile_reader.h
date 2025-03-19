//
// Created by 刘继玺 on 25-3-1.
//

#ifndef IFILE_READER_H
#define IFILE_READER_H

#include "../define/Ivideo_frame.h"
#include "../define/Iaudio_samples.h"

namespace jxplay{
class IFileReader{
public:
    struct Listener{
        virtual void OnFileReaderNotifyAudioSamples(std::shared_ptr<IAudioSamples>) = 0;
        virtual void OnFileReaderNotifyVideoFrame(std::shared_ptr<IVideoFrame>) = 0;

        virtual void OnFileReaderNotifyAudioFinished() = 0;
        virtual void OnFileReaderNotifyVideoFinished() = 0;
        virtual ~Listener() = default;
    };
    virtual void SetListener(IFileReader::Listener* listener) = 0;

    virtual bool Open(std::string& filePath) = 0;
    virtual void Start() = 0;
    virtual void Pause() = 0;
    virtual void SeekTo(float progress) = 0;
    virtual void Stop() = 0;

    virtual float GetDuration() = 0;
    virtual int GetVideoWidth() = 0;
    virtual int GetVideoHeight() = 0;

    virtual ~IFileReader() = default;

    static IFileReader* Create();
};
}

#endif //IFILE_READER_H
