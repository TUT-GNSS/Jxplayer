//
// Created by 刘继玺 on 25-3-2.
//

#ifndef IVIDEO_PIPELINE_H
#define IVIDEO_PIPELINE_H
#include <iostream>
#include "../../include/Igl_context.h"
#include "../define/Ivideo_frame.h"
#include "../../include/Ivideo_filter.h"

namespace jxplay{
class IVideoPipeline{
public:
    struct Listener{
      virtual void OnVideoPipelineNotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) = 0;
      virtual void OnVideoPipelineNotifyVideoFinished() = 0;
      virtual ~Listener() = default;
    };
    virtual std::shared_ptr<IVideoFilter> AddVideoFilter(VideoFilterType type) = 0;
    virtual void RemoveVideoFilter(VideoFilterType type) = 0;

    virtual void SetListener(Listener* listener) = 0;
    virtual void NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) = 0;
    virtual void NotifyVideoFinished() = 0;
    virtual void Stop() = 0;
    virtual ~IVideoPipeline() = default;

    static IVideoPipeline* Create(GLContext& gl_context);
};
}
#endif //IVIDEO_PIPELINE_H
