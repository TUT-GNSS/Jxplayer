//
// Created by 刘继玺 on 25-3-4.
//

#ifndef IVIDEO_ENCODER_H
#define IVIDEO_ENCODER_H

#include "../../define/file_writer_parameters.h"
#include "../../define/Iav_packet.h"
#include "../../define/Ivideo_frame.h"

namespace jxplay {
class IVideoEncoder {
public:
    struct Listener {
        virtual void OnVideoEncoderNotifyPacket(std::shared_ptr<IAVPacket>) = 0;
        virtual void OnVideoEncoderNotifyFinished() = 0;
        virtual ~Listener() = default;
    };

    virtual void SetListener(Listener* listener) = 0;
    virtual bool Configure(FileWriterParameters& parameters, int flags) = 0;
    virtual void NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) = 0;

};
}



#endif //IVIDEO_ENCODER_H
