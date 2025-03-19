//
// Created by 刘继玺 on 25-2-26.
//

#ifndef FILE_WRITER_PARAMETERS_H
#define FILE_WRITER_PARAMETERS_H


namespace jxplay{
struct FileWriterParameters {
    // 音频相关参数
    int sample_rate = 44100;  // 采样率，默认值为44100 Hz
    int channels = 2;        // 通道数，默认值为2（立体声）

    // 视频相关参数
    int width = 720;    // 视频宽度，默认值为720像素
    int height =1280;  // 视频高度，默认值为1280像素
    int fps = 25;       // 帧率，默认值为30帧每秒
};
}
#endif //FILE_WRITER_PARAMETERS_H
