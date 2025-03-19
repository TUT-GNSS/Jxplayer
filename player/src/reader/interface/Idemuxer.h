//
// Created by 刘继玺 on 25-2-26.
//

#ifndef IDEMUXER_H
#define IDEMUXER_H
#include <QtWidgets/QWidget>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "../../define/Iav_packet.h"

namespace jxplay {
struct IDeMuxer {
    struct Listener {
        virtual void OnDeMuxStart() = 0;
        virtual void OnDeMuxStop() = 0;
        virtual void OnDeMuxEOF() = 0;
        virtual void OnDeMuxError(int code, const char* msg) = 0;

        virtual void OnNotifyAudioStream(AVStream* stream) = 0;
        virtual void OnNotifyVideoStream(AVStream* stream) = 0;

        virtual void OnNotifyAudioPacket(std::shared_ptr<IAVPacket> packet) = 0;
        virtual void OnNotifyVideoPacket(std::shared_ptr<IAVPacket> packet) = 0;

        virtual ~Listener() = default;
    };

    virtual void SetListener(Listener* listener) = 0;

    virtual bool Open(const std::string& url) = 0;
    virtual void Start() = 0;
    virtual void Pause() = 0;
    virtual void SeekTo(float position) = 0;
    virtual void Stop() = 0;

    virtual float GetDuration() = 0;

    virtual ~IDeMuxer() = default;

    static IDeMuxer* Create();
};
}

#endif //IDEMUXER_H
