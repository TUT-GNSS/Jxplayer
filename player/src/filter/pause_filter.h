//
// Created by 刘继玺 on 25-3-8.
//

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "video_filter.h"

#ifndef PAUSE_FILTER_H
#define PAUSE_FILTER_H

namespace jxplay {
    class PauseFilter: public VideoFilter{
    public:
        PauseFilter();
        ~PauseFilter() override;
        void SetString(const std::string &name, const std::string &value) override;
        bool MainRender(std::shared_ptr<IVideoFrame> frame, unsigned int output_texture) override;
        void Initialize() override;
    private:
        std::string pause_path_ = std::string(RESOURCE_DIR) + "/pause.png";
        unsigned int pause_texture_ = 0;
        int pause_texture_width_ = 0;
        int pause_texture_height_ = 0;
        int u_pause_texture_location_ = 0;
        int u_is_pause_location_ = 0;
    };
}



#endif //PAUSE_FILTER_H

