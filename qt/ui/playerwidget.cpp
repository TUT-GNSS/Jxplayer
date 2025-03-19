//
// Copyright (c) 2024 Yellow. All rights reserved.
//

#include "playerwidget.h"

#include <QVBoxLayout>

#include "../../player/include/Iplayer.h"
#include "../../player/qt/openGL_view.h"

PlayerWidget::PlayerWidget(QWidget *parent, std::shared_ptr<jxplay::IPlayer> player) : QWidget(parent), player_(player) {
    openGLView_ = new jxplay::OpenGLView(this);
    if (player_) {
        player_->AttachDisplayView(openGLView_->GetVideoDisplayView());
    }

    auto* vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->addWidget(openGLView_);
}
PlayerWidget::PlayerWidget(QWidget *parent, std::shared_ptr<jxplay::IPlayer> player,std::shared_ptr<ControllerWidget> controller_widget):PlayerWidget(parent, player){
    controller_widget_ = controller_widget;
}

PlayerWidget::~PlayerWidget() {
    if (player_ && openGLView_) {
        player_->DetachDisplayView(openGLView_->GetVideoDisplayView());
    }
}

void PlayerWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (controller_widget_) {
        controller_widget_->PlayButtonClicked();
    }
}

void PlayerWidget::mouseDoubleClickEvent(QMouseEvent *event) {

}
