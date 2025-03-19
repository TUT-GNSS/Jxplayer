//
// Created by 刘继玺 on 25-3-8.
//
#include "pause_filter.h"

#include <iostream>

#include "../utils/gl_utils.h"

namespace jxplay {
PauseFilter::PauseFilter() {
    description_.type = VideoFilterType::kPause;
    // 修改后的顶点着色器
    description_.vertex_shader_source= R"(
    #version 330 core
    layout(location = 0) in vec2 a_position;
    layout(location = 1) in vec2 a_texCoord;

    out vec2 v_texCoord;
    uniform bool u_is_pause;
    void main() {
        if(u_is_pause){
            gl_Position = vec4(a_position, 0.0, 1.0);
            v_texCoord = a_texCoord;
        }else{
            gl_Position = vec4(a_position, 0.0, 1.0);
            v_texCoord = a_texCoord;
        }
    }
)";

    description_.fragment_shader_source= R"(
    #version 330 core
    in vec2 v_texCoord;
    out vec4 FragColor;

    uniform sampler2D u_texture;
    uniform sampler2D u_pause;
    uniform bool u_is_pause;
    void main() {
        vec4 color =texture(u_is_pause ? u_pause : u_texture, v_texCoord);
        if (u_is_pause && color.a < 0.1)
            discard;
        FragColor = color;
    }
)";
}

PauseFilter::~PauseFilter() {
    if (pause_texture_) {
        glDeleteTextures(1, &pause_texture_);
        pause_texture_ = 0;
    }
}

void PauseFilter::SetString(const std::string &name, const std::string &value) {
    VideoFilter::SetString(name, value);
    if (name == "PausePath") {
        pause_path_ = value;
    }
}

void PauseFilter::Initialize() {
    if (initialized_) {
        return;
    }

    VideoFilter::Initialize();

    if (pause_path_.empty()) {
        initialized_ = false;
        return;
    }
    pause_texture_ = GLUtils::LoadImageFileToTexture(pause_path_,pause_texture_width_,pause_texture_height_);
    u_pause_texture_location_ = glGetUniformLocation(shader_program_, "u_pause");
    u_is_pause_location_ = glGetUniformLocation(shader_program_, "u_is_pause");
}

bool PauseFilter::MainRender(std::shared_ptr<IVideoFrame> frame, unsigned int output_texture) {

    // 1. 渲染原始视频帧
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frame->texture_id);
    glUniform1i(u_texture_location_, 0);

    glUniform1i(u_is_pause_location_,GL_FALSE); // 渲染原始视频帧
    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);  // 使用全屏渲染原始帧
    glBindVertexArray(0);

    // 2.渲染暂停贴纸
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pause_texture_);
    glUniform1i(u_pause_texture_location_, 1);

    glUniform1i(u_is_pause_location_,GL_TRUE);
    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // 缩放并定位贴纸
    glBindVertexArray(0);

    return true;

}
}