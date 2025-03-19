//
// Created by 刘继玺 on 25-2-27.
//

#ifndef IVIDEO_DECODER_H
#define IVIDEO_DECODER_H

#include "../../define/Iav_packet.h"
#include "../../define/Ivideo_frame.h"

struct AVStream;

namespace jxplay{
class IVideoDecoder{
public:
    struct Listener {
        virtual void OnNotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) = 0;
        virtual ~Listener() = default;
    };
    virtual void SetListener(Listener* listener) = 0;
    virtual void SetStream(AVStream* stream) = 0;
    virtual void Decode(std::shared_ptr<IAVPacket> packet) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Pause() = 0;

    virtual int GetVideoWidth() = 0;
    virtual int GetVideoHeight() = 0;

    virtual ~IVideoDecoder() = default;

    static IVideoDecoder* Create();
};
}
#endif //IVIDEO_DECODER_H
