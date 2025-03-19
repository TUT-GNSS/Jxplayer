//
// Created by 刘继玺 on 25-3-4.
//

#include "muxer.h"
#include <iostream>

namespace jxplay {

Muxer::~Muxer() {
    if (format_context_) {
        av_write_trailer(format_context_);
        avio_closep(&format_context_->pb);
        avformat_free_context(format_context_);
        format_context_ = nullptr;
    }
}

bool Muxer::Configure(const std::string &output_file_path, FileWriterParameters &parameters, int flags) {
    std::lock_guard<std::mutex> lock(muxer_mutex_);

    //创建输出上下文
    if (avformat_alloc_output_context2(&format_context_,nullptr,"mp4",output_file_path.c_str())<0) {
        std::cerr << "Could not creat output context" << std::endl;
        return false;
    }

    // 添加音频流
    audio_stream_info_.av_stream = avformat_new_stream(format_context_,nullptr);
    if (!audio_stream_info_.av_stream) {
        std::cerr << "Failed to creat audio stream" << std::endl;
        return false;
    }

    // 设置音频流参数
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (codec) {
        AVCodecContext* codec_context = avcodec_alloc_context3(codec);
        if (codec_context) {
            codec_context->sample_rate = parameters.sample_rate;
            av_channel_layout_default(&codec_context->ch_layout,parameters.channels);
            codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
            codec_context->bit_rate = 128000;
            codec_context->frame_size = 1024;
            codec_context->time_base = {1, parameters.sample_rate};

            if (avcodec_open2(codec_context,codec,nullptr) == 0) {
                avcodec_parameters_from_context(audio_stream_info_.av_stream->codecpar,codec_context);
                audio_stream_info_.av_stream->time_base = codec_context->time_base;
            }
            else {
                std::cerr << "Muxer::avcodec_open2" << std::endl;
            }
            avcodec_free_context(&codec_context);
        }
        else {
            std::cerr << "Could not create audio stream" << std::endl;
        }
    }else {
        std::cerr << "Could not create audio stream" << std::endl;
    }

    // 添加视频流
    video_stream_info_.av_stream = avformat_new_stream(format_context_,nullptr);
    if (!video_stream_info_.av_stream) {
        std::cerr << "Could not create video stream" << std::endl;
        return false;
    }

    // 设置视频流参数
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (codec) {
        AVCodecContext* codec_context = avcodec_alloc_context3(codec);
        if (codec_context) {
            codec_context->bit_rate = parameters.width*parameters.height*4;
            codec_context->width = parameters.width;
            codec_context->height = parameters.height;
            codec_context->time_base = { 1, parameters.fps };
            codec_context->framerate = {parameters.fps,1};
            codec_context->gop_size = 30;
            codec_context->max_b_frames=1;
            codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

            if (avcodec_open2(codec_context,codec,nullptr) == 0) {
                avcodec_parameters_from_context(video_stream_info_.av_stream->codecpar,codec_context);
                // video_stream_info_.av_stream->time_base = codec_context->time_base; // 同步时间基
            }else {
                std::cerr << "Could not create video stream" << std::endl;
            }
            avcodec_free_context(&codec_context);
        }
        else {
            std::cerr << "Could not create video stream" << std::endl;
        }
    }else {
        std::cerr << "Could not create video stream" << std::endl;
    }

    // 打开输出文件
    if (!(format_context_->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_context_->pb,output_file_path.c_str(),AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file" << std::endl;
            return false;
        }
    }

    //写文件头
    AVDictionary *options = nullptr;
    av_dict_set(&options,"movflags","faststart",0);
    int ret = avformat_write_header(format_context_,&options);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret,errbuf,sizeof(errbuf));
        std::cerr << "Error occurred when opening output file: "<<errbuf << std::endl;
        return false;
    }

    return true;
}

