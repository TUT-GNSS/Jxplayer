//
// Created by 刘继玺 on 25-3-1.
//

#ifndef IPLAYER_H
#define IPLAYER_H


#include <memory>

#include "Igl_context.h"
#include "Iplayback_listener.h"
#include "Ivideo_filter.h"

namespace jxplay{

class IVideoDisplayView;

class IPlayer {
public:
        virtual void AttachDisplayView(std::shared_ptr<IVideoDisplayView> display_view) = 0;
        virtual void DetachDisplayView(std::shared_ptr<IVideoDisplayView> display_view) = 0;

        virtual void SetPlaybackListener(std::shared_ptr<IPlaybackListener> listener) = 0;

        virtual bool Open(std::string &file_path) = 0;
        virtual void Play() = 0;
        virtual void Pause() = 0;
        virtual void SeekTo(float position) = 0;
        virtual bool IsPlaying() = 0;

        virtual std::shared_ptr<IVideoFilter> AddVideoFilter(VideoFilterType type) = 0;
        virtual void RemoveVideoFilter(VideoFilterType type) = 0;

        virtual bool StartRecording(const std::string &output_file_path, int flags) = 0;
        virtual void StopRecording() = 0;
        virtual bool IsRecording() = 0;

        virtual void SetPlaybackSpeed(float speed) = 0;

        virtual ~IPlayer() = default;

        static IPlayer *Create(GLContext gl_context);
};

}  // namespace av

#endif //IPLAYER_H
