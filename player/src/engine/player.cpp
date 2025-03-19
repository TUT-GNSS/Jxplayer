//
// Created by 刘继玺 on 25-3-1.
//

#include "player.h"

#include <iostream>

#include "audio_pipeline.h"
#include "../core/sync_notifier.h"
#include "player/src/writer/file_writer.h"

namespace jxplay {

IPlayer* IPlayer::Create(GLContext gl_context) { return new Player(gl_context);}

Player::Player(GLContext& gl_context):gl_context_(gl_context),task_pool_gl_context_(gl_context) {

   task_pool_ = std::make_shared<TaskPool>();
   InitTaskPoolGLContext();

   // 文件读取器
   file_reader_ = std::shared_ptr<IFileReader>(IFileReader::Create());
   //音视频同步器
   synchronizer_ = std::make_shared<AVSynchronizer>(gl_context_);
   // 音视频管线
   audio_pipeline_ = std::shared_ptr<IAudioPipeline>(IAudioPipeline::Create(2,44100));
   video_pipeline_ = std::shared_ptr<IVideoPipeline>(IVideoPipeline::Create(gl_context));

   // 音频输出设备
   audio_speaker_ = std::shared_ptr<IAudioSpeaker>(IAudioSpeaker::Create(2,44100));

   // 串联模块
   file_reader_->SetListener(this);
   synchronizer_->SetListener(this);
   audio_pipeline_->SetListener(this);
   video_pipeline_->SetListener(this);
}

Player::~Player() {
   file_reader_->Stop();
   synchronizer_->Stop();
   video_pipeline_->Stop();
   audio_speaker_->Stop();

   for (auto& display : display_views_set_) {
      display->Clear();
   }
   display_views_set_.clear();

   file_reader_ = nullptr;
   synchronizer_ = nullptr;
   video_pipeline_ = nullptr;
   audio_pipeline_ = nullptr;
   audio_speaker_ = nullptr;
   file_writer_ = nullptr;

   DestroyTaskPoolGLContext();
}

void Player::InitTaskPoolGLContext() {
   std::cout<<"InitTaskPoolGLContext"<<std::endl;
   task_pool_->SubmitTask([this]() {
      task_pool_gl_context_.Initialize();
      task_pool_gl_context_.MakeCurrent();
   });
}
void Player::DestroyTaskPoolGLContext() {
   std::cout<<"DestroyTaskPoolGLContext"<<std::endl;
   SyncNotifier notifier;
   task_pool_->SubmitTask([&]() {
      task_pool_gl_context_.MakeCurrent();
      task_pool_gl_context_.Destroy();
      notifier.Notify();
   });
   notifier.Wait();
}

void Player::AttachDisplayView(std::shared_ptr<IVideoDisplayView> display_view) {
   std::lock_guard<std::recursive_mutex> lock(display_view_mutex_);
   display_view->SetTaskPool(task_pool_);
   display_views_set_.insert(display_view);
}

void Player::DetachDisplayView(std::shared_ptr<IVideoDisplayView> display_view) {
   std::lock_guard<std::recursive_mutex> lock(display_view_mutex_);
   display_view->Clear();
   display_views_set_.erase(display_view);
}

void Player::SetPlaybackListener(std::shared_ptr<IPlaybackListener> listener) {
   std::lock_guard<std::recursive_mutex> lock(playback_listener_mutex_);
   playback_listener_ = listener;
}

bool Player::Open(std::string &file_path) {
   if (!file_reader_) {
      return false;
   }
   return file_reader_->Open(file_path);
}

void Player::Play() {
   if (file_reader_) {
      file_reader_->Start();
   }
   is_playing_ = true;
   std::lock_guard<std::recursive_mutex> lock(playback_listener_mutex_);
   if (playback_listener_) {
      playback_listener_->NotifyPlaybackStarted();
   }
}

void Player::Pause() {
   if (!file_reader_) {
      return;
   }

   file_reader_->Pause();
   is_playing_ = false;
   std::lock_guard<std::recursive_mutex> lock(playback_listener_mutex_);
   if (playback_listener_) {
      playback_listener_->NotifyPlaybackPaused();
   }
}

void Player::SeekTo(float position) {
   if (IsPlaying()) {
      Pause();
   }
   if (!file_reader_) {
      return;
   }
   file_reader_->SeekTo(position);
   synchronizer_->Reset();
}

bool Player::IsPlaying() {
   return is_playing_;
}

std::shared_ptr<IVideoFilter> Player::AddVideoFilter(VideoFilterType type) {
   return video_pipeline_? video_pipeline_->AddVideoFilter(type) : nullptr;
}
void Player::RemoveVideoFilter(VideoFilterType type) {
   video_pipeline_->RemoveVideoFilter(type);
}

bool Player::StartRecording(const std::string &output_file_path, int flags) {
   std::lock_guard<std::mutex> lock(file_writer_mutex_);
   if (file_writer_) {
      file_writer_->StopWriter();
   }
   file_writer_ = std::shared_ptr<IFileWriter>(IFileWriter::Create(gl_context_));

   FileWriterParameters parameters;
   parameters.width = file_reader_->GetVideoWidth();
   parameters.height = file_reader_->GetVideoHeight();
   is_recording = file_writer_->StartWriter(output_file_path,parameters,flags);
   return is_recording;
}


void Player::StopRecording() {
   std::lock_guard<std::mutex> lock(file_writer_mutex_);
   if (file_writer_) {
      file_writer_->StopWriter();
      file_writer_ = nullptr;
   }
   is_recording = false;
}

bool Player::IsRecording() {
   return is_recording;
}

// 继承自IFileReader::Listener
void Player::OnFileReaderNotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) {
   if (synchronizer_) {
      synchronizer_->NotifyAudioSamples(audio_samples);
   }
}

