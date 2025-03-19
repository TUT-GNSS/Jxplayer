//
// Created by 刘继玺 on 25-3-2.
//

#ifndef OPENGL_VIEW_H
#define OPENGL_VIEW_H

#include <QOpenGLContext>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <memory>

namespace jxplay {

class IVideoDisplayView;

class OpenGLView : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit OpenGLView(QWidget *parent = nullptr);
    ~OpenGLView() override = default;

    std::shared_ptr<IVideoDisplayView> GetVideoDisplayView() {
        return video_display_view_;
    }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    int gl_width_ = 0;
    int gl_height_ = 0;
    std::shared_ptr<IVideoDisplayView> video_display_view_;
};

}  // namespace jxplay

#endif //OPENGL_VIEW_H
