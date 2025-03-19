#include "controllerwidget.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>
#include <iostream>

#include "../../player/include/Iplayer.h"
#include "../../player/src/filter/video_filter.h"

#define DEBUG_PATH

ControllerWidget::ControllerWidget(QWidget* parent, std::shared_ptr<jxplay::IPlayer> player)
    : QWidget(parent), player_(player) {
    // 初始化导入按钮
     button_import_ = new QPushButton("导入", this);
     connect(button_import_, &QPushButton::clicked, this, &ControllerWidget::OnImportButtonClicked);

    // 初始化播放按钮
    button_play_ = new QPushButton("播放", this);
    connect(button_play_, &QPushButton::clicked, this, &ControllerWidget::OnPlayButtonClicked);

     // 初始化倍速按钮
    button_speed_ = new QPushButton("倍速", this);
    connect(button_speed_, &QPushButton::clicked, this, &ControllerWidget::OnSpeedButtonClicked);

    // 初始化导出按钮
    button_export_ = new QPushButton("录制", this);
    connect(button_export_, &QPushButton::clicked, this, &ControllerWidget::OnExportButtonClicked);

    // 初始化视频滤镜按钮 - 垂直翻转
    button_video_filter_flip_vertical_ = new QPushButton("添加垂直翻转滤镜", this);
    connect(button_video_filter_flip_vertical_, &QPushButton::clicked, this,
            &ControllerWidget::OnVideoFilterFlipVerticalButtonClicked);

    // 初始化视频滤镜按钮 - 灰度
    button_video_filter_gray_ = new QPushButton("添加黑白滤镜", this);
    connect(button_video_filter_gray_, &QPushButton::clicked, this, &ControllerWidget::OnVideoFilterGrayButtonClicked);

    // 初始化视频滤镜按钮 - 反转
    button_video_filter_invert_ = new QPushButton("添加反色滤镜", this);
    connect(button_video_filter_invert_, &QPushButton::clicked, this,
            &ControllerWidget::OnVideoFilterInvertButtonClicked);
    
    // 初始化视频滤镜按钮 - 贴纸
    button_video_filter_sticker_ = new QPushButton("添加贴纸滤镜", this);
    connect(button_video_filter_sticker_, &QPushButton::clicked, this,
            &ControllerWidget::OnVideoFilterStickerButtonClicked);

    // 布局设置
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(button_import_);
    layout->addWidget(button_play_);
    layout->addWidget(button_speed_);
    layout->addWidget(button_export_);
    layout->addStretch();
    layout->addWidget(button_video_filter_flip_vertical_);
    layout->addWidget(button_video_filter_gray_);
    layout->addWidget(button_video_filter_invert_);
    layout->addWidget(button_video_filter_sticker_);
    setLayout(layout);

#ifdef DEBUG_PATH
    // std::string videoFilePathStd = std::string(RESOURCE_DIR) + "/video/video0.mp4";
    // player_->Open(videoFilePathStd);
#endif
}

