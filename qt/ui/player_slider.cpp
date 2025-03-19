//
// Created by 刘继玺 on 25-3-8.
//

#include "player_slider.h"
#include <iostream>
#include <QStyle>

PlayerSlider::PlayerSlider(Qt::Orientation orientation, QWidget *parent) : QSlider(orientation, parent) {}

void PlayerSlider::mousePressEvent(QMouseEvent *ev) {
    // 计算点击位置对应值
    // int pos = minimum() + (maximum()-minimum()) * ev->x() / width();
    // setValue(pos);
    //根据鼠标点击的位置 来设置滑动条的 位置
    // int value = QStyle::sliderValueFromPosition(this->minimum(), this->maximum(), ev->x(), this->width());
    // setValue(value);
}
