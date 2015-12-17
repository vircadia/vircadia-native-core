//
//  Created by Bradley Austin Davis on 2015-12-15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlWebWindowClass.h"

#include <mutex>

#include <QtCore/QThread>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>

#include <QtQuick/QQuickItem>

#include <QtWebSockets/QWebSocketServer>
#include <QtWebChannel/QWebChannel>

#include <DependencyManager.h>

#include "impl/websocketclientwrapper.h"
#include "impl/websockettransport.h"
#include "OffscreenUi.h"

static QWebSocketServer * webChannelServer { nullptr };
static WebSocketClientWrapper * webChannelClientWrapper { nullptr };
static QWebChannel webChannel;
static std::once_flag webChannelSetup;
static const uint16_t WEB_CHANNEL_PORT = 51016;
static std::atomic<int> nextWindowId;

void initWebChannelServer() {
    std::call_once(webChannelSetup, [] {
        webChannelServer = new QWebSocketServer("EventBridge Server", QWebSocketServer::NonSecureMode);
        webChannelClientWrapper = new WebSocketClientWrapper(webChannelServer);
        if (!webChannelServer->listen(QHostAddress::LocalHost, WEB_CHANNEL_PORT)) {
                qFatal("Failed to open web socket server.");
        }
        QObject::connect(webChannelClientWrapper, &WebSocketClientWrapper::clientConnected, &webChannel, &QWebChannel::connectTo);
    });
}

void QmlScriptEventBridge::emitWebEvent(const QString& data) {
    QMetaObject::invokeMethod(this, "webEventReceived", Qt::QueuedConnection, Q_ARG(QString, data));
}

void QmlScriptEventBridge::emitScriptEvent(const QString& data) {
    QMetaObject::invokeMethod(this, "scriptEventReceived", Qt::QueuedConnection, 
        Q_ARG(int, _webWindow->getWindowId()),
        Q_ARG(QString, data)
    );
}

QScriptValue QmlWebWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    QmlWebWindowClass* retVal { nullptr };
    const QString title = context->argument(0).toString();
    QString url = context->argument(1).toString();
    if (!url.startsWith("http") && !url.startsWith("file://")) {
        url = QUrl::fromLocalFile(url).toString();
    }
    const int width = std::max(100, std::min(1280, context->argument(2).toInt32()));;
    const int height = std::max(100, std::min(720, context->argument(3).toInt32()));;

    // Build the event bridge and wrapper on the main thread
    QMetaObject::invokeMethod(DependencyManager::get<OffscreenUi>().data(), "load", Qt::BlockingQueuedConnection,
        Q_ARG(const QString&, "QmlWebWindow.qml"),
        Q_ARG(std::function<void(QQmlContext*, QObject*)>, [&](QQmlContext* context, QObject* object) {
            initWebChannelServer();
            retVal = new QmlWebWindowClass(object);
            webChannel.registerObject(url.toLower(), retVal);
            retVal->setTitle(title);
            retVal->setURL(url);
            retVal->setSize(width, height);
        })
    );
    connect(engine, &QScriptEngine::destroyed, retVal, &QmlWebWindowClass::deleteLater);
    return engine->newQObject(retVal);
}

QmlWebWindowClass::QmlWebWindowClass(QObject* qmlWindow) 
    : _isToolWindow(false), _windowId(++nextWindowId), _eventBridge(new QmlScriptEventBridge(this)), _qmlWindow(qmlWindow)
{
    qDebug() << "Created window with ID " << _windowId;
    Q_ASSERT(_qmlWindow);
    Q_ASSERT(dynamic_cast<const QQuickItem*>(_qmlWindow));
}

void QmlWebWindowClass::setVisible(bool visible) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setVisible", Qt::AutoConnection, Q_ARG(bool, visible));
        return;
    }

    auto qmlWindow = (QQuickItem*)_qmlWindow;
    if (qmlWindow->isEnabled() != visible) {
        qmlWindow->setEnabled(visible);
        emit visibilityChanged(visible);
    }
}

bool QmlWebWindowClass::isVisible() const {
    if (QThread::currentThread() != thread()) {
        bool result;
        QMetaObject::invokeMethod(const_cast<QmlWebWindowClass*>(this), "isVisible", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result));
        return result;
    }

    return ((QQuickItem*)_qmlWindow)->isEnabled();
}


glm::vec2 QmlWebWindowClass::getPosition() const {
    if (QThread::currentThread() != thread()) {
        glm::vec2 result;
        QMetaObject::invokeMethod(const_cast<QmlWebWindowClass*>(this), "getPosition", Qt::BlockingQueuedConnection, Q_RETURN_ARG(glm::vec2, result));
        return result;
    }

    return glm::vec2(((QQuickItem*)_qmlWindow)->x(), ((QQuickItem*)_qmlWindow)->y());
}


void QmlWebWindowClass::setPosition(const glm::vec2& position) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setPosition", Qt::QueuedConnection, Q_ARG(glm::vec2, position));
        return;
    }

    ((QQuickItem*)_qmlWindow)->setPosition(QPointF(position.x, position.y));
}

void QmlWebWindowClass::setPosition(int x, int y) {
    setPosition(glm::vec2(x, y));
}

glm::vec2 QmlWebWindowClass::getSize() const {
    if (QThread::currentThread() != thread()) {
        glm::vec2 result;
        QMetaObject::invokeMethod(const_cast<QmlWebWindowClass*>(this), "getSize", Qt::BlockingQueuedConnection, Q_RETURN_ARG(glm::vec2, result));
        return result;
    }
    
    return glm::vec2(((QQuickItem*)_qmlWindow)->width(), ((QQuickItem*)_qmlWindow)->height());
}

