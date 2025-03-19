//
// Created by 刘继玺 on 25-3-5.
//

#include "file_writer.h"

#include <iostream>

namespace jxplay {
IFileWriter* IFileWriter::Create(GLContext& gl_context) {
    return new FileWriter(gl_context);
}

FileWriter::FileWriter(GLContext& gl_context) {
    audio_encoder_ = std::make_shared<AudioEncoder>();
    video_encoder_ = std::make_shared<VideoEncoder>(gl_context );
    muxer_ = std::make_shared<Muxer>();

    audio_encoder_->SetListener(this);
    video_encoder_->SetListener(this);
}

FileWriter::~FileWriter() {
    StopWriter();
    audio_encoder_ = nullptr;
    video_encoder_ = nullptr;
    muxer_ = nullptr;
}

bool FileWriter::StartWriter(const std::string &output_file_path, FileWriterParameters &parameters, int flags) {
    bool succ1 = audio_encoder_->Configure(parameters, flags);
    bool succ2 = video_encoder_->Configure(parameters, flags);
    bool succ3 = muxer_->Configure(output_file_path, parameters, flags);

    std::cout << "FileWriter::StartWriter: " << succ1 << " " << succ2 << " " << succ3 << std::endl;
    return succ1 && succ2 && succ3;
}

void FileWriter::StopWriter() {
    NotifyAudioFinished();
    NotifyVideoFinished();
}

void FileWriter::NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) {
    if (audio_encoder_) {
        audio_encoder_->NotifyAudioSamples(audio_samples);
    }
}

void FileWriter::NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
    if (video_encoder_) {
        video_encoder_->NotifyVideoFrame(video_frame);
    }
}

void FileWriter::NotifyAudioFinished() {
    auto audio_samples = std::make_shared<IAudioSamples>();
    audio_samples->flags |= static_cast<int> (AVFrameFlag::kEOS);
    NotifyAudioSamples(audio_samples);
}
void FileWriter::NotifyVideoFinished() {
    auto video_samples = std::make_shared<IVideoFrame>();
    video_samples->flags |= static_cast<int> (AVFrameFlag::kEOS);
    NotifyVideoFrame(video_samples);
}

// 继承自 AudioEncoder::Listener
void FileWriter::OnAudioEncoderNotifyPacket(std::shared_ptr<IAVPacket> packet) {
    if (muxer_) {
        muxer_->NotifyAudioPacket(packet);
    }
}

void FileWriter::OnAudioEncoderNotifyFinished() {
    if (muxer_) {
        muxer_->NotifyAudioFinished();
    }
}

// 继承自 VideoEncoder::Listener
void FileWriter::OnVideoEncoderNotifyPacket(std::shared_ptr<IAVPacket> packet) {
    if (muxer_) {
        muxer_->NotifyVideoPacket(packet);
    }
}

void FileWriter::OnVideoEncoderNotifyFinished() {
    if (muxer_) {
        muxer_->NotifyVideoFinished();
    }
}


}
