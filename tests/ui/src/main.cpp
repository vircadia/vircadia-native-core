//
//  main.cpp
//  tests/render-utils/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OffscreenUi.h"
#include <QWindow>
#include <QFile>
#include <QTime>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QResizeEvent>
#include <QLoggingCategory>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QApplication>
#include <QOpenGLDebugLogger>
#include <QOpenGLFunctions>
#include <QQmlContext>
#include <QtQml/QQmlApplicationEngine>
#include <PathUtils.h>
#include <memory>
#include <glm/glm.hpp>
#include <PathUtils.h>
#include <QDir>
#include "MessageDialog.h"
#include "VrMenu.h"
#include "InfoView.h"
#include <QDesktopWidget>

class RateCounter {
    std::vector<float> times;
    QElapsedTimer timer;
public:
    RateCounter() {
        timer.start();
    }

    void reset() {
        times.clear();
    }

    unsigned int count() const {
        return times.size() - 1;
    }

    float elapsed() const {
        if (times.size() < 1) {
            return 0.0f;
        }
        float elapsed = *times.rbegin() - *times.begin();
        return elapsed;
    }

    void increment() {
        times.push_back(timer.elapsed() / 1000.0f);
    }

    float rate() const {
        if (elapsed() == 0.0f) {
            return 0.0f;
        }
        return (float) count() / elapsed();
    }
};


class MenuConstants : public QObject{
    Q_OBJECT
    Q_ENUMS(Item)

public:
    enum Item {
        RenderLookAtTargets,
    };

public:
    MenuConstants(QObject* parent = nullptr) : QObject(parent) {

    }
};

const QString& getResourcesDir() {
    static QString dir;
    if (dir.isEmpty()) {
        QDir path(__FILE__);
        path.cdUp();
        dir = path.cleanPath(path.absoluteFilePath("../../../interface/resources/")) + "/";
        qDebug() << "Resources Path: " << dir;
    }
    return dir;
}

const QString& getQmlDir() {
    static QString dir;
    if (dir.isEmpty()) {
        dir = getResourcesDir() + "qml/";
        qDebug() << "Qml Path: " << dir;
    }
    return dir;
}

const QString& getTestQmlDir() {
    static QString dir;
    if (dir.isEmpty()) {
        QDir path(__FILE__);
        path.cdUp();
        dir = path.cleanPath(path.absoluteFilePath("../")) + "/";
        qDebug() << "Qml Test Path: " << dir;
    }
    return dir;
}

// Create a simple OpenGL window that renders text in various ways
class QTestWindow : public QWindow, private QOpenGLFunctions  {
    Q_OBJECT

    QOpenGLContext* _context{ nullptr };
    QSize _size;
    bool _altPressed{ false };
    RateCounter fps;
    QTimer _timer;
    int testQmlTexture{ 0 };

public:
    QObject* rootMenu;

    QTestWindow() {
        _timer.setInterval(1);
        connect(&_timer, &QTimer::timeout, [=] {
            draw();
        });

        DependencyManager::set<OffscreenUi>();
        setSurfaceType(QSurface::OpenGLSurface);

        QSurfaceFormat format;
        format.setDepthBufferSize(16);
        format.setStencilBufferSize(8);
        format.setVersion(4, 1);
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
        format.setOption(QSurfaceFormat::DebugContext);

        setFormat(format);

        _context = new QOpenGLContext;
        _context->setFormat(format);
        if (!_context->create()) {
            qFatal("Could not create OpenGL context");
        }

        show();
        makeCurrent();
        initializeOpenGLFunctions();

        {
            QOpenGLDebugLogger* logger = new QOpenGLDebugLogger(this);
            logger->initialize(); // initializes in the current context, i.e. ctx
            logger->enableMessages();
            connect(logger, &QOpenGLDebugLogger::messageLogged, this, [&](const QOpenGLDebugMessage & debugMessage) {
                qDebug() << debugMessage;
            });
            //        logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
        }

        qDebug() << (const char*)this->glGetString(GL_VERSION);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.2f, 0.2f, 0.2f, 1);
        glDisable(GL_DEPTH_TEST);

        MessageDialog::registerType();
        VrMenu::registerType();
        InfoView::registerType();


        auto offscreenUi = DependencyManager::get<OffscreenUi>(); 
        offscreenUi->create(_context);
        connect(offscreenUi.data(), &OffscreenUi::textureUpdated, this, [this, offscreenUi](int textureId) {
            testQmlTexture = textureId;
        });

