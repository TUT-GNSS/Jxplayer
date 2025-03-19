//
// Created by 刘继玺 on 25-3-2.
//

#ifndef IVIDEO_DISPLAY_VIEW_H
#define IVIDEO_DISPLAY_VIEW_H
#include "../define/Ivideo_frame.h"
#include "../../include/Igl_context.h"
#include "../core/task_pool.h"

namespace jxplay{

class IVideoDisplayView {
public:
    enum class EContentMode {
        kScaleToFill = 0,
        kScaleAspectFit,
        kScaleAspectFill
    };

    virtual void SetDisplaySize(int width, int height) = 0;
    virtual void SetTaskPool(std::shared_ptr<TaskPool> task_pool) = 0;
    virtual void InitializeGL() = 0;
    virtual void Render(std::shared_ptr<IVideoFrame> video_frame, EContentMode mode) = 0;
    virtual void Render(int width, int height, float red, float green, float blue) = 0;
    virtual void Clear() = 0;

    virtual ~IVideoDisplayView() = default;

    static IVideoDisplayView* Create();
};

}
#endif //IVIDEO_DISPLAY_VIEW_H
