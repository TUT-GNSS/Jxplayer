//
// Created by 刘继玺 on 25-3-2.
//

#include "openGL_view.h"

#include "../src/interface/Ivideo_display_view.h"

namespace jxplay {

OpenGLView::OpenGLView(QWidget *parent) : QOpenGLWidget(parent) {
    video_display_view_= std::shared_ptr<IVideoDisplayView>(IVideoDisplayView::Create());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void OpenGLView::initializeGL() {
    if (video_display_view_) {
        video_display_view_->InitializeGL();
    }
}

void OpenGLView::resizeGL(int w, int h) {
    gl_width_= w * devicePixelRatio();
    gl_height_= h * devicePixelRatio();
}

void OpenGLView::paintGL() {
    if (video_display_view_) {
        video_display_view_->Render(gl_width_, gl_height_, 0.0, 0.0, 0.0);
    }
    update();
}

}  // namespace av