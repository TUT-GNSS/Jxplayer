//
// Created by 刘继玺 on 25-2-28.
//

#ifndef IGLCONTEXT_H
#define IGLCONTEXT_H

// 定义不同平台的OpenGL上下文，这里只定义了QT的OpenGL上下文
#include <QOffscreenSurface>
#include <QOpenGLContext>

namespace jxplay{
class GLContext{
public:
    explicit GLContext(QOpenGLContext *shared_gl_context);
    virtual ~GLContext();

    bool Initialize();   // 初始化OpenGL上下文
    void Destroy();      // 销毁OpenGL上下文
    void MakeCurrent();  // 设置当前OpenGL上下文
    void DoneCurrent();  // 释放当前OpenGL上下文

private:
    QOpenGLContext *shared_gl_context_ = nullptr;
    QOpenGLContext *context_ = nullptr;
    QOffscreenSurface *surface_ = nullptr;
};
}

#endif //IGLCONTEXT_H
