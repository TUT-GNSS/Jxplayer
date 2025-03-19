//
// Created by 刘继玺 on 25-3-3.
//

#ifndef VIDEO_FILTER_H
#define VIDEO_FILTER_H
#include <string>
#include <unordered_map>

#include "../define/Ivideo_frame.h"
#include "../../include/Ivideo_filter.h"

namespace jxplay {
class VideoFilter: public IVideoFilter{
public:
    VideoFilter() = default;
    ~VideoFilter();

    VideoFilterType GetType() const override { return description_.type; }

    void SetFloat(const std::string& name, float value) override;
    float GetFloat(const std::string& name) override;

    void SetInt(const std::string& name, int value) override;
    int GetInt(const std::string& name) override;

    void SetString(const std::string& name, const std::string& value) override;
    std::string GetString(const std::string& name) override;

    bool Render(std::shared_ptr<IVideoFrame> frame, unsigned int output_texture);

    static VideoFilter* Create(VideoFilterType type);

protected:
    virtual void Initialize();
    virtual bool MainRender(std::shared_ptr<IVideoFrame> frame, unsigned int output_texture);
    bool PreRender(std::shared_ptr<IVideoFrame> frame, unsigned int output_texture);
    bool PostRender();
    int GetUniformLocation(const std::string& name);

protected:
    struct VideoFilterDescription {
        VideoFilterType type{VideoFilterType::kNone};
        const char* vertex_shader_source{nullptr};
        const char* fragment_shader_source{nullptr};
    };

protected:
    VideoFilterDescription description_;
    bool initialized_ = false;
    unsigned int shader_program_ = 0;
    unsigned int VAO_ = 0;
    unsigned int VBO_ = 0;
    int u_texture_location_;
    std::unordered_map<std::string, int> uniform_location_cache_;
    std::unordered_map<std::string, float> float_values_;
    std::unordered_map<std::string, int> int_values_;
    std::unordered_map<std::string, std::string> string_values_;
};
}


#endif //VIDEO_FILTER_H
