//
// Created by 刘继玺 on 25-3-8.
//

#ifndef PLAYER_SLIDER_H
#define PLAYER_SLIDER_H

#include <QSlider>
#include <QMouseEvent>

class PlayerSlider: public QSlider {
    Q_OBJECT
public:
    // PlayerSlider(QWidget *parent = nullptr);

    explicit PlayerSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *ev) override;
};

#endif //PLAYER_SLIDER_H
