//
//  Created by Bradley Austin Davis on 2015-04-22
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <gl/Config.h>
#include <gl/OglplusHelpers.h>
#include <gl/GLHelpers.h>
#include <memory>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>
#include <QtCore/QUuid>

#include <QtGui/QWindow>
#include <QtGui/QImage>
#include <QtGui/QGuiApplication>
#include <QtGui/QResizeEvent>
#include <QtGui/QScreen>

#include <gl/QOpenGLContextWrapper.h>

#include <QtScript/QScriptEngine>

#include <QtQml/QQmlContext>
#include <QtQml/QQmlApplicationEngine>

#include <GLMHelpers.h>
#include <gl/OffscreenGLCanvas.h>
#include <OffscreenUi.h>
#include <PathUtils.h>
#include <PathUtils.h>
#include <VrMenu.h>
#include <InfoView.h>
#include <QmlWebWindowClass.h>
#include <RegisteredMetaTypes.h>

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

const QString& getExamplesDir() {
    static QString dir;
    if (dir.isEmpty()) {
        QDir path(__FILE__);
        path.cdUp();
        dir = path.cleanPath(path.absoluteFilePath("../../../examples/")) + "/";
        qDebug() << "Resources Path: " << dir;
    }
    return dir;
}

const QString& getInterfaceQmlDir() {
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

    size_t count() const {
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




extern QOpenGLContext* qt_gl_global_share_context();


static bool hadUncaughtExceptions(QScriptEngine& engine, const QString& fileName) {
    if (engine.hasUncaughtException()) {
        const auto backtrace = engine.uncaughtExceptionBacktrace();
        const auto exception = engine.uncaughtException().toString();
        const auto line = QString::number(engine.uncaughtExceptionLineNumber());
        engine.clearExceptions();

        auto message = QString("[UncaughtException] %1 in %2:%3").arg(exception, fileName, line);
        if (!backtrace.empty()) {
            static const auto lineSeparator = "\n    ";
            message += QString("\n[Backtrace]%1%2").arg(lineSeparator, backtrace.join(lineSeparator));
        }
        qWarning() << qPrintable(message);
        return true;
    }
    return false;
}

const unsigned int SCRIPT_DATA_CALLBACK_USECS = floor(((1.0f / 60.0f) * 1000 * 1000) + 0.5f);

static QScriptValue debugPrint(QScriptContext* context, QScriptEngine* engine) {
    QString message = "";
    for (int i = 0; i < context->argumentCount(); i++) {
        if (i > 0) {
            message += " ";
        }
        message += context->argument(i).toString();
    }
    qDebug().noquote() << "script:print()<<" << message;  // noquote() so that \n is treated as newline

    message = message.replace("\\", "\\\\")
        .replace("\n", "\\n")
        .replace("\r", "\\r")
        .replace("'", "\\'");
    engine->evaluate("Script.print('" + message + "')");

    return QScriptValue();
}

class ScriptEngine : public QScriptEngine {
    Q_OBJECT

public:
    void loadFile(const QString& scriptPath) {
        if (_isRunning) {
            return;
        }
        qDebug() << "Loading script from " << scriptPath;
        _fileNameString = scriptPath;

        QFile file(scriptPath);
        if (file.exists()) {
            file.open(QIODevice::ReadOnly);
            _scriptContents = file.readAll();
        } else {
            qFatal("Missing file ");
        }
        runInThread();
    }

    Q_INVOKABLE void stop() {
        if (!_isFinished) {
            if (QThread::currentThread() != thread()) {
                QMetaObject::invokeMethod(this, "stop");
                return;
            }
            _isFinished = true;
            if (_wantSignals) {
                emit runningStateChanged();
            }
        }
    }

    Q_INVOKABLE void print(const QString& message) {
        if (_wantSignals) {
            emit printedMessage(message);
        }
    }

    Q_INVOKABLE QObject* setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot) {
        // create the timer, add it to the map, and start it
        QTimer* newTimer = new QTimer(this);
        newTimer->setSingleShot(isSingleShot);

        connect(newTimer, &QTimer::timeout, this, &ScriptEngine::timerFired);

        // make sure the timer stops when the script does
        connect(this, &ScriptEngine::scriptEnding, newTimer, &QTimer::stop);

        _timerFunctionMap.insert(newTimer, function);

        newTimer->start(intervalMS);
        return newTimer;
    }

    Q_INVOKABLE QObject* setInterval(const QScriptValue& function, int intervalMS) {
        return setupTimerWithInterval(function, intervalMS, false);
    }

    Q_INVOKABLE QObject* setTimeout(const QScriptValue& function, int timeoutMS) {
        return setupTimerWithInterval(function, timeoutMS, true);
    }
private:

    void runInThread() {
        QThread* workerThread = new QThread();
        connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);
        connect(workerThread, &QThread::started, this, &ScriptEngine::run);
        connect(workerThread, &QThread::finished, this, &ScriptEngine::deleteLater);
        connect(this, &ScriptEngine::doneRunning, workerThread, &QThread::quit);
        moveToThread(workerThread);
        workerThread->start();
    }

    void init() {
        _isInitialized = true;
        registerMetaTypes(this);
        registerGlobalObject("Script", this);
        qScriptRegisterSequenceMetaType<QVector<QUuid>>(this);
        qScriptRegisterSequenceMetaType<QVector<QString>>(this);
        globalObject().setProperty("OverlayWebWindow", newFunction(QmlWebWindowClass::constructor));
        QScriptValue printConstructorValue = newFunction(debugPrint);
        globalObject().setProperty("print", printConstructorValue);
    }

    void timerFired() {
        QTimer* callingTimer = reinterpret_cast<QTimer*>(sender());
        QScriptValue timerFunction = _timerFunctionMap.value(callingTimer);

        if (!callingTimer->isActive()) {
            // this timer is done, we can kill it
            _timerFunctionMap.remove(callingTimer);
            delete callingTimer;
        }

        // call the associated JS function, if it exists
        if (timerFunction.isValid()) {
            timerFunction.call();
        }
    }


    void run() {
        if (!_isInitialized) {
            init();
        }

        _isRunning = true;
        if (_wantSignals) {
            emit runningStateChanged();
        }

        QScriptValue result = evaluate(_scriptContents, _fileNameString);
        QElapsedTimer startTime;
        startTime.start();

        int thisFrame = 0;

        qint64 lastUpdate = usecTimestampNow();

        while (!_isFinished) {
            int usecToSleep = (thisFrame++ * SCRIPT_DATA_CALLBACK_USECS) - startTime.nsecsElapsed() / 1000; // nsec to usec
            if (usecToSleep > 0) {
                usleep(usecToSleep);
            }

            if (_isFinished) {
                break;
            }

            QCoreApplication::processEvents();
            if (_isFinished) {
                break;
            }

            qint64 now = usecTimestampNow();
            float deltaTime = (float)(now - lastUpdate) / (float)USECS_PER_SECOND;
            if (!_isFinished) {
                if (_wantSignals) {
                    emit update(deltaTime);
                }
            }
            lastUpdate = now;

            // Debug and clear exceptions
            hadUncaughtExceptions(*this, _fileNameString);
        }

        if (_wantSignals) {
            emit scriptEnding();
        }

        if (_wantSignals) {
            emit finished(_fileNameString, this);
        }

        _isRunning = false;

        if (_wantSignals) {
            emit runningStateChanged();
            emit doneRunning();
        }
    }

    void registerGlobalObject(const QString& name, QObject* object) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "registerGlobalObject",
                Q_ARG(const QString&, name),
                Q_ARG(QObject*, object));
            return;
        }
        if (!globalObject().property(name).isValid()) {
            if (object) {
                QScriptValue value = newQObject(object);
                globalObject().setProperty(name, value);
            } else {
                globalObject().setProperty(name, QScriptValue());
            }
        }
    }

    void registerFunction(const QString& name, QScriptEngine::FunctionSignature functionSignature, int numArguments) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "registerFunction",
                Q_ARG(const QString&, name),
                Q_ARG(QScriptEngine::FunctionSignature, functionSignature),
                Q_ARG(int, numArguments));
            return;
        }
        QScriptValue scriptFun = newFunction(functionSignature, numArguments);
        globalObject().setProperty(name, scriptFun);
    }

    void registerFunction(const QString& parent, const QString& name, QScriptEngine::FunctionSignature functionSignature, int numArguments) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "registerFunction",
                Q_ARG(const QString&, name),
                Q_ARG(QScriptEngine::FunctionSignature, functionSignature),
                Q_ARG(int, numArguments));
            return;
        }

        QScriptValue object = globalObject().property(parent);
        if (object.isValid()) {
            QScriptValue scriptFun = newFunction(functionSignature, numArguments);
            object.setProperty(name, scriptFun);
        }
    }

