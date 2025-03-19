//
// Created by 刘继玺 on 25-3-4.
//

#ifndef STICKER_FILTER_H
#define STICKER_FILTER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../../third_party/InspireFace/include/inspireface.h"
#include "video_filter.h"


namespace jxplay {
class StickerFilter : public VideoFilter{
public:
    StickerFilter();
    ~StickerFilter() override;
    void SetString(const std::string &name, const std::string &value) override;
    bool MainRender(std::shared_ptr<IVideoFrame> frame, unsigned int output_texture) override;
    void Initialize() override;

private:
    bool LoadFaceDetectionModel(const std::string &model_path);
    bool DetectFaces(std::shared_ptr<IVideoFrame> frame);
    glm::mat4 CalculateStickerModelMatrix(const glm::vec2& left_eye, const glm::vec2& right_eye, float roll, float yaw,
                                          float pitch);
private:
    struct DetectedFaceInfo{
        const glm::vec2 left_eye;
        const glm::vec2 right_eye;
        float roll;
        float yaw;
        float pitch;

        std::vector<glm::vec2> key_points;

        DetectedFaceInfo(const glm::vec2& left_eye, const glm::vec2& right_eye,float roll,float yaw, float pitch,
            const std::vector<glm::vec2>& key_points):left_eye(left_eye),right_eye(right_eye),roll(roll),yaw(yaw),
            pitch(pitch),key_points(key_points){}
    };

    std::string sticker_path_;
    std::string model_path_;
    unsigned int sticker_texture_ = 0;
    int sticker_texture_width_ = 0;
    int sticker_texture_height_ = 0;
    int u_sticker_texture_location_ = 0;
    int u_model_matrix_location_ = 0;
    int u_is_sticker_location_ = 0;
    int u_is_key_point_location_ = 0;

    // 人脸检测
    HFSession session_ = nullptr;
    std::vector<DetectedFaceInfo> detected_faces_;

};
}

#endif //STICKER_FILTER_H
