//
// Created by 刘继玺 on 25-2-25.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QSlider>
#include <QSplitter>
#include <QTimer>
#include <iostream>
#include <QVBoxLayout>

#include "../player/include/Iplayback_listener.h"
#include "ui/controllerwidget.h"
#include "ui/playerwidget.h"
#include "ui/player_slider.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QWidget {
    Q_OBJECT
   public:
    MainWindow();
    ~MainWindow() override = default;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    // void keyReleaseEvent(QKeyEvent* event) override;

private slots:
     void OnSliderMoved(int value);

private:
    class PlaybackListener : public jxplay::IPlaybackListener {
    public:
        explicit PlaybackListener(MainWindow* window);
        void NotifyPlaybackStarted() override;
        void NotifyPlaybackTimeChanged(float time_stamp, float duration) override;
        void NotifyPlaybackPaused() override;
        void NotifyPlaybackEOF() override;

    private:
        MainWindow* window_{nullptr};
    };

    bool is_seeking_ = false;  // 添加跳转状态标志
    bool was_playing_ = false;
    std::shared_ptr<ControllerWidget> controller_widget_{nullptr};
    std::shared_ptr<PlayerWidget> player_widget_{nullptr};
    std::shared_ptr<QSlider> progress_slider_;
    std::unique_ptr<QVBoxLayout> vbox_;
    std::shared_ptr<jxplay::IPlayer> player_;
};


#endif //MAINWINDOW_H