signals:
    void scriptLoaded(const QString& scriptFilename);
    void errorLoadingScript(const QString& scriptFilename);
    void update(float deltaTime);
    void scriptEnding();
    void finished(const QString& fileNameString, ScriptEngine* engine);
    void cleanupMenuItem(const QString& menuItemString);
    void printedMessage(const QString& message);
    void errorMessage(const QString& message);
    void runningStateChanged();
    void evaluationFinished(QScriptValue result, bool isException);
    void loadScript(const QString& scriptName, bool isUserLoaded);
    void reloadScript(const QString& scriptName, bool isUserLoaded);
    void doneRunning();


private:
    QString _scriptContents;
    QString _fileNameString;
    QString _parentURL;
    bool _isInitialized { false };
    std::atomic<bool> _isFinished { false };
    std::atomic<bool> _isRunning { false };
    bool _wantSignals { true };
    QHash<QTimer*, QScriptValue> _timerFunctionMap;
};



ScriptEngine* loadScript(const QString& scriptFilename) {
    ScriptEngine* scriptEngine = new ScriptEngine();
    scriptEngine->loadFile(scriptFilename);
    return scriptEngine;
}

OffscreenGLCanvas* _chromiumShareContext { nullptr };
Q_GUI_EXPORT void qt_gl_set_global_share_context(QOpenGLContext *context);


