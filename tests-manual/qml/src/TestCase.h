#pragma once

#include <functional>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions_4_1_Core>
#include <qml/OffscreenSurface.h>

class TestCase {
public:
    using QmlPtr = QSharedPointer<hifi::qml::OffscreenSurface>;
    using Builder = std::function<TestCase*(QWindow*)>;
    TestCase(QWindow* window) : _window(window) {}
    virtual void init();
    virtual void destroy();
    virtual void update();
    virtual void draw() = 0;
    static QUrl getTestResource(const QString& relativePath);

protected:
    QOpenGLFunctions_4_1_Core _glf;
    QWindow* _window;
    std::function<void(uint32_t, void*)> _discardLamdba;
};
