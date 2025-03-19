//
// Created by 刘继玺 on 25-3-2.
//

#include "video_display_view.h"

#include <iostream>

#include "../core/sync_notifier.h"
#include "../core/task_pool.h"
#include "../utils/gl_utils.h"

namespace jxplay {
// 顶点着色器
static const char* vertex_shader_source = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main()
    {
        gl_Position = vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

// 线段着色器
static const char* fragment_shader_source = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;

    uniform sampler2D texture1;

    void main()
    {
        FragColor = texture(texture1, TexCoord);
    }
)";

IVideoDisplayView* IVideoDisplayView::Create() {
    return new VideoDisplayView();
}

VideoDisplayView::~VideoDisplayView() {
    Clear();
}

void VideoDisplayView::SetTaskPool(std::shared_ptr<TaskPool> task_pool) {
    task_pool_ = task_pool;
}

void VideoDisplayView::InitializeGL() {
    shader_program_ = GLUtils::CompileAndLinkProgram(vertex_shader_source, fragment_shader_source);
    float vertices[] = {
        // positions         // texture coords
        1.0f,  1.0f,  0.0f,   1.0f, 1.0f,
        1.0f,  -1.0f, 0.0f,   1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
        -1.0f, 1.0f,  0.0f,   0.0f, 1.0f};

    unsigned int indices[] = {0, 1, 3, 1, 2, 3};

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    glGenBuffers(1, &EBO_);

    glBindVertexArray(VAO_);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void VideoDisplayView::SetDisplaySize(int width, int height) {
    glViewport(0, 0, width, height);
}

void VideoDisplayView::Render(std::shared_ptr<IVideoFrame> video_frame, EContentMode mode) {
    if (!video_frame) {return;}

    std::lock_guard<std::mutex> lock(video_frame_mutex_);
    video_frame_ = video_frame;
    mode_ = mode;
}

void VideoDisplayView::Render(int width, int height, float red, float green, float blue) {
    std::lock_guard<std::mutex> lock(video_frame_mutex_);

    glClearColor(red, green, blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!video_frame_) return;

    if (!video_frame_->texture_id) {
        glGenTextures(1, &video_frame_->texture_id);
        glBindTexture(GL_TEXTURE_2D, video_frame_->texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, video_frame_->width, video_frame_->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     video_frame_->data.get());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glBindTexture(GL_TEXTURE_2D, video_frame_->texture_id);
    }

    glUseProgram(shader_program_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, video_frame_->texture_id);

    glBindVertexArray(VAO_);

    switch (mode_) {
        case EContentMode::kScaleToFill:
            glViewport(0, 0, width, height);
            break;
        case EContentMode::kScaleAspectFit: {
            float aspect_ratio = static_cast<float>(video_frame_->width) / video_frame_->height;
            float screen_aspect_ratio = static_cast<float>(width) / height;
            if (aspect_ratio > screen_aspect_ratio) {
                int new_height = static_cast<int>(width / aspect_ratio);
                glViewport(0, (height - new_height) / 2, width, new_height);
            } else {
                int new_width = static_cast<int>(height * aspect_ratio);
                glViewport((width - new_width) / 2, 0, new_width, height);
            }
            break;
        }
        case EContentMode::kScaleAspectFill: {
            float aspect_ratio = static_cast<float>(video_frame_->width) / video_frame_->height;
            float screen_aspect_ratio = static_cast<float>(width) / height;
            if (aspect_ratio > screen_aspect_ratio) {
                int new_width = static_cast<int>(height * aspect_ratio);
                glViewport((width - new_width) / 2, 0, new_width, height);
            } else {
                int new_height = static_cast<int>(width / aspect_ratio);
                glViewport(0, (height - new_height) / 2, width, new_height);
            }
            break;
        }
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoDisplayView::Clear() {
    SyncNotifier notifier;
    if (task_pool_) {
        task_pool_->SubmitTask([&]() {
            if (shader_program_ > 0) glDeleteProgram(shader_program_);
            std::lock_guard<std::mutex> lock(video_frame_mutex_);
            video_frame_ = nullptr;
            notifier.Notify();
        });
    }
    notifier.Wait();
}



}