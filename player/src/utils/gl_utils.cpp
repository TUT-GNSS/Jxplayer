//
// Created by 刘继玺 on 25-3-2.
//

#include "gl_utils.h"

#include <QImage>

// 注意：这里需要定义STB_IMAGE_IMPLEMENTATION，否则编译器会报错
// #define STB_IMAGE_IMPLEMENTATION
// #include <stb_image.h>

#include <iostream>

namespace jxplay {

/**
 * 编译着色器工具函数
 * @param type 着色器类型 (GL_VERTEX_SHADER / GL_FRAGMENT_SHADER)
 * @param source GLSL源代码
 * @return 编译成功的着色器ID，若失败会输出错误日志[4](@ref)
 */
static unsigned int CompileShader(unsigned int type, const char* source) {
    // 创建着色器对象并绑定源码
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // 检查编译状态
    int success;
    char info_log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << info_log << std::endl;
    }
    return shader;
}

/**
 * 编译并链接着色器程序
 * @param vertex_shader_source 顶点着色器源码
 * @param fragment_shader_source 片段着色器源码
 * @return 链接成功的着色器程序ID[4](@ref)
 */
unsigned int GLUtils::CompileAndLinkProgram(const char* vertex_shader_source, const char* fragment_shader_source) {
    // 分别编译顶点和片段着色器
    auto vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_shader_source);
    auto fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_shader_source);

    // 创建程序并附加着色器
    auto shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    // 清理临时着色器对象
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader_program;
}

/**
 * 检查OpenGL错误状态
 * @param tag 调试标识
 * @return 是否存在未处理的GL错误[4](@ref)
 */
bool GLUtils::CheckGLError(const char* tag) {
    bool noErr = true;
    for (;;) {
        GLenum error = glGetError();
        if (error == GL_NO_ERROR) return noErr;
        noErr = false;
        std::cerr << tag << " glGetError (0x" << std::hex << error << std::endl;
    }
}

/**
 * 生成空白纹理对象
 * @param width 纹理宽度
 * @param height 纹理高度
 * @param internal_format 内部存储格式 (如GL_RGBA)
 * @param format 像素数据格式 (如GL_RGBA)
 * @return 生成的纹理ID[4](@ref)
 */
GLuint GLUtils::GenerateTexture(int width, int height, GLenum internal_format, GLenum format) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // 预分配显存空间但不填充数据
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}


// GLuint GLUtils::LoadImageFileToTexture(const std::string& image_path, int& width, int& height) {
//     GLuint texture = 0;
//     int nrChannels;
//     // 加载图像数据（强制4通道：STBI_rgb_alpha）
//     unsigned char* data = stbi_load(image_path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
//     if (data) {
//         glGenTextures(1, &texture);
//         glBindTexture(GL_TEXTURE_2D, texture);
//         // 上传像素数据并生成mipmap
//         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
//         glGenerateMipmap(GL_TEXTURE_2D);
//         glBindTexture(GL_TEXTURE_2D, 0);
//         stbi_image_free(data);  // 释放CPU端图像数据[2](@ref)
//     } else {
//         std::cerr << "Failed to load texture: " << image_path << std::endl;
//     }
//     return texture;
// }

/**
 * 加载图像文件生成OpenGL纹理
 * @param image_path 图片路径（支持PNG/JPEG/BMP等格式）[1,3](@ref)
 * @param width [out] 图像宽度
 * @param height [out] 图像高度
 * @return 生成的纹理ID，失败返回0
 *
 * @note 使用stb_image库加载图像，强制转换为RGBA格式[5](@ref)
 *       如需适配OpenGL坐标系，建议调用stbi_set_flip_vertically_on_load(true)[1](@ref)
 */
GLuint GLUtils::LoadImageFileToTexture(const std::string& image_path, int& width, int& height) {
    GLuint texture = 0;

    // 使用QImage加载图像文件
    QImage img;
    if (!img.load(QString::fromStdString(image_path))) {
        std::cerr << "Failed to load texture: " << image_path << std::endl;
        return texture;
    }

    // 转换为OpenGL兼容的RGBA格式
    QImage glImage = img.convertToFormat(QImage::Format_RGBA8888);
    width = glImage.width();
    height = glImage.height();

    // 创建OpenGL纹理
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // 上传像素数据（自动处理行对齐）
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, glImage.constBits());

    // 生成mipmap并设置默认参数
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}
}