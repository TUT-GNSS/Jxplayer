//
// Created by 刘继玺 on 25-3-1.
//

#include <iostream>
#include "../../include/Igl_context.h"

namespace jxplay {
GLContext::GLContext(QOpenGLContext* shared_gl_context):shared_gl_context_(shared_gl_context) {}

GLContext::~GLContext() {
    DoneCurrent();
    Destroy();
}

bool GLContext::Initialize() {
    if (!shared_gl_context_) {
        std::cerr << "GLContext::Initialize() failed : GL context is null!" << std::endl;
        return false;
    }
    context_ = new QOpenGLContext();
    context_->setShareContext(shared_gl_context_);
    if (!context_->create()) {
        std::cerr << "GLContext::Initialize() failed : context creation failed!" << std::endl;
        return false;
    }

    surface_ = new QOffscreenSurface();
    surface_->create();
    context_->makeCurrent(surface_);

    return true;
}

void GLContext::Destroy() {
    if (surface_) {
        delete surface_;
        surface_ = nullptr;
    }
    if (context_) {
        delete context_;
        context_ = nullptr;
    }
}

void GLContext::MakeCurrent() {
    if (context_ && surface_) {
        context_->makeCurrent(surface_);
    }
}


void GLContext::DoneCurrent() {
    if (context_) {
        context_->doneCurrent();
    }
}


}
