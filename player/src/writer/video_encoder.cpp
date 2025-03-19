//
// Created by 刘继玺 on 25-3-4.
//

#include "video_encoder.h"

#include <iostream>

#include <player/src/utils/gl_utils.h>
#include <player/src/filter/video_filter.h>

namespace jxplay {
VideoEncoder::VideoEncoder(GLContext &gl_context) : shared_gl_context_(gl_context), thread_(&VideoEncoder::ThreadLoop,this) {}

VideoEncoder::~VideoEncoder() {
    StopThread();
    if (sws_context_) {
        sws_freeContext(sws_context_);
    }
    if (av_frame_) {
        av_frame_free(&av_frame_);
    }
    if (av_packet_) {
        av_packet_free(&av_packet_);
    }
    if (encode_context_) {
        avcodec_free_context(&encode_context_);
    }
}

void VideoEncoder::SetListener(Listener* listener) {
    std::lock_guard<std::mutex> lock(listener_mutex_);
    listener_ = listener;
}

// 创建并配置H.264视频编码器
bool VideoEncoder::Configure(FileWriterParameters &parameters, int flags) {
    // 1. 查找H.264编码器实现
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264); // 根据编码器ID获取AVCodec实例
    if (!codec) {
        std::cerr << "Could not find H264 codec" << std::endl;
        return false;
    }

    // 2. 分配编码器上下文
    encode_context_ = avcodec_alloc_context3(codec); // 创建与编码器关联的上下文对象
    if (!encode_context_) {
        std::cerr << "Could not allocate h264 codec context" << std::endl;
        return false;
    }

    // 3. 配置编码参数（核心参数设置参考Web API规范
    encode_context_->bit_rate = parameters.width * parameters.height * 4; // 比特率计算
    encode_context_->width = parameters.width;    // 视频宽度（单位：像素）
    encode_context_->height = parameters.height;   // 视频高度（单位：像素）
    encode_context_->time_base = {1,parameters.fps}; // 时间基准（分母为帧率）
    encode_context_->framerate = {parameters.fps, 1}; // 帧率设置（分子/分母）
    encode_context_->gop_size = 30;     // 关键帧间隔
    encode_context_->max_b_frames = 1;  // B帧最大数量
    encode_context_->pix_fmt = AV_PIX_FMT_YUV420P; // 像素格式

    // 4. 初始化编码器
    if (avcodec_open2(encode_context_, codec, nullptr) < 0) { // 使用默认参数打开编码器
        std::cerr << "Could not open encoder" << std::endl;
        return false;
    }

    // 5. 分配视频帧内存
    av_frame_ = av_frame_alloc(); // 创建AVFrame用于存储原始帧数据
    if (!av_frame_) {
        std::cerr << "Could not allocate video frame" << std::endl;
        return false;
    }
    av_frame_->format = encode_context_->pix_fmt; // 设置像素格式
    av_frame_->width = parameters.width;         // 帧宽度
    av_frame_->height = parameters.height;        // 帧高度
    av_frame_->pts = 0;                          // 初始显示时间戳

    // 6. 分配帧缓冲区
    auto ret = av_frame_get_buffer(av_frame_, 1); // 按字节对齐分配内存
    if (ret < 0) {
        std::cerr << "av_frame_get_buffer failed!" << std::endl;
        if (av_packet_) {  // 异常处理中清理已分配资源
            av_frame_free(&av_frame_);
        }
        return false;
    }

    // 7. 分配视频包内存
    av_packet_ = av_packet_alloc(); // 创建AVPacket用于存储编码后的数据包
    if (!av_packet_) {
        std::cerr << "Could not allocate video packet" << std::endl;
        return false;
    }
    return true;
}

void VideoEncoder::NotifyVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
    if (!video_frame) {
        return;
    }
    std::lock_guard<std::mutex> lock(video_frame_queue_mutex_);
    video_frame_queue_.push(video_frame);
    condition_var_.notify_one();
}

