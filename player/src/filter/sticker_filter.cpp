//
// Created by 刘继玺 on 25-3-4.
//

#include "sticker_filter.h"

#include <iostream>

#include "../utils/gl_utils.h"

namespace jxplay {
StickerFilter::StickerFilter() {
    description_.type = VideoFilterType::kSticker;
    description_.vertex_shader_source = R"(
        #version 330 core
        layout(location = 0) in vec2 a_position;
        layout(location = 1) in vec2 a_texCoord;

        out vec2 v_texCoord;
        uniform mat4 u_model;
        uniform bool u_isSticker;
        uniform bool u_isKeyPoint;

        void main() {
            if (u_isKeyPoint) {
                // 如果是关键点渲染，则只考虑位置，不需要纹理坐标
                gl_Position = u_model * vec4(a_position, 0.0, 1.0);
            } else {
                // 普通纹理渲染
                if (u_isSticker) {
                    gl_Position = u_model * vec4(a_position, 0.0, 1.0);
                    v_texCoord = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
                } else {
                    gl_Position = vec4(a_position, 0.0, 1.0);
                    v_texCoord = a_texCoord;
                }
            }
        }
    )";

    description_.fragment_shader_source = R"(
        #version 330 core
        in vec2 v_texCoord;
        out vec4 FragColor;

        uniform sampler2D u_texture;
        uniform sampler2D u_sticker;
        uniform bool u_isSticker;
        uniform bool u_isKeyPoint;
        uniform vec4 u_color;  // 用于控制关键点的颜色

        void main() {
            if (u_isKeyPoint) {
                // 如果是关键点渲染，直接输出 u_color，默认为绿色
                FragColor = u_color;
            } else {
                // 普通的纹理渲染
                vec4 color = texture(u_isSticker ? u_sticker : u_texture, v_texCoord);
                if (u_isSticker && color.a < 0.1)
                    discard;
                FragColor = color;
            }
        }
    )";

}

StickerFilter::~StickerFilter() {
    if (sticker_texture_) {
        glDeleteTextures(1, &sticker_texture_);
        sticker_texture_ = 0;
    }
    HFReleaseInspireFaceSession(session_);
}

void StickerFilter::SetString(const std::string &name, const std::string &value) {
    VideoFilter::SetString(name, value);
    if (name == "StickerPath") {
        sticker_path_ = value;
    }
    if (name == "ModelPath") {
        model_path_ = value;
    }
}

void StickerFilter::Initialize() {
    if (initialized_) {
        return;
    }

    VideoFilter::Initialize();

    if (sticker_path_.empty()|| model_path_.empty()) {
        initialized_ = false;
        return;
    }
    HFSetLogLevel(HF_LOG_NONE);
    if (!LoadFaceDetectionModel(model_path_)) {
        initialized_ = false;
        return;
    }

    sticker_texture_ = GLUtils::LoadImageFileToTexture(sticker_path_,sticker_texture_width_,sticker_texture_height_);
    u_sticker_texture_location_ = glGetUniformLocation(shader_program_, "u_sticker");
    u_model_matrix_location_ = glGetUniformLocation(shader_program_, "u_model");
    u_is_sticker_location_ = glGetUniformLocation(shader_program_, "u_isSticker");
    u_is_key_point_location_ = glGetUniformLocation(shader_program_, "u_isKeyPoint");
}

bool StickerFilter::MainRender(std::shared_ptr<IVideoFrame> frame, unsigned int output_texture) {
    if (!DetectFaces(frame)) {
        return false;
    }

    // 1. 渲染原始视频帧
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frame->texture_id);
    glUniform1i(u_texture_location_, 0);

    glUniform1i(u_is_sticker_location_, GL_FALSE);   // 渲染原始视频帧
    glUniform1i(u_is_key_point_location_, GL_FALSE);  // 设置为关键点模式
    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);  // 使用全屏渲染原始帧
    glBindVertexArray(0);

    // 2. 渲染贴纸
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sticker_texture_);
    glUniform1i(u_sticker_texture_location_, 1);

    // 渲染每个检测到的人脸的贴纸
    for (const auto& face : detected_faces_) {
        // 渲染贴纸
        glm::mat4 model = CalculateStickerModelMatrix(face.left_eye, face.right_eye, face.roll, face.yaw, face.pitch);
        glUniformMatrix4fv(u_model_matrix_location_, 1, GL_FALSE, glm::value_ptr(model));

        glUniform1i(u_is_sticker_location_, GL_TRUE);    // 开始渲染贴纸
        glUniform1i(u_is_key_point_location_, GL_FALSE);  // 设置为关键点模式
        glBindVertexArray(VAO_);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);  // 缩放并定位贴纸
        glBindVertexArray(0);

// #if 0
        // 3. 渲染人脸关键点（使用绿色）
        glUniform4f(glGetUniformLocation(shader_program_, "u_color"), 0.0f, 1.0f, 0.0f, 1.0f);

        // 渲染关键点
        for (const auto& point : face.key_points) {
            glm::vec2 pos = point;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f));
            model = glm::scale(model, glm::vec3(0.005f, 0.003f, 1.0f));  // 适当缩放关键点大小

            glUniformMatrix4fv(u_model_matrix_location_, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(u_is_key_point_location_, GL_TRUE);  // 设置为关键点模式
            glBindVertexArray(VAO_);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);  // 绘制关键点
            glBindVertexArray(0);
        }
// #endif
    }

    return true;

}

