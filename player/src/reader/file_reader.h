//
// Created by 刘继玺 on 25-3-1.
//

#ifndef FILE_READER_H
#define FILE_READER_H

#include "demuxer.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include "../interface/Ifile_reader.h"

namespace jxplay {
class FileReader : public IFileReader, DeMuxer::Listener, AudioDecoder::Listener, VideoDecoder::Listener {
public:
    FileReader();
    ~FileReader() override;

    void SetListener(IFileReader::Listener* listener) override;

    bool Open(std::string& file_path) override;
    void Start() override;
    void Pause() override;
    void SeekTo(float position) override;
    void Stop() override;

    float GetDuration() override;
    int GetVideoWidth() override;
    int GetVideoHeight() override;
private:
    // 继承自IDeMuxer::Listener
    void OnDeMuxStart() override;
    void OnDeMuxStop() override;
    void OnDeMuxEOF() override;
    void OnDeMuxError(int code, const char* msg) override;
    void OnNotifyAudioStream(struct AVStream* stream) override;
    void OnNotifyVideoStream(struct AVStream* stream) override;
    void OnNotifyAudioPacket(std::shared_ptr<IAVPacket> packet) override;
    void OnNotifyVideoPacket(std::shared_ptr<IAVPacket> packet) override;

    // 继承自IAudioDecoder::Listener
    void OnNotifyAudioSamples(std::shared_ptr<IAudioSamples>) override;

    // 继承自IVideoDecoder::Listener
    void OnNotifyVideoFrame(std::shared_ptr<IVideoFrame>) override;

private:
    // 数据回调
    IFileReader::Listener* listener_ = nullptr;
    std::recursive_mutex listener_mutex_;

    // 解封装器
    std::shared_ptr<IDeMuxer> demuxer_;

    // 解码器
    std::shared_ptr<IAudioDecoder> audio_decoder_;
    std::shared_ptr<IVideoDecoder> video_decoder_;
};
}


#endif //FILE_READER_H
