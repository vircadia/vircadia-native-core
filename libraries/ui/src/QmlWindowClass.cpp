//
//  Created by Bradley Austin Davis on 2015-12-15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlWindowClass.h"

#include <mutex>

#include <QtCore/QThread>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>

#include <QtQuick/QQuickItem>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>
#include <QtWebChannel/QWebChannel>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "OffscreenUi.h"

QWebSocketServer* QmlWindowClass::_webChannelServer { nullptr };
static QWebChannel webChannel;
static const uint16_t WEB_CHANNEL_PORT = 51016;
static std::atomic<int> nextWindowId;
static const char* const SOURCE_PROPERTY = "source";
static const char* const TITLE_PROPERTY = "title";
static const char* const WIDTH_PROPERTY = "width";
static const char* const HEIGHT_PROPERTY = "height";
static const char* const VISIBILE_PROPERTY = "visible";

void QmlScriptEventBridge::emitWebEvent(const QString& data) {
    QMetaObject::invokeMethod(this, "webEventReceived", Qt::QueuedConnection, Q_ARG(QString, data));
}

void QmlScriptEventBridge::emitScriptEvent(const QString& data) {
    QMetaObject::invokeMethod(this, "scriptEventReceived", Qt::QueuedConnection, 
        Q_ARG(int, _webWindow->getWindowId()), Q_ARG(QString, data));
}

class QmlWebTransport : public QWebChannelAbstractTransport {
    Q_OBJECT
public:
    QmlWebTransport(QWebSocket* webSocket) : _webSocket(webSocket) {
        // Translate from the websocket layer to the webchannel layer
        connect(webSocket, &QWebSocket::textMessageReceived, [this](const QString& message) {
            QJsonParseError error;
            QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &error);
            if (error.error || !document.isObject()) {
                qWarning() << "Unable to parse incoming JSON message" << message;
                return;
            }
            emit messageReceived(document.object(), this);
        });
    }

    virtual void sendMessage(const QJsonObject &message) override {
        // Translate from the webchannel layer to the websocket layer
        _webSocket->sendTextMessage(QJsonDocument(message).toJson(QJsonDocument::Compact));
    }

private:
    QWebSocket* const _webSocket;
};


void QmlWindowClass::setupServer() {
    if (!_webChannelServer) {
        _webChannelServer = new QWebSocketServer("EventBridge Server", QWebSocketServer::NonSecureMode);
        if (!_webChannelServer->listen(QHostAddress::LocalHost, WEB_CHANNEL_PORT)) {
            qFatal("Failed to open web socket server.");
        }

        QObject::connect(_webChannelServer, &QWebSocketServer::newConnection, [] {
            webChannel.connectTo(new QmlWebTransport(_webChannelServer->nextPendingConnection()));
        });
    }
}

QScriptValue QmlWindowClass::internalConstructor(const QString& qmlSource, 
    QScriptContext* context, QScriptEngine* engine, 
    std::function<QmlWindowClass*(QQmlContext*, QObject*)> function) 
{
    const auto argumentCount = context->argumentCount();
    QString url;
    QString title;
    int width = 100, height = 100;
    bool visible = true;
    if (argumentCount > 1) {

        if (!context->argument(0).isUndefined()) {
            title = context->argument(0).toString();
        }
        if (!context->argument(1).isUndefined()) {
            url = context->argument(1).toString();
        }
        if (context->argument(2).isNumber()) {
            width = context->argument(2).toInt32();
        }
        if (context->argument(3).isNumber()) {
            height = context->argument(3).toInt32();
        }
    } else {
        auto argumentObject = context->argument(0);
        qDebug() << argumentObject.toString();
        if (!argumentObject.property(TITLE_PROPERTY).isUndefined()) {
            title = argumentObject.property(TITLE_PROPERTY).toString();
        }
        if (!argumentObject.property(SOURCE_PROPERTY).isUndefined()) {
            url = argumentObject.property(SOURCE_PROPERTY).toString();
        }
        if (argumentObject.property(WIDTH_PROPERTY).isNumber()) {
            width = argumentObject.property(WIDTH_PROPERTY).toInt32();
        }
        if (argumentObject.property(HEIGHT_PROPERTY).isNumber()) {
            height = argumentObject.property(HEIGHT_PROPERTY).toInt32();
        }
        if (argumentObject.property(VISIBILE_PROPERTY).isBool()) {
            visible = argumentObject.property(VISIBILE_PROPERTY).toBool();
        }
    }

    if (!url.startsWith("http") && !url.startsWith("file://") && !url.startsWith("about:")) {
        url = QUrl::fromLocalFile(url).toString();
    }

    width = std::max(100, std::min(1280, width));
    height = std::max(100, std::min(720, height));

    QmlWindowClass* retVal{ nullptr };

    // Build the event bridge and wrapper on the main thread
    QMetaObject::invokeMethod(DependencyManager::get<OffscreenUi>().data(), "load", Qt::BlockingQueuedConnection,
        Q_ARG(const QString&, qmlSource),
        Q_ARG(std::function<void(QQmlContext*, QObject*)>, [&](QQmlContext* context, QObject* object) {
            setupServer();
            retVal = function(context, object);
            context->engine()->setObjectOwnership(retVal->_qmlWindow, QQmlEngine::CppOwnership);
            registerObject(url.toLower(), retVal);
            if (!title.isEmpty()) {
                retVal->setTitle(title);
            }
            retVal->setSize(width, height);
            object->setProperty(SOURCE_PROPERTY, url);
            if (visible) {
                object->setProperty("enabled", true);
            }
        }));
    connect(engine, &QScriptEngine::destroyed, retVal, &QmlWindowClass::deleteLater);
    return engine->newQObject(retVal);
}


