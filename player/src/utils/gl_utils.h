//
// Created by 刘继玺 on 25-3-2.
//

#ifndef GL_UTILS_H
#define GL_UTILS_H

#include "../../include/Igl_context.h"

namespace jxplay{

struct GLUtils {
    static unsigned int CompileAndLinkProgram(const char* vertex_shader_source, const char* fragment_shader_source);
    static bool CheckGLError(const char* tag = "");
    static GLuint GenerateTexture(int width, int height, GLenum internal_format, GLenum format);
    static GLuint LoadImageFileToTexture(const std::string& image_path, int& width, int& height);
};

}  // namespace av



#endif //GL_UTILS_H