glm::mat4 StickerFilter::CalculateStickerModelMatrix(const glm::vec2 &left_eye, const glm::vec2 &right_eye, float roll, float yaw, float pitch) {
    // 1. 计算左右眼之间的中心点作为脸部中心
    glm::vec2 face_center_2D = (left_eye + right_eye) * 0.5f;

    // 2. 估计眼睛中心到额头的相对位置
    float eye_distance = glm::distance(left_eye, right_eye);
    glm::vec3 relative_forehead_pos(0.0f, eye_distance * 2.5f, 0.0f);  // 额头位于眼睛中心上方 两眼之间距离乘2.5
    // glm::vec3 relative_forehead_pos(0.0f, 0.0f, 0.0f);  // 额头位于眼睛中心上方 两眼之间距离乘2.5

    // 3. 将相对额头位置应用旋转矩阵，得到旋转后的三维位置
    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));    // 绕Y轴
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));  // 绕X轴
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(-roll), glm::vec3(0.0f, 0.0f, 1.0f));  // 绕Z轴

    // 4. 计算额头的绝对位置
    glm::vec3 forehead_pos_3D = glm::vec3(rotation_matrix * glm::vec4(relative_forehead_pos, 1.0f));
    glm::vec3 face_center_3D = glm::vec3(face_center_2D, 0.0f);
    glm::vec3 final_forehead_pos = face_center_3D + forehead_pos_3D;

    // 5. 初始化模型矩阵
    glm::mat4 model_matrix = glm::mat4(1.0f);

    // 6. 将贴纸平移到额头位置
    model_matrix = glm::translate(model_matrix, final_forehead_pos);

    // 7. 应用旋转（已经通过旋转矩阵包含在内）
    model_matrix *= rotation_matrix;

    // 8. 缩放，确保贴纸的大小符合脸部比例
    float scale_factor = eye_distance * 2.5f;  // 缩放比例，具体可调
    model_matrix = glm::scale(model_matrix, glm::vec3(scale_factor, scale_factor, scale_factor));

    return model_matrix;

}

bool StickerFilter::LoadFaceDetectionModel(const std::string &model_path) {
    if (model_path.empty()) {
        return false;
    }
    if (HFLaunchInspireFace(model_path_.c_str())!=HSUCCEED) {
        std::cerr << "Failed to load face detection model" << std::endl;
        return false;
    }
    HOption option = HF_ENABLE_QUALITY | HF_ENABLE_MASK_DETECT | HF_ENABLE_INTERACTION;
    HFDetectMode det_mode = HF_DETECT_MODE_TRACK_BY_DETECTION;
    HInt32 max_metect_num = 2;
    HInt32 detect_pixel_level = 320;
    HInt32 track_by_detect_fps = 20;
    if (HFCreateInspireFaceSessionOptional(option, det_mode, max_metect_num, detect_pixel_level, track_by_detect_fps,
                                           &session_) != HSUCCEED) {
        std::cout << "Create FaceContext error: " << std::endl;
        return false;
    }

    HFSessionSetTrackPreviewSize(session_, detect_pixel_level);
    HFSessionSetFilterMinimumFacePixelSize(session_, 0);

    return true;
}

bool StickerFilter::DetectFaces(std::shared_ptr<IVideoFrame> frame) {
    if (!frame || !frame->data) {
        return false;
    }

    HFImageData image_param = {0};
    image_param.data = frame->data.get();
    image_param.width = frame->width;
    image_param.height = frame->height;
    image_param.rotation = HF_CAMERA_ROTATION_0;
    image_param.format = HF_STREAM_RGBA;

    HFImageStream image_handle = {0};
    if (HFCreateImageStream(&image_param, &image_handle) != HSUCCEED) {
        std::cout << "Create ImageStream error: " << std::endl;
        return false;
    }

    HFMultipleFaceData multiple_face_data = {0};
    if (HFExecuteFaceTrack(session_, image_handle, &multiple_face_data) != HSUCCEED) {
        std::cout << "Execute HFExecuteFaceTrack error: " << std::endl;
        HFReleaseImageStream(image_handle);
        return false;
    }

    detected_faces_.clear();

    for (int index = 0; index < multiple_face_data.detectedNum; ++index) {
        HInt32 num_of_lmk;
        HFGetNumOfFaceDenseLandmark(&num_of_lmk);
        HPoint2f denseLandmarkPoints[num_of_lmk];
        if (HFGetFaceDenseLandmarkFromFaceToken(multiple_face_data.tokens[index], denseLandmarkPoints, num_of_lmk) !=
            HSUCCEED) {
            std::cerr << "HFGetFaceDenseLandmarkFromFaceToken error!!" << std::endl;
            HFReleaseImageStream(image_handle);
            return false;
        }

        std::vector<glm::vec2> points;
        for (int i = 0; i < num_of_lmk; ++i) {
            // 坐标转换，映射到[-1.0, 1.0]的OpenGL坐标系
            points.emplace_back((denseLandmarkPoints[i].x / frame->width) * 2.0f - 1.0f,
                                1.0f - (denseLandmarkPoints[i].y / frame->height) * 2.0f);
        }

        // 转换左眼和右眼坐标
        glm::vec2 leftEye = glm::vec2((denseLandmarkPoints[105].x / frame->width) * 2.0f - 1.0f,
                                      1.0f - (denseLandmarkPoints[105].y / frame->height) * 2.0f);
        glm::vec2 rightEye = glm::vec2((denseLandmarkPoints[55].x / frame->width) * 2.0f - 1.0f,
                                       1.0f - (denseLandmarkPoints[55].y / frame->height) * 2.0f);

        detected_faces_.push_back(DetectedFaceInfo(leftEye, rightEye, multiple_face_data.angles.roll[index],
                                                   multiple_face_data.angles.yaw[index],
                                                   multiple_face_data.angles.pitch[index], points));
    }

    if (HFReleaseImageStream(image_handle) != HSUCCEED) {
        std::cout << "Release ImageStream error: " << std::endl;
    }
    return multiple_face_data.detectedNum > 0;
}

}