void Player::OnFileReaderNotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
   if (synchronizer_) {
      synchronizer_->NotifyVideoFrame(video_frame);
   }
}

void Player::OnFileReaderNotifyAudioFinished() {
   if (synchronizer_) {
      synchronizer_->NotifyAudioFinished();
   }
}

void Player::OnFileReaderNotifyVideoFinished() {
   if (synchronizer_) {
      synchronizer_->NotifyVideoFinished();
   }
}

// 继承自AVSynchronizer::Listener
void Player::OnAVSynchronizerNotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) {
   if (audio_pipeline_) {
      audio_pipeline_->NotifyAudioSamples(audio_samples);
   }
}

void Player::OnAVSynchronizerNotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
   if (video_pipeline_) {
      video_pipeline_->NotifyVideoFrame(video_frame);
   }
}

void Player::OnAVSynchronizerNotifyAudioFinished() {
   if (audio_pipeline_) {
      audio_pipeline_->NotifyAudioFinished();
   }
}

void Player::OnAVSynchronizerNotifyVideoFinished() {
   if (video_pipeline_) {
      video_pipeline_->NotifyVideoFinished();
   }
}


// 继承自IAudioPipeline::Listener
void Player::OnAudioPipelineNotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples) {
   if (audio_speaker_) {
      audio_speaker_->PlayAudioSamples(audio_samples);
   }
   {
      std::lock_guard<std::mutex> lock(file_writer_mutex_);
      if (file_writer_) {
         file_writer_->NotifyAudioSamples(audio_samples);
      }
   }
   std::lock_guard<std::recursive_mutex> lock(playback_listener_mutex_);
   if (playback_listener_) {
      playback_listener_->NotifyPlaybackTimeChanged(audio_samples->GetTimeStamp(),file_reader_->GetDuration());
   }

}
void Player::OnAudioPipelineNotifyAudioFinished() {
   {
      std::lock_guard<std::recursive_mutex> lock(playback_listener_mutex_);
      if (playback_listener_) {
         playback_listener_->NotifyPlaybackEOF();
      }
   }
   {
      std::lock_guard<std::mutex> lock(file_writer_mutex_);
      if (file_writer_) {
         file_writer_->NotifyAudioFinished();
      }
   }
}

// 继承自IVideoPipeline::Listener
void Player::OnVideoPipelineNotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
   for (const auto& display_view : display_views_set_) {
      display_view->Render(video_frame,IVideoDisplayView::EContentMode::kScaleAspectFit);
   }
   {
      std::lock_guard<std::mutex>lock(file_writer_mutex_);
      if (file_writer_) {
         file_writer_->NotifyVideoFrame(video_frame);
      }
   }
}

void Player::OnVideoPipelineNotifyVideoFinished() {
   std::lock_guard<std::mutex> lock(file_writer_mutex_);
   if (file_writer_) {
      file_writer_->NotifyVideoFinished();
   }
}

void Player::SetPlaybackSpeed(float speed) {
   audio_pipeline_->NotifyPlaybackSpeed(speed);
}

}
