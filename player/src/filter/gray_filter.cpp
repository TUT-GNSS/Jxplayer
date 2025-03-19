//
// Created by 刘继玺 on 25-3-3.
//

#include "gray_filter.h"

namespace jxplay {

GrayFilter::GrayFilter() {
    description_.type = VideoFilterType::kGray;
    description_.vertex_shader_source= R"(
        #version 330 core
        layout(location = 0) in vec2 a_position;
        layout(location = 1) in vec2 a_texCoord;
        out vec2 v_texCoord;
        void main() {
            gl_Position = vec4(a_position, 0.0, 1.0);
            v_texCoord = a_texCoord;
        }
    )";
    description_.fragment_shader_source= R"(
        #version 330 core
        in vec2 v_texCoord;
        out vec4 FragColor;
        uniform sampler2D u_texture;
        void main() {
            vec4 color = texture(u_texture, v_texCoord);
            float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
            FragColor = vec4(gray, gray, gray, color.a);
        }
    )";
}

}