// Method called by Qt scripts to create a new web window in the overlay
QScriptValue QmlWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    return internalConstructor("QmlWindow.qml", context, engine, [&](QQmlContext* context, QObject* object){
        return new QmlWindowClass(object);
    });
}

QmlWindowClass::QmlWindowClass(QObject* qmlWindow)
    : _windowId(++nextWindowId), _qmlWindow(qmlWindow)
{
    qDebug() << "Created window with ID " << _windowId;
    Q_ASSERT(_qmlWindow);
    Q_ASSERT(dynamic_cast<const QQuickItem*>(_qmlWindow));
}

void QmlWindowClass::registerObject(const QString& name, QObject* object) {
    webChannel.registerObject(name, object);
}

void QmlWindowClass::deregisterObject(QObject* object) {
    webChannel.deregisterObject(object);
}

void QmlWindowClass::setVisible(bool visible) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setVisible", Qt::AutoConnection, Q_ARG(bool, visible));
        return;
    }

    auto qmlWindow = asQuickItem();
    if (qmlWindow->isEnabled() != visible) {
        qmlWindow->setEnabled(visible);
        emit visibilityChanged(visible);
    }
}

QQuickItem* QmlWindowClass::asQuickItem() const {
    return dynamic_cast<QQuickItem*>(_qmlWindow);
}

bool QmlWindowClass::isVisible() const {
    if (QThread::currentThread() != thread()) {
        bool result;
        QMetaObject::invokeMethod(const_cast<QmlWindowClass*>(this), "isVisible", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result));
        return result;
    }

    return asQuickItem()->isEnabled();
}


glm::vec2 QmlWindowClass::getPosition() const {
    if (QThread::currentThread() != thread()) {
        glm::vec2 result;
        QMetaObject::invokeMethod(const_cast<QmlWindowClass*>(this), "getPosition", Qt::BlockingQueuedConnection, Q_RETURN_ARG(glm::vec2, result));
        return result;
    }

    return glm::vec2(asQuickItem()->x(), asQuickItem()->y());
}


void QmlWindowClass::setPosition(const glm::vec2& position) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setPosition", Qt::QueuedConnection, Q_ARG(glm::vec2, position));
        return;
    }

    asQuickItem()->setPosition(QPointF(position.x, position.y));
}

void QmlWindowClass::setPosition(int x, int y) {
    setPosition(glm::vec2(x, y));
}

glm::vec2 QmlWindowClass::getSize() const {
    if (QThread::currentThread() != thread()) {
        glm::vec2 result;
        QMetaObject::invokeMethod(const_cast<QmlWindowClass*>(this), "getSize", Qt::BlockingQueuedConnection, Q_RETURN_ARG(glm::vec2, result));
        return result;
    }
    
    return glm::vec2(asQuickItem()->width(), asQuickItem()->height());
}

void QmlWindowClass::setSize(const glm::vec2& size) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setSize", Qt::QueuedConnection, Q_ARG(glm::vec2, size));
    }

    asQuickItem()->setSize(QSizeF(size.x, size.y));
}

void QmlWindowClass::setSize(int width, int height) {
    setSize(glm::vec2(width, height));
}

void QmlWindowClass::setTitle(const QString& title) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setTitle", Qt::QueuedConnection, Q_ARG(QString, title));
    }

    _qmlWindow->setProperty(TITLE_PROPERTY, title);
}

void QmlWindowClass::close() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
    }
    _qmlWindow->setProperty("destroyOnInvisible", true);
    _qmlWindow->setProperty("visible", false);
    _qmlWindow->deleteLater();
}

void QmlWindowClass::hasClosed() {
}

void QmlWindowClass::raise() {
    QMetaObject::invokeMethod(_qmlWindow, "raiseWindow", Qt::QueuedConnection);
}

#include "QmlWindowClass.moc"
