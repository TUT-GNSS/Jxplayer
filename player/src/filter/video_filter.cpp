//
// Created by 刘继玺 on 25-3-3.
//

#include "video_filter.h"

#include <iostream>
#include "player/src/utils/gl_utils.h"
#include "flip_vertical_filter.h"
#include "gray_filter.h"
#include "invert_filter.h"
#include "sticker_filter.h"
#include "pause_filter.h"


namespace jxplay {
VideoFilter::~VideoFilter() {
    glDeleteProgram(shader_program_);
    glDeleteVertexArrays(1,&VAO_);
    glDeleteBuffers(1,&VBO_);
}

void VideoFilter::SetFloat(const std::string &name, float value) {
    float_values_[name] = value;
}
float VideoFilter::GetFloat(const std::string &name) {
    return float_values_[name];
}

void VideoFilter::SetInt(const std::string &name, int value) {
    int_values_[name] = value;
}
int VideoFilter::GetInt(const std::string &name) {
    return int_values_[name];
}

void VideoFilter::SetString(const std::string &name, const std::string &value) {
    string_values_[name] = value;
}
std::string VideoFilter::GetString(const std::string &name) {
    return string_values_[name];
}

bool VideoFilter::Render(std::shared_ptr<IVideoFrame> frame, unsigned int output_texture) {
    if (!PreRender(frame, output_texture)) {
        return false;
    }
    if (!MainRender(frame, output_texture)) {
        return false;
    }
    return PostRender();
}

bool VideoFilter::PreRender(std::shared_ptr<IVideoFrame> frame, unsigned int output_texture) {
    Initialize();
    if (!initialized_) {
        return false;
    }

    // 帧缓冲对象FBO配置 bind FBO 和 attach输出纹理
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output_texture, 0);
    // 检查FBO完整性
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER::Framebuffer is not complete!"<<glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
        return false;
    }

    // 渲染配置
    glViewport(0, 0, frame->width, frame->height); // 设置窗口与视频帧尺寸匹配
    glClearColor(0.0, 0.0, 0.0, 0.0); // RGBA全为0表示透明黑
    glClear(GL_COLOR_BUFFER_BIT); // 清除颜色缓冲
    glUseProgram(shader_program_); // 激活着色器程序 准备执行着色器渲染
    return true;
}

bool VideoFilter::MainRender(std::shared_ptr<IVideoFrame> frame, unsigned int outputTexture) {
    // 绑定输入纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frame->texture_id);
    if (u_texture_location_== -1) {
        std::cerr << "ERROR: uniform 'u_texture' not found or inactive!" << std::endl;
    } else {
        glUniform1i(u_texture_location_, 0);
    }

    // Render quad
    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    return true;
}

bool VideoFilter::PostRender() {
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void VideoFilter::Initialize() {
    if (initialized_) {
       return;
    }
    initialized_ = true;

    // 编译并链接着色器
    shader_program_ = GLUtils::CompileAndLinkProgram(description_.vertex_shader_source,description_.fragment_shader_source);

    // 全屏四边形顶点数据定义
    float vertices[] = {
        // 位置坐标(XY)   纹理坐标(UV)
        -1.0f, 1.0f,    0.0f, 1.0f,  // 左上
        -1.0f, -1.0f,   0.0f, 0.0f,  // 左下
        1.0f,  -1.0f,   1.0f, 0.0f,  // 右下
        1.0f,  1.0f,    1.0f, 1.0f   // 右上
    }; // 顶点按逆时针顺序排列，适合GL_TRIANGLE_FAN模式

    // 顶点缓冲对象VBO 和 顶点数组对象VAO设置
    glGenVertexArrays(1,&VAO_);
    glGenBuffers(1,&VBO_);
    //绑定VAO 后续操作自动记录属性配置
    glBindVertexArray(VAO_);
    //配置VBO：绑定+上传数据
    glBindBuffer(GL_ARRAY_BUFFER,VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 顶点属性配置
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); // 使能通道0

    // 纹理坐标配置
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1); // 使能通道1

    // 解绑VAO
    glBindVertexArray(0);

    // 获取着色器中Uniform位置
    u_texture_location_= glGetUniformLocation(shader_program_, "u_texture");
}

int VideoFilter::GetUniformLocation(const std::string &name) {
    auto [it,inserted] = uniform_location_cache_.try_emplace(name,glGetUniformLocation(shader_program_, name.c_str()));
    if (it->second == -1) {
        std::cerr << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;
    }
    return it->second;
}

VideoFilter* VideoFilter::Create(VideoFilterType type) {
    switch (type) {
        case VideoFilterType::kFlipVertical:
            return new FlipVerticalFilter();
        case VideoFilterType::kGray:
            return new GrayFilter();
        case VideoFilterType::kInvert:
            return new InvertFilter();
        case VideoFilterType::kSticker:
            return new StickerFilter();
        case VideoFilterType::kPause:
            return new PauseFilter();
        default:
            break;
    }

    return nullptr;
}



}