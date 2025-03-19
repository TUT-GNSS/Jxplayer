#include <QApplication>
#include <QSurfaceFormat>
#include <QTranslator>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    // 设置共享OpenGL上下文属性
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // 配置OpenGL Surface格式
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);                   // 设置深度缓冲区大小
    fmt.setVersion(3, 3);                         // 设置OpenGL版本
    fmt.setProfile(QSurfaceFormat::CoreProfile);  // 设置OpenGL配置文件
    QSurfaceFormat::setDefaultFormat(fmt);        // 设置默认格式

    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec(); // 进入应用程序事件循环
}