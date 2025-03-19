#ifndef LEARNAV_PLAYERWIDGET_H
#define LEARNAV_PLAYERWIDGET_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <memory>
#include "controllerwidget.h"
namespace jxplay {
class OpenGLView;
class IPlayer;
}  // namespace jxplay

class PlayerWidget final : public QWidget {
    Q_OBJECT
public:
    explicit PlayerWidget(QWidget *parent, std::shared_ptr<jxplay::IPlayer> player);
    explicit PlayerWidget(QWidget *parent, std::shared_ptr<jxplay::IPlayer> player,std::shared_ptr<ControllerWidget> controller_widget);
    // explicit PlayerWidget(QWidget *parent);
    ~PlayerWidget() override;
protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
private:
    jxplay::OpenGLView *openGLView_{nullptr};
    std::shared_ptr<jxplay::IPlayer> player_{nullptr};
    std::shared_ptr<ControllerWidget> controller_widget_{nullptr};
};

#endif  // LEARNAV_PLAYERWIDGET_H
