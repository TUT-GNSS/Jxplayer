#ifndef LEARNAV_CONTROLLERWIDGET_H
#define LEARNAV_CONTROLLERWIDGET_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>

namespace jxplay {
struct IPlayer;
}

class ControllerWidget : public QWidget {
    Q_OBJECT
public:
    explicit ControllerWidget(QWidget* parent, std::shared_ptr<jxplay::IPlayer> player);
    ~ControllerWidget() override = default;
    void PlayButtonClicked();

private slots:
    void OnImportButtonClicked();
    void OnPlayButtonClicked();
    void OnSpeedButtonClicked();
    void OnExportButtonClicked();
    void OnVideoFilterFlipVerticalButtonClicked();
    void OnVideoFilterGrayButtonClicked();
    void OnVideoFilterInvertButtonClicked();
    void OnVideoFilterStickerButtonClicked();

private:
    QPushButton* button_import_{nullptr};
    QPushButton* button_play_{nullptr};
    QPushButton* button_speed_{nullptr};
    QPushButton* button_export_{nullptr};
    QPushButton* button_video_filter_flip_vertical_{nullptr};
    QPushButton* button_video_filter_gray_{nullptr};
    QPushButton* button_video_filter_invert_{nullptr};
    QPushButton* button_video_filter_sticker_{nullptr};

    std::shared_ptr<jxplay::IPlayer> player_;
    bool is_playing_ = false;
};

#endif  // LEARNAV_CONTROLLERWIDGET_H
