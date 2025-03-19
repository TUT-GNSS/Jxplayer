//
// Created by 刘继玺 on 25-3-1.
//

#ifndef PLAYER_H
#define PLAYER_H

#include <unordered_set>


#include "av_synchronizer.h"
#include "../core/task_pool.h"
#include "../../include/Iplayer.h"
#include "../interface/Ifile_reader.h"
#include "../interface/Ivideo_display_view.h"
#include "../interface/Ivideo_pipeline.h"
#include "../interface/Iaudio_pipeline.h"
#include "../interface/Iaudio_speaker.h"
#include "../interface/Ifile_writer.h"

namespace jxplay {
class Player : public IPlayer, IFileReader::Listener,AVSynchronizer::Listener,
                IVideoPipeline::Listener,IAudioPipeline::Listener{
public:
    Player(GLContext& gl_context);
    ~Player() override;

    void AttachDisplayView(std::shared_ptr<IVideoDisplayView> display_view) override;
    void DetachDisplayView(std::shared_ptr<IVideoDisplayView> display_view) override;

    void SetPlaybackListener(std::shared_ptr<IPlaybackListener> listener) override;

    bool Open(std::string& file_path) override;
    void Play() override;
    void Pause() override;
    void SeekTo(float progress) override;
    bool IsPlaying() override;

    std::shared_ptr<IVideoFilter> AddVideoFilter(VideoFilterType type) override;
    void RemoveVideoFilter(VideoFilterType type) override;

    bool StartRecording(const std::string& output_file_path, int flags) override;
    void StopRecording() override;
    bool IsRecording() override;

    void SetPlaybackSpeed(float speed) override;
private:
    void InitTaskPoolGLContext();
    void DestroyTaskPoolGLContext();

    // 继承自IFileReader::Listener
    void OnFileReaderNotifyAudioSamples(std::shared_ptr<IAudioSamples>) override;
    void OnFileReaderNotifyVideoFrame(std::shared_ptr<IVideoFrame>) override;
    void OnFileReaderNotifyAudioFinished() override;
    void OnFileReaderNotifyVideoFinished() override;

    // 继承自AVSynchronizer::Listener
    void OnAVSynchronizerNotifyAudioSamples(std::shared_ptr<IAudioSamples>) override;
    void OnAVSynchronizerNotifyVideoFrame(std::shared_ptr<IVideoFrame>) override;
    void OnAVSynchronizerNotifyAudioFinished() override;
    void OnAVSynchronizerNotifyVideoFinished() override;

    // 继承自IAudioPipeline::Listener
    void OnAudioPipelineNotifyAudioSamples(std::shared_ptr<IAudioSamples>) override;
    void OnAudioPipelineNotifyAudioFinished() override;

    // 继承自IVideoPipeline::Listener
    void OnVideoPipelineNotifyVideoFrame(std::shared_ptr<IVideoFrame>) override;
    void OnVideoPipelineNotifyVideoFinished() override;;

private:
    GLContext gl_context_;
    GLContext task_pool_gl_context_;

    std::recursive_mutex playback_listener_mutex_;
    std::shared_ptr<IPlaybackListener> playback_listener_;

    // 文件读取器
    std::shared_ptr<IFileReader> file_reader_;

    // 音画同步
    std::shared_ptr<AVSynchronizer> synchronizer_;

    // 音频处理管线
    std::shared_ptr<IAudioPipeline> audio_pipeline_;
    //视频处理管线
    std::shared_ptr<IVideoPipeline> video_pipeline_;

    // 音频扬声器
    std::shared_ptr<IAudioSpeaker> audio_speaker_;

    // 视频播放
    std::recursive_mutex display_view_mutex_;
    std::unordered_set<std::shared_ptr<IVideoDisplayView>> display_views_set_;

    // 视频录制
    std::mutex file_writer_mutex_;
    std::shared_ptr<IFileWriter> file_writer_;

    // 用于执行需要GL环境的操作 如销毁纹理
    std::shared_ptr<TaskPool> task_pool_;

    std::atomic<bool> is_playing_ = false;
    std::atomic<bool> is_recording = false;

};
}


#endif //PLAYER_H