void QmlWebWindowClass::setSize(const glm::vec2& size) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setSize", Qt::QueuedConnection, Q_ARG(glm::vec2, size));
    }

    ((QQuickItem*)_qmlWindow)->setSize(QSizeF(size.x, size.y));
}

void QmlWebWindowClass::setSize(int width, int height) {
    setSize(glm::vec2(width, height));
}

static const char* const URL_PROPERTY = "source";

QString QmlWebWindowClass::getURL() const { 
    if (QThread::currentThread() != thread()) {
        QString result;
        QMetaObject::invokeMethod(const_cast<QmlWebWindowClass*>(this), "getURL", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, result));
        return result;
    }
    return _qmlWindow->property(URL_PROPERTY).toString();
}

void QmlWebWindowClass::setURL(const QString& urlString) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setURL", Qt::QueuedConnection, Q_ARG(QString, urlString));
    }
    _qmlWindow->setProperty(URL_PROPERTY, urlString);
}

static const char* const TITLE_PROPERTY = "title";

void QmlWebWindowClass::setTitle(const QString& title) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setTitle", Qt::QueuedConnection, Q_ARG(QString, title));
    }

    _qmlWindow->setProperty(TITLE_PROPERTY, title);
}

void QmlWebWindowClass::close() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
    }
    _qmlWindow->setProperty("destroyOnInvisible", true);
    _qmlWindow->setProperty("visible", false);
    _qmlWindow->deleteLater();
}

void QmlWebWindowClass::hasClosed() {
}

void QmlWebWindowClass::raise() {
}

#if 0

#include <QtCore/QThread>

#include <QtScript/QScriptEngine>

WebWindowClass::WebWindowClass(const QString& title, const QString& url, int width, int height, bool isToolWindow)
    : QObject(NULL), _eventBridge(new ScriptEventBridge(this)), _isToolWindow(isToolWindow) {
    /*
    if (_isToolWindow) {
        ToolWindow* toolWindow = qApp->getToolWindow();

        auto dockWidget = new QDockWidget(title, toolWindow);
        dockWidget->setFeatures(QDockWidget::DockWidgetMovable);
        connect(dockWidget, &QDockWidget::visibilityChanged, this, &WebWindowClass::visibilityChanged);

        _webView = new QWebView(dockWidget);
        addEventBridgeToWindowObject();

        dockWidget->setWidget(_webView);

        auto titleWidget = new QWidget(dockWidget);
        dockWidget->setTitleBarWidget(titleWidget);

        toolWindow->addDockWidget(Qt::TopDockWidgetArea, dockWidget, Qt::Horizontal);

        _windowWidget = dockWidget;
    } else {
        auto dialogWidget = new QDialog(qApp->getWindow(), Qt::Window);
        dialogWidget->setWindowTitle(title);
        dialogWidget->resize(width, height);
        dialogWidget->installEventFilter(this);
        connect(dialogWidget, &QDialog::finished, this, &WebWindowClass::hasClosed);

        auto layout = new QVBoxLayout(dialogWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        dialogWidget->setLayout(layout);

        _webView = new QWebView(dialogWidget);

        layout->addWidget(_webView);

        addEventBridgeToWindowObject();

        _windowWidget = dialogWidget;
    }

    auto style = QStyleFactory::create("fusion");
    if (style) {
    _webView->setStyle(style);
    }

    _webView->setPage(new DataWebPage());
    if (!url.startsWith("http") && !url.startsWith("file://")) {
    _webView->setUrl(QUrl::fromLocalFile(url));
    } else {
    _webView->setUrl(url);
    }
    connect(this, &WebWindowClass::destroyed, _windowWidget, &QWidget::deleteLater);
    connect(_webView->page()->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared,
    this, &WebWindowClass::addEventBridgeToWindowObject);
    */
}

void WebWindowClass::hasClosed() {
    emit closed();
}


void WebWindowClass::setVisible(bool visible) {
}

QString WebWindowClass::getURL() const {
    return QString();
}

void WebWindowClass::setURL(const QString& url) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setURL", Qt::AutoConnection, Q_ARG(QString, url));
        return;
    }
}

QSizeF WebWindowClass::getSize() const {
    QSizeF size;
    return size;
}

void WebWindowClass::setSize(const QSizeF& size) {
    setSize(size.width(), size.height());
}

void WebWindowClass::setSize(int width, int height) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setSize", Qt::AutoConnection, Q_ARG(int, width), Q_ARG(int, height));
        return;
    }
}

glm::vec2 WebWindowClass::getPosition() const {
    return glm::vec2();
}

void WebWindowClass::setPosition(const glm::vec2& position) {
    setPosition(position.x, position.y);
}

void WebWindowClass::setPosition(int x, int y) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setPosition", Qt::AutoConnection, Q_ARG(int, x), Q_ARG(int, y));
        return;
    }
}

void WebWindowClass::raise() {
}

QScriptValue WebWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    WebWindowClass* retVal { nullptr };
    //QString file = context->argument(0).toString();
    //QMetaObject::invokeMethod(DependencyManager::get<WindowScriptingInterface>().data(), "doCreateWebWindow", Qt::BlockingQueuedConnection,
    //        Q_RETURN_ARG(WebWindowClass*, retVal),
    //        Q_ARG(const QString&, file),
    //        Q_ARG(QString, context->argument(1).toString()),
    //        Q_ARG(int, context->argument(2).toInteger()),
    //        Q_ARG(int, context->argument(3).toInteger()),
    //        Q_ARG(bool, context->argument(4).toBool()));

    //connect(engine, &QScriptEngine::destroyed, retVal, &WebWindowClass::deleteLater);

    return engine->newQObject(retVal);
}

void WebWindowClass::setTitle(const QString& title) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setTitle", Qt::AutoConnection, Q_ARG(QString, title));
        return;
    }
}

#endif