void VideoEncoder::ThreadLoop() {
    shared_gl_context_.Initialize();
    shared_gl_context_.MakeCurrent();

    unsigned int fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    while (true) {
        std::shared_ptr<IVideoFrame> video_frame;
        {
            std::unique_lock<std::mutex> lock(video_frame_queue_mutex_);
            condition_var_.wait(lock, [this]{return abort_||!video_frame_queue_.empty();});
            if (abort_) {
                break;
            }

            video_frame = video_frame_queue_.front();
            video_frame_queue_.pop();
        }
        if (video_frame->flags & static_cast<int>(AVFrameFlag::kEOS)) {
            EncodeVideoFrame(nullptr);
        }else {
            PrepareAndEncodeVideoFrame(video_frame);
        }
    }

    {
        std::unique_lock<std::mutex> lock(video_frame_queue_mutex_);
        CleanPacketsQueue();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    shared_gl_context_.DoneCurrent();
    shared_gl_context_.Destroy();
}

void VideoEncoder::StopThread() {
    abort_ = true;
    condition_var_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void VideoEncoder::FlipVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
    if (!texture_id_) {
        texture_id_ = GLUtils::GenerateTexture(encode_context_->width, encode_context_->height, GL_RGBA, GL_RGBA);
    }
    // 垂直翻转画面
    if (!flip_vertical_filter_) {
        flip_vertical_filter_ = std::shared_ptr<VideoFilter>(VideoFilter::Create(VideoFilterType::kFlipVertical));
    }
    flip_vertical_filter_->Render(video_frame, texture_id_);
}

void VideoEncoder::ConvertTextureToFrame(unsigned int texture_id, int width, int height) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);

    // 确保缓冲区大小合适
    if (rgba_data_.size() != width * height * 4) {
        rgba_data_.resize(width * height * 4);
    }

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba_data_.data());

    uint8_t* src_slice[1] = {rgba_data_.data()};
    int src_stride[1] = {width * 4};

    if (!sws_context_) {
        sws_context_ = sws_getContext(width, height,AV_PIX_FMT_RGBA,encode_context_->width,encode_context_->height,
                                        AV_PIX_FMT_YUV420P,SWS_BILINEAR,nullptr,nullptr,nullptr);

        if (!sws_context_) {
            std::cerr << "Could not reinitialize SwsContext with new dimensionst" << std::endl;
            return;
        }
    }
    if (sws_scale(sws_context_,src_slice,src_stride,0,height,av_frame_->data,av_frame_->linesize) <= 0) {
        std::cerr << "Could not sws_scale " << std::endl;
        return;
    }
}

void VideoEncoder::PrepareAndEncodeVideoFrame(std::shared_ptr<IVideoFrame> video_frame) {
    if (!av_frame_||!av_packet_) {
        std::cerr << "Could not allocate video packet" << std::endl;
        return;
    }
    FlipVideoFrame(video_frame);

    ConvertTextureToFrame(texture_id_, encode_context_->width, encode_context_->height);
    // ++av_frame_->pts;
    av_frame_->pts = frame_index_++;
    EncodeVideoFrame(av_frame_);

}

void VideoEncoder::EncodeVideoFrame(const AVFrame* av_frame) {
    // 阶段1：发送原始帧数据到编码器
    int ret = avcodec_send_frame(encode_context_, av_frame);
    if (ret < 0) {
        std::cerr << "Frame发送失败: " << av_err2str(ret) << std::endl;
        return; // 严重错误应立即终止编码流程
    }

    // 阶段2：循环接收编码后的数据包
    while (true) {
        ret = avcodec_receive_packet(encode_context_, av_packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // EAGAIN表示需要输入更多帧，EOF表示编码器缓冲区已清空
            if (!av_frame) std::cout << "编码器缓冲区已刷新完成" << std::endl;
            break; // 正常退出循环
        } else if (ret < 0) {
            std::cerr << "数据包接收错误: " << av_err2str(ret) << std::endl;
            break; // 非致命错误继续处理后续数据
        }

        // 修正B帧的DTS和PTS顺序
        if (av_packet_->dts != AV_NOPTS_VALUE && av_packet_->pts < av_packet_->dts) {
            std::swap(av_packet_->pts, av_packet_->dts);
        }

        // 阶段3：封装并传递编码后的数据包
        auto av_packet = std::make_shared<IAVPacket>(av_packet_clone(av_packet_));
        av_packet->time_base = encode_context_->time_base; // 同步时间基

        // 线程安全的数据包分发
        {
            std::lock_guard<std::mutex> lock(listener_mutex_);
            if (listener_) {
                listener_->OnVideoEncoderNotifyPacket(av_packet);
            }
        }

        av_packet_unref(av_packet_); // 必须释放数据包引用
    }

    // 阶段4：处理编码结束信号
    if (!av_frame) {
        std::lock_guard<std::mutex> lock(listener_mutex_);
        if (listener_) {
            listener_->OnVideoEncoderNotifyPacket(nullptr); // 发送结束标志
        }
    }
}

void VideoEncoder::CleanPacketsQueue() {
    std::queue<std::shared_ptr<IVideoFrame>> temp_queue;
    std::swap(video_frame_queue_, temp_queue);
}


}