// Create a simple OpenGL window that renders text in various ways
class QTestWindow : public QWindow {
    Q_OBJECT

    QOpenGLContextWrapper* _context{ nullptr };
    QSize _size;
    bool _altPressed{ false };
    RateCounter fps;
    QTimer _timer;
    int testQmlTexture{ 0 };
    ProgramPtr _program;
    ShapeWrapperPtr _plane;
    QScriptEngine* _scriptEngine { nullptr };

public:
    QObject* rootMenu;

    QTestWindow() {
        _scriptEngine = new ScriptEngine();
        _timer.setInterval(1);
        QObject::connect(&_timer, &QTimer::timeout, this, &QTestWindow::draw);

        _chromiumShareContext = new OffscreenGLCanvas();
        _chromiumShareContext->create();
        _chromiumShareContext->makeCurrent();
        qt_gl_set_global_share_context(_chromiumShareContext->getContext());

        {
            setSurfaceType(QSurface::OpenGLSurface);
            QSurfaceFormat format = getDefaultOpenGLSurfaceFormat();
            setFormat(format);
            _context = new QOpenGLContextWrapper();
            _context->setFormat(format);
            _context->setShareContext(_chromiumShareContext->getContext());
        }


        if (!_context->create()) {
            qFatal("Could not create OpenGL context");
        }

        show();

        makeCurrent();

        glewExperimental = true;
        glewInit();
        glGetError();

        using namespace oglplus;
        Context::Enable(Capability::Blend);
        Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
        Context::Disable(Capability::DepthTest);
        Context::Disable(Capability::CullFace);
        Context::ClearColor(0.2f, 0.2f, 0.2f, 1);

        InfoView::registerType();

        auto offscreenUi = DependencyManager::set<OffscreenUi>();
        {
            offscreenUi->create(_context->getContext());
            offscreenUi->setProxyWindow(this);

            connect(offscreenUi.data(), &OffscreenUi::textureUpdated, this, [this, offscreenUi](int textureId) {
                testQmlTexture = textureId;
            });

            makeCurrent();
        }


        auto primaryScreen = QGuiApplication::primaryScreen();
        auto targetScreen = primaryScreen;
        auto screens = QGuiApplication::screens();
        if (screens.size() > 1) {
            for (auto screen : screens) {
                if (screen != targetScreen) {
                    targetScreen = screen;
                    break;
                }
            }
        }
        auto rect = targetScreen->availableGeometry();
        rect.setWidth(rect.width() * 0.8f);
        rect.setHeight(rect.height() * 0.8f);
        rect.moveTo(QPoint(20, 20));
        setGeometry(rect);

#ifdef QML_CONTROL_GALLERY
        offscreenUi->setBaseUrl(QUrl::fromLocalFile(getTestQmlDir()));
        offscreenUi->load(QUrl("main.qml"));
#else 
        offscreenUi->setBaseUrl(QUrl::fromLocalFile(getInterfaceQmlDir()));
        offscreenUi->load(QUrl("TestRoot.qml"));
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
        auto error = glGetError();
        if (error != GL_NO_ERROR) {
            qDebug() << "GL error in entering draw " << error;
        }

        using namespace oglplus;
        Context::Clear().ColorBuffer().DepthBuffer();
        ivec2 size(_size.width(), _size.height());
        size *= devicePixelRatio();
        size = glm::max(size, ivec2(100, 100));
        Context::Viewport(size.x, size.y);
        if (!_program) {
            _program = loadDefaultShader();
            _plane = loadPlane(_program);
        }

        if (testQmlTexture > 0) {
            glBindTexture(GL_TEXTURE_2D, testQmlTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        _program->Bind();
        _plane->Use();
        _plane->Draw();
        _context->swapBuffers(this);

        fps.increment();
        if (fps.elapsed() >= 10.0f) {
            qDebug() << "FPS: " << fps.rate();
            fps.reset();
        }
    }

    void makeCurrent() {
        _context->makeCurrent(this);
    }

    void resizeWindow(const QSize & size) {
        _size = size;
        DependencyManager::get<OffscreenUi>()->resize(_size);
    }


protected:
    void resizeEvent(QResizeEvent* ev) override {
        resizeWindow(ev->size());
    }

  
    void keyPressEvent(QKeyEvent* event) override {
        _altPressed = Qt::Key_Alt == event->key();
        switch (event->key()) {
            case Qt::Key_B:
                if (event->modifiers() & Qt::CTRL) {
                    auto offscreenUi = DependencyManager::get<OffscreenUi>();
                    offscreenUi->load("Browser.qml");
                }
                break;

            case Qt::Key_J:
                if (event->modifiers() & Qt::CTRL) {
                     loadScript(getExamplesDir() + "tests/qmlWebTest.js");
                }
                break;

            case Qt::Key_K:
                if (event->modifiers() & Qt::CTRL) {
                    OffscreenUi::question("Message title", "Message contents", [](QMessageBox::Button b){
                        qDebug() << b;
                    });
                }
                break;
        }
        QWindow::keyPressEvent(event);
    }

    void moveEvent(QMoveEvent* event) override {
        static qreal oldPixelRatio = 0.0;
        if (devicePixelRatio() != oldPixelRatio) {
            oldPixelRatio = devicePixelRatio();
            resizeWindow(size());
        }
        QWindow::moveEvent(event);
    }
};

const char * LOG_FILTER_RULES = R"V0G0N(
hifi.offscreen.focus.debug=false
qt.quick.mouse.debug=false
)V0G0N";

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = message;

#ifdef Q_OS_WIN
    if (!logMessage.isEmpty()) {
        OutputDebugStringA(logMessage.toLocal8Bit().constData());
        OutputDebugStringA("\n");
    }
#endif
}


int main(int argc, char** argv) {    
    QGuiApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);
    QTestWindow window;
    app.exec();
    return 0;
}

#include "main.moc"