void Muxer::NotifyAudioPacket(std::shared_ptr<IAVPacket> packet) {
    if (!packet || !packet->av_packet) {
        std::cerr << "Muxer::NotifyAudioPacket: packet is null" << std::endl;
        return;
    }
    std::lock_guard<std::mutex> lock(muxer_mutex_);
    if (audio_stream_info_.av_stream && !audio_stream_info_.is_finished) {
        AVPacket* av_packet = packet->av_packet;
        av_packet->stream_index = audio_stream_info_.av_stream->index;
        av_packet->pts = av_rescale_q(av_packet->pts, packet->time_base, audio_stream_info_.av_stream->time_base);
        av_packet->dts = av_rescale_q(av_packet->dts, packet->time_base, audio_stream_info_.av_stream->time_base);
        av_packet->duration = av_rescale_q(av_packet->duration, packet->time_base, audio_stream_info_.av_stream->time_base);
        audio_stream_info_.packets_queue.push(packet);
        WriteInterleavedPacket();
    }
}

void Muxer::NotifyVideoPacket(std::shared_ptr<IAVPacket> packet) {
    if (!packet || !packet->av_packet) {
        std::cerr << "Muxer::NotifyVideoPacket: packet is null" << std::endl;
        return;
    }
    std::lock_guard<std::mutex> lock(muxer_mutex_);
    if (video_stream_info_.av_stream && !video_stream_info_.is_finished) {
        AVPacket* av_packet = packet->av_packet;
        av_packet->stream_index = video_stream_info_.av_stream->index;
        av_packet->pts = av_rescale_q(av_packet->pts, packet->time_base, video_stream_info_.av_stream->time_base);
        av_packet->dts= av_rescale_q(av_packet->dts, packet->time_base, video_stream_info_.av_stream->time_base);
        av_packet->duration = av_rescale_q(av_packet->duration, packet->time_base, video_stream_info_.av_stream->time_base);
        video_stream_info_.packets_queue.push(packet);
        WriteInterleavedPacket();
    }
}

void Muxer::WriteInterleavedPacket() {

    while (!audio_stream_info_.packets_queue.empty() && !video_stream_info_.packets_queue.empty()) {
        auto audio_packet = audio_stream_info_.packets_queue.front();
        auto video_packet = video_stream_info_.packets_queue.front();


        // 分别使用每个包的时间基
        // double audio_time = audio_packet->av_packet->pts * av_q2d(audio_stream_info_.av_stream->time_base);
        // double video_time = video_packet->av_packet->pts * av_q2d(video_stream_info_.av_stream->time_base);

        const AVRational global_timebase = {1, 1000};
        int64_t audio_time = av_rescale_q(audio_packet->av_packet->pts, audio_stream_info_.av_stream->time_base, global_timebase);
        int64_t video_time = av_rescale_q(video_packet->av_packet->pts, video_stream_info_.av_stream->time_base, global_timebase);

        std::cout<<"audio time: "<<audio_time<<"video time: "<<video_time<<std::endl;
       // 根据时间基先后 决定写入音频还是视频
        if (audio_time<=video_time) {
            audio_stream_info_.packets_queue.pop();
            av_interleaved_write_frame(format_context_,audio_packet->av_packet);
            av_packet_unref(audio_packet->av_packet);//释放写之后的包
        }else {
            video_stream_info_.packets_queue.pop();
            av_interleaved_write_frame(format_context_,video_packet->av_packet);
            av_packet_unref(video_packet->av_packet);//释放写之后的包
        }
    }

}

void Muxer::NotifyAudioFinished() {
    std::cout<<"Muxer::NotifyAudioFinished"<<std::endl;
    std::lock_guard<std::mutex> lock(muxer_mutex_);
    audio_stream_info_.is_finished = true;
}
void Muxer::NotifyVideoFinished() {
    std::cout<<"Muxer::NotifyVideoFinished"<<std::endl;
    std::lock_guard<std::mutex> lock(muxer_mutex_);
    video_stream_info_.is_finished = true;
}

}