        makeCurrent();

        offscreenUi->setProxyWindow(this);
        QDesktopWidget* desktop = QApplication::desktop();
        QRect rect = desktop->availableGeometry(desktop->screenCount() - 1);
        int height = rect.height();
        //rect.setHeight(height / 2);
        rect.setY(rect.y() + height / 2);
        setGeometry(rect);
//        setFramePosition(QPoint(-1000, 0));
//        resize(QSize(800, 600));

#ifdef QML_CONTROL_GALLERY
        offscreenUi->setBaseUrl(QUrl::fromLocalFile(getTestQmlDir()));
        offscreenUi->load(QUrl("main.qml"));
#else 
        offscreenUi->setBaseUrl(QUrl::fromLocalFile(getQmlDir()));
        offscreenUi->load(QUrl("TestRoot.qml"));
        offscreenUi->load(QUrl("TestMenu.qml"));
        // Requires a root menu to have been loaded before it can load
        VrMenu::load();
#endif
        installEventFilter(offscreenUi.data());
        offscreenUi->resume();
        _timer.start();
    }

    virtual ~QTestWindow() {
        DependencyManager::destroy<OffscreenUi>();
    }

private:
    void draw() {
        if (!isVisible()) {
            return;
        }

        makeCurrent();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio());

        renderQml();

        _context->swapBuffers(this);
        glFinish();

        fps.increment();
        if (fps.elapsed() >= 2.0f) {
            qDebug() << "FPS: " << fps.rate();
            fps.reset();
        }
    }

    void makeCurrent() {
        _context->makeCurrent(this);
    }

    void renderQml();

    void resizeWindow(const QSize & size) {
        _size = size;
        DependencyManager::get<OffscreenUi>()->resize(_size);
    }


protected:
    void resizeEvent(QResizeEvent* ev) override {
        resizeWindow(ev->size());
    }

  
    void keyPressEvent(QKeyEvent* event) {
        _altPressed = Qt::Key_Alt == event->key();
        switch (event->key()) {
            case Qt::Key_B:
                if (event->modifiers() & Qt::CTRL) {
                    auto offscreenUi = DependencyManager::get<OffscreenUi>();
                    offscreenUi->load("Browser.qml");
                }
                break;
            case Qt::Key_L:
                if (event->modifiers() & Qt::CTRL) {
                    InfoView::show(getResourcesDir() + "html/interface-welcome.html", true);
                }
                break;
            case Qt::Key_K:
                if (event->modifiers() & Qt::CTRL) {
                    OffscreenUi::question("Message title", "Message contents", [](QMessageBox::Button b){
                        qDebug() << b;
                    });
                }
                break;
            case Qt::Key_J:
                if (event->modifiers() & Qt::CTRL) {
                    auto offscreenUi = DependencyManager::get<OffscreenUi>();
                    rootMenu = offscreenUi->getRootItem()->findChild<QObject*>("rootMenu");
                    QMetaObject::invokeMethod(rootMenu, "popup");
                }
                break;
        }
        QWindow::keyPressEvent(event);
    }
    QQmlContext* menuContext{ nullptr };
    void keyReleaseEvent(QKeyEvent *event) {
        if (_altPressed && Qt::Key_Alt == event->key()) {
            VrMenu::toggle();
        }
    }

    void moveEvent(QMoveEvent* event) {
        static qreal oldPixelRatio = 0.0;
        if (devicePixelRatio() != oldPixelRatio) {
            oldPixelRatio = devicePixelRatio();
            resizeWindow(size());
        }
        QWindow::moveEvent(event);
    }
};

void QTestWindow::renderQml() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (testQmlTexture > 0) {
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, testQmlTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0);
        glVertex2f(-1, -1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, 1);
        glTexCoord2f(1, 1);
        glVertex2f(1, 1);
        glTexCoord2f(1, 0);
        glVertex2f(1, -1);
    }
    glEnd();
}


const char * LOG_FILTER_RULES = R"V0G0N(
hifi.offscreen.focus.debug=false
qt.quick.mouse.debug=false
)V0G0N";

int main(int argc, char** argv) {    
    QApplication app(argc, argv);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);
    QTestWindow window;
    app.exec();
    return 0;
}

#include "main.moc"
