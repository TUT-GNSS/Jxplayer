//
// Created by 刘继玺 on 25-3-2.
//

#ifndef VIDEO_DISPLAY_VIEW_H
#define VIDEO_DISPLAY_VIEW_H

#include <mutex>
#include "../interface/Ivideo_display_view.h"

namespace jxplay {
class VideoDisplayView : public IVideoDisplayView{
public:
    VideoDisplayView() = default;
    ~VideoDisplayView() override;

    void SetTaskPool(std::shared_ptr<TaskPool> task_pool) override;
    void InitializeGL() override;
    void SetDisplaySize(int width, int height) override;
    void Render(std::shared_ptr<IVideoFrame> video_frame, EContentMode mode) override;
    void Render(int width, int height, float red, float green, float blue) override;
    void Clear() override;

private:
    std::shared_ptr<TaskPool> task_pool_;

    std::mutex video_frame_mutex_;
    std::shared_ptr<IVideoFrame> video_frame_;
    EContentMode mode_;

    unsigned int shader_program_ = 0;
    unsigned int VAO_= 0;
    unsigned int VBO_= 0;
    unsigned int EBO_= 0;

};
}



#endif //VIDEO_DISPLAY_VIEW_H
