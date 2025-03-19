//
// Created by 刘继玺 on 25-3-3.
//

#include "flip_vertical_filter.h"

namespace jxplay {

FlipVerticalFilter::FlipVerticalFilter() {
    description_.type = VideoFilterType::kFlipVertical;
    description_.vertex_shader_source= R"(
        #version 330 core
        layout(location = 0) in vec2 a_position;
        layout(location = 1) in vec2 a_texCoord;
        out vec2 v_texCoord;
        void main() {
            gl_Position = vec4(a_position, 0.0, 1.0);
            v_texCoord = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
        }
    )";

    description_.fragment_shader_source= R"(
        #version 330 core
        in vec2 v_texCoord;
        out vec4 FragColor;
        uniform sampler2D u_texture;
        void main() {
            FragColor = texture(u_texture, v_texCoord);
        }
    )";
}

}