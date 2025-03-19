//
// Created by 刘继玺 on 25-2-25.
//

#ifndef IPLAYBACKLISTENER_H
#define IPLAYBACKLISTENER_H
#include <string>

namespace jxplay {
    struct IPlaybackListener {
        virtual void NotifyPlaybackStarted() = 0;
        virtual void NotifyPlaybackTimeChanged(float timeStamp, float duration) = 0;
        virtual void NotifyPlaybackPaused() = 0;
        virtual void NotifyPlaybackEOF() = 0;

        virtual ~IPlaybackListener() = default;
    };
};  // namespace jxplay

#endif //IPLAYBACKLISTENER_H

