//
// Created by 刘继玺 on 25-3-1.
//

#include "file_reader.h"

#include <iostream>

namespace jxplay {

IFileReader* IFileReader::Create() { return new FileReader(); }

FileReader::FileReader() {
    // 解封装器
    demuxer_ = std::shared_ptr<IDeMuxer>(IDeMuxer::Create());
    // 音视频解码器
    audio_decoder_ = std::shared_ptr<IAudioDecoder>(IAudioDecoder::Create(2,44100));
    video_decoder_ = std::shared_ptr<IVideoDecoder>(IVideoDecoder::Create());

    // 串联模块
    demuxer_->SetListener(this);
    audio_decoder_->SetListener(this);
    video_decoder_->SetListener(this);
}
FileReader::~FileReader() {
    demuxer_->Pause();
    audio_decoder_ = nullptr;
    video_decoder_ = nullptr;
    demuxer_ = nullptr;
}

void FileReader::SetListener(IFileReader::Listener* listener) {
    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    listener_ = listener;
}

bool FileReader::Open(std::string& file_path) {
    if (!demuxer_) {
        return false;
    }
    return demuxer_->Open(file_path);
}

void FileReader::Start() {
    if (demuxer_) {
        demuxer_->Start();
    }
    if (audio_decoder_) {
        audio_decoder_->Start();
    }
    if (video_decoder_) {
        video_decoder_->Start();
    }
}

void FileReader::Pause() {
    if (demuxer_) {
        demuxer_->Pause();
    }
    if (audio_decoder_) {
        audio_decoder_->Pause();
    }
    if (video_decoder_) {
        video_decoder_->Pause();
    }
}

void FileReader::SeekTo(float position) {
    if (demuxer_) {
        demuxer_->SeekTo(position);
    }
}

void FileReader::Stop() {
    if (demuxer_) {
        demuxer_->Stop();
    }
    if (audio_decoder_) {
        audio_decoder_->Stop();
    }
    if (video_decoder_) {
        video_decoder_->Stop();
    }
}

float FileReader::GetDuration() {
    return demuxer_ ? demuxer_->GetDuration() : 0.0f;
}
int FileReader::GetVideoWidth() {
    return video_decoder_ ? video_decoder_->GetVideoWidth() : 0;
}
int FileReader::GetVideoHeight() {
    return video_decoder_ ? video_decoder_->GetVideoHeight() : 0;
}

// 继承自IDeMuxer::Listener
void FileReader::OnDeMuxStart() { std::cout << "FileReader::OnDeMuxStart" << std::endl; }

void FileReader::OnDeMuxStop() { std::cout << "FileReader::OnDeMuxStop" << std::endl; }

void FileReader::OnDeMuxEOF() { std::cout << "FileReader::OnDeMuxEOF" << std::endl; }

void FileReader::OnDeMuxError(int code, const char* msg) { std::cout << "FileReader::OnDeMuxError" << std::endl; }

void FileReader::OnNotifyAudioStream(struct AVStream* stream) {
    if (audio_decoder_) {
        audio_decoder_->SetStream(stream);
    }
}

void FileReader::OnNotifyVideoStream(struct AVStream* stream) {
    if (video_decoder_) {
        video_decoder_->SetStream(stream);
    }
}

void FileReader::OnNotifyAudioPacket(std::shared_ptr<IAVPacket> packet) {
    if (audio_decoder_) {
        audio_decoder_->Decode(packet);
    }
}

void FileReader::OnNotifyVideoPacket(std::shared_ptr<IAVPacket> packet) {
    if (video_decoder_) {
        video_decoder_->Decode(packet);
    }
}

// 继承自IAudioDecoder::Listener
void FileReader::OnNotifyAudioSamples(std::shared_ptr<IAudioSamples> audioSamples) {
    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    if (listener_) {
        listener_->OnFileReaderNotifyAudioSamples(audioSamples);
    }
}

// 继承自IVideoDecoder::Listener
void FileReader::OnNotifyVideoFrame(std::shared_ptr<IVideoFrame> videoFrame) {
    std::lock_guard<std::recursive_mutex> lock(listener_mutex_);
    if (listener_) {
        listener_->OnFileReaderNotifyVideoFrame(videoFrame);
    }
}

}