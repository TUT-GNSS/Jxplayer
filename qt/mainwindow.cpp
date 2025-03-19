//
// Created by 刘继玺 on 25-2-25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainWindow.h" resolved

#include "mainwindow.h"
#include "ui_MainWindow.h"
#include "../player/src/engine/player.h"
#include <QTimer>

MainWindow::PlaybackListener::PlaybackListener(MainWindow *window) : window_(window) {}

void MainWindow::PlaybackListener::NotifyPlaybackStarted() { std::cout << "NotifyPlaybackStarted" << std::endl; }
void MainWindow::PlaybackListener::NotifyPlaybackTimeChanged(float time_stamp, float duration) {
    // 只有不在手动跳转时才更新进度条
    if (!window_->is_seeking_) {
        window_->progress_slider_->setValue(static_cast<int>(time_stamp / duration * 1000));
    }
}

void MainWindow::PlaybackListener::NotifyPlaybackPaused() { std::cout << "NotifyPlaybackPaused" << std::endl; }

void MainWindow::PlaybackListener::NotifyPlaybackEOF() { std::cout << "NotifyPlaybackEOF" << std::endl; }

MainWindow::MainWindow() : QWidget(),progress_slider_(nullptr) {

    // 创建共享的OpenGL上下文
    jxplay::GLContext shared_gl_context(QOpenGLContext::globalShareContext());
    player_ = std::shared_ptr<jxplay::IPlayer>(jxplay::IPlayer::Create(shared_gl_context));
    player_->SetPlaybackListener(std::make_shared<PlaybackListener>(this));

    // 允许接收键盘事件
    setFocusPolicy(Qt::StrongFocus);

    // 添加播放控件
    controller_widget_ = std::make_shared<ControllerWidget>(this,player_);

    // 设置布局
    vbox_ = std::make_unique<QVBoxLayout>();
    vbox_->setContentsMargins(0, 0, 0, 0);
    setLayout(vbox_.get());

    // 添加播放器主界面
    player_widget_ = std::make_shared<PlayerWidget>(this,player_,controller_widget_);

    // 添加进度条
    progress_slider_ = std::make_shared<QSlider>(Qt::Horizontal, this);
    progress_slider_->setRange(0, 1000);
    progress_slider_->setValue(0);
    // 连接进度条的 sliderMoved 信号到自定义的槽函数
    connect(progress_slider_.get(), &QSlider::sliderPressed, this, [this]() {
        is_seeking_ = true;  // 开始拖动时设置标志
        was_playing_ = player_->IsPlaying();
        if (player_->IsPlaying()) {
            player_->Pause();
        }
    });
    connect(progress_slider_.get(), &QSlider::sliderReleased, this, [this]() {
        is_seeking_ = false;
        OnSliderMoved(progress_slider_->value());
        // 恢复拖动进度条前的播放状态
        if (was_playing_) {
            player_->Play();
        }
    });
    connect(progress_slider_.get(), &QSlider::valueChanged, this, &MainWindow::OnSliderMoved);
    // 设置控件位置
    vbox_->addWidget(player_widget_.get());
    vbox_->addWidget(progress_slider_.get());
    vbox_->addWidget(controller_widget_.get());

    // 设置窗口最小尺寸
    setMinimumWidth(1280);
    setMinimumHeight(720);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    // 处理窗口大小调整事件
    QWidget::resizeEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    // if (event->button() == Qt::LeftButton) {
    //    controller_widget_->PlayButtonClicked();
    // }
    QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    // 处理鼠标移动事件
    QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    // 处理鼠标释放事件
    QWidget::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
    // 处理鼠标双击事件
    if (windowState() == Qt::WindowMaximized) {
        showNormal();  // 如果窗口已最大化，则恢复正常大小
    } else {
        showMaximized();  // 否则，最大化窗口
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MainWindow::OnSliderMoved(int value) {
    // 处理进度条滑动事件
    if (is_seeking_) {
        // 立即执行跳转操作
        player_->SeekTo(static_cast<float>(value) / 1000);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    switch(event->key()) {
        case Qt::Key_Space:
           controller_widget_->PlayButtonClicked();
        break;
        case Qt::Key_Left:
            is_seeking_ = false;
            OnSliderMoved(std::max(progress_slider_->value()-25, 0));
            is_seeking_ = true;
        break;
        case Qt::Key_Right:
            is_seeking_ = false;
            OnSliderMoved(std::min(progress_slider_->value()+25, 1000));
            is_seeking_ = true;
        break;
    }
    QWidget::keyPressEvent(event);
}

// void MainWindow::Pause() {
//
// }
//
// void MainWindow::Play() {
//     player_->Play();
//     player_->RemoveVideoFilter(jxplay::VideoFilterType::kPause);
// }