void ControllerWidget::OnImportButtonClicked() {
    if (!player_) {return;}

    QString videoFilePath = QFileDialog::getOpenFileName(
        nullptr, tr("打开视频文件"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "media files(*.avi *.mp4 *.mov)", nullptr, QFileDialog::DontUseNativeDialog);

    if (videoFilePath.isEmpty()) return;

    std::string videoFilePathStd = videoFilePath.toStdString();
    player_->Open(videoFilePathStd);

    // 导入后自动播放
    player_->Play();
    button_play_->setText("暂停");
}

void ControllerWidget::OnPlayButtonClicked() {
    if (!player_) return;

    if (player_->IsPlaying()) {
        auto video_filter = player_->AddVideoFilter(jxplay::VideoFilterType::kPause);
        if (video_filter) {
            video_filter->SetString("PausePath", std::string(RESOURCE_DIR) + "/ui/pause.png");
        }
        QTimer::singleShot(100, [this](){
            player_->Pause();
        });
        button_play_->setText("播放");
    } else {
        player_->Play();
        player_->RemoveVideoFilter(jxplay::VideoFilterType::kPause);
        button_play_->setText("暂停");
    }
}

void ControllerWidget::PlayButtonClicked() {
    OnPlayButtonClicked();
}

void ControllerWidget::OnSpeedButtonClicked() {
    std::cout<<"speed"<<std::endl;
    if (!player_) return;

    if (button_speed_->text() == "1.0" || button_speed_->text() == "倍速") {
        player_->SetPlaybackSpeed(0.5);
        button_speed_->setText("0.5");
        std::cout<<"1.0"<<std::endl;
    }else if (button_speed_->text() == "0.5") {
        player_->SetPlaybackSpeed(0.8);
        button_speed_->setText("0.8");
        std::cout<<"0.5"<<std::endl;
    }else {
        player_->SetPlaybackSpeed(1.0);
        button_speed_->setText("1.0");
        std::cout<<"0.8"<<std::endl;
    }
}

void ControllerWidget::OnExportButtonClicked() {
    std::cout<<"export"<<std::endl;
    if (!player_) return;

    if (player_->IsRecording()) {
        player_->StopRecording();
        button_export_->setText("录制");
    } else {
        if (!player_->IsPlaying()) {
            QMessageBox::information(nullptr, "提示", "非播放状态下无法录制视频！");
            return;
        }
// #ifdef DEBUG_PATH
//         // 如果cache 目录不存在，创建cache目录
//         QDir dir(CACHE_DIR);
//         if (!dir.exists()) dir.mkpath(".");
//         // 当前时间作为文件名
//         auto fileName = std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".mp4";
//         std::string outputPath = std::string(CACHE_DIR) + "/" + fileName;
//         play_->StartRecording(outputPath, 0);
//         button_export_->setText("停止");
//         return;
// #endif

        QString dirName = QFileDialog::getSaveFileName(
            this, tr("选择导出路径"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        if (dirName.isEmpty()) {
            return;
        }
        std::string dirNameStd = dirName.toStdString();
        player_->StartRecording(dirNameStd, 0);
        button_export_->setText("停止");
    }
}

void ControllerWidget::OnVideoFilterFlipVerticalButtonClicked() {
    std::cout<<"video_filter_Flip"<<std::endl;
    if (!player_) {
        return;
    }

    if (button_video_filter_flip_vertical_->text() == "添加垂直翻转滤镜") {
        auto video_filter = player_->AddVideoFilter(jxplay::VideoFilterType::kFlipVertical);
        button_video_filter_flip_vertical_->setText("移除垂直翻转滤镜");
    } else {
        player_->RemoveVideoFilter(jxplay::VideoFilterType::kFlipVertical);
        button_video_filter_flip_vertical_->setText("添加垂直翻转滤镜");
    }
}

void ControllerWidget::OnVideoFilterGrayButtonClicked() {
    std::cout<<"video_filter_gray"<<std::endl;
    if (!player_) return;

    if (button_video_filter_gray_->text() == "添加黑白滤镜") {
        auto videoFilter = player_->AddVideoFilter(jxplay::VideoFilterType::kGray);
        button_video_filter_gray_->setText("移除黑白滤镜");
    } else {
        player_->RemoveVideoFilter(jxplay::VideoFilterType::kGray);
        button_video_filter_gray_->setText("添加黑白滤镜");
    }
}

void ControllerWidget::OnVideoFilterInvertButtonClicked() {
    std::cout<<"video_filter_invert"<<std::endl;
    if (!player_) return;

    if (button_video_filter_invert_->text() == "添加反色滤镜") {
        auto videoFilter = player_->AddVideoFilter(jxplay::VideoFilterType::kInvert);
        button_video_filter_invert_->setText("移除反色滤镜");
    } else {
        player_->RemoveVideoFilter(jxplay::VideoFilterType::kInvert);
        button_video_filter_invert_->setText("添加反色滤镜");
    }
}

void ControllerWidget::OnVideoFilterStickerButtonClicked() {
    std::cout<<"video_filter_sticker"<<std::endl;
    if (!player_) return;

    if (button_video_filter_sticker_->text() == "添加贴纸滤镜") {
        auto videoFilter = player_->AddVideoFilter(jxplay::VideoFilterType::kSticker);
        if (videoFilter) {
            videoFilter->SetString("StickerPath", std::string(RESOURCE_DIR) + "/sticker/Sticker0.png");
            // videoFilter->SetString("StickerPath", std::string(RESOURCE_DIR) + "/sticker/EyeMask.png");
            videoFilter->SetString("ModelPath", std::string(RESOURCE_DIR) + "/pack/Megatron");
        }
        button_video_filter_sticker_->setText("移除贴纸滤镜");
    } else {
        player_->RemoveVideoFilter(jxplay::VideoFilterType::kSticker);
        button_video_filter_sticker_->setText("添加贴纸滤镜");
    }
}