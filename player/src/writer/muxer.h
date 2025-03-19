//
// Created by 刘继玺 on 25-3-4.
//

#ifndef MUXER_H
#define MUXER_H

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

#include "Interface/IMuxer.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

namespace jxplay {
class Muxer : public IMuxer{
public:
   Muxer() = default;
   ~Muxer();

   bool Configure(const std::string& output_file_path, FileWriterParameters& parameters, int flags) override;
   void NotifyAudioPacket(std::shared_ptr<IAVPacket> packet) override;
   void NotifyVideoPacket(std::shared_ptr<IAVPacket> packet) override;
   void NotifyAudioFinished() override;
   void NotifyVideoFinished() override;

private:
   void WriteInterleavedPacket();

private:
   struct StreamInfo {
      AVStream* av_stream = nullptr;
      bool is_extra_data_written = false;
      bool is_finished = false;
      std::queue<std::shared_ptr<IAVPacket>> packets_queue;
   };

   std::mutex muxer_mutex_;
   AVFormatContext* format_context_ = nullptr;

   StreamInfo audio_stream_info_;
   StreamInfo video_stream_info_;

};
}


#endif //MUXER_H
