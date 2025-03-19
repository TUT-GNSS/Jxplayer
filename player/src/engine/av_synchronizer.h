//
// Created by 刘继玺 on 25-3-1.
//

#ifndef AV_SYNCHRONIZER_H
#define AV_SYNCHRONIZER_H
#include <memory>
#include <queue>
#include <thread>
#include <iostream>

#include "../core/sync_notifier.h"
#include "../define/Iaudio_samples.h"
#include "../define/Ivideo_frame.h"

namespace jxplay {
class AVSynchronizer{
public:
    struct Listener {
        virtual void OnAVSynchronizerNotifyAudioSamples(std::shared_ptr<jxplay::IAudioSamples> audio_samples) = 0;
        virtual void OnAVSynchronizerNotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) = 0;
        virtual void OnAVSynchronizerNotifyAudioFinished() = 0;
        virtual void OnAVSynchronizerNotifyVideoFinished() = 0;
        virtual ~Listener() = default;
    };

    explicit AVSynchronizer(GLContext& gl_context);
    ~AVSynchronizer();

    void SetListener(Listener* listener);
    void Start();
    void Stop();
    void Reset();

    void NotifyAudioSamples(std::shared_ptr<IAudioSamples> audio_samples);
    void NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame);
    void NotifyAudioFinished();
    void NotifyVideoFinished();
private:
    void ThreadLoop();
    void Synchronize();
    void CleanAudioQueue();
    void CleanVideoQueue();

    struct StreamInfo {
        float current_time_stamp = 0.0f;
        bool is_finished = false;
        void Reset() {
            current_time_stamp = 0.0f;
            is_finished = false;
        }
    };

    const double sync_threshold = 0.05; // 同步间隔50ms
    GLContext gl_context_;

    SyncNotifier notifier_;

    std::mutex queue_mutex_;
    std::queue<std::shared_ptr<IAudioSamples>> audio_queue_;
    std::queue<std::shared_ptr<IVideoFrame>> video_queue_;

    StreamInfo audio_stream_info_;
    StreamInfo video_stream_info_;

    std::recursive_mutex listener_mutex_;
    Listener* listener_;

    std::thread sync_thread_;
    std::atomic<bool> abort_ = false;
    std::atomic<bool> reset_= false;

};
}


#endif //AV_SYNCHRONIZER_H
