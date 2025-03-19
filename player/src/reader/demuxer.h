//
// Created by 刘继玺 on 25-2-26.
//

#ifndef DEMUXER_H
#define DEMUXER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "../core/sync_notifier.h"
#include "interface/Idemuxer.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace jxplay {

class DeMuxer : public IDeMuxer{
public:
    DeMuxer();
    ~DeMuxer() override;

    void SetListener(Listener* listener) override;

    bool Open(const std::string& url) override;
    void Start() override;
    void Pause() override;
    void SeekTo(float position) override;
    void Stop() override;

    float GetDuration() override;
private:
    void ThreadPool();
    void ProcessSeek();
    bool ReadAndSendPacket();

    void ReleaseAudioPipelineResource();
    void ReleaseVideoPipelineResource();
    struct StreamInfo {
        int stream_index = -1;
        std::atomic<int> pipeline_resource_count = 3;
        std::shared_ptr<std::function<void()>> pipeline_release_callback;
    };
private:
    IDeMuxer::Listener* listener_ = nullptr;
    std::mutex mutex_;
    std::recursive_mutex listener_mutex_;
    std::thread thread_;
    std::atomic<bool> pause_ = true;
    std::atomic<bool> abort_ = false;
    std::atomic<bool> seek_ = false;
    SyncNotifier notifier_;


    AVFormatContext* format_context_ = nullptr;
    std::mutex format_mutex_;
    StreamInfo audio_stream_info_;
    StreamInfo video_stream_info_;

    float seek_progress_ = -1.0f;

};

} // jxplay

#endif //DEMUXER_H
