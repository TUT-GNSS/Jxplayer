//
// Created by 刘继玺 on 25-3-4.
//

#ifndef IMUXER_H
#define IMUXER_H

#include <string>

#include "../../define/file_writer_parameters.h"
#include "../../define/Iav_packet.h"

namespace jxplay{
class IMuxer{
public:
    virtual bool Configure(const std::string& output_file_path, FileWriterParameters& parameters, int flags) = 0;
    virtual void NotifyAudioPacket(std::shared_ptr<IAVPacket> packet) = 0;
    virtual void NotifyVideoPacket(std::shared_ptr<IAVPacket> packet) = 0;
    virtual void NotifyAudioFinished() = 0;
    virtual void NotifyVideoFinished() = 0;

    virtual ~IMuxer() = default;

    static IMuxer* Create();
};
};

#endif //IMUXER_H
