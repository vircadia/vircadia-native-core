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
#include <QtWebSockets/QWebSocket>
#include <QtWebChannel/QWebChannel>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <AddressManager.h>
#include <DependencyManager.h>

#include "OffscreenUi.h"

QWebSocketServer* QmlWebWindowClass::_webChannelServer { nullptr };
static QWebChannel webChannel;
static const uint16_t WEB_CHANNEL_PORT = 51016;
static std::atomic<int> nextWindowId;
static const char* const URL_PROPERTY = "source";
static const char* const TITLE_PROPERTY = "title";
static const QRegExp HIFI_URL_PATTERN { "^hifi://" };

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


void QmlWebWindowClass::setupServer() {
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

// Method called by Qt scripts to create a new web window in the overlay
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
            setupServer();
            retVal = new QmlWebWindowClass(object);
            webChannel.registerObject(url.toLower(), retVal);
            retVal->setTitle(title);
            retVal->setURL(url);
            retVal->setSize(width, height);
        }));
    connect(engine, &QScriptEngine::destroyed, retVal, &QmlWebWindowClass::deleteLater);
    return engine->newQObject(retVal);
}

QmlWebWindowClass::QmlWebWindowClass(QObject* qmlWindow) 
    : _windowId(++nextWindowId), _qmlWindow(qmlWindow)
{
    qDebug() << "Created window with ID " << _windowId;
    Q_ASSERT(_qmlWindow);
    Q_ASSERT(dynamic_cast<const QQuickItem*>(_qmlWindow));
    QObject::connect(_qmlWindow, SIGNAL(navigating(QString)), this, SLOT(handleNavigation(QString)));
}

void QmlWebWindowClass::handleNavigation(const QString& url) {
    DependencyManager::get<AddressManager>()->handleLookupString(url);
}

void QmlWebWindowClass::setVisible(bool visible) {
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

QQuickItem* QmlWebWindowClass::asQuickItem() const {
    return dynamic_cast<QQuickItem*>(_qmlWindow);
}

bool QmlWebWindowClass::isVisible() const {
    if (QThread::currentThread() != thread()) {
        bool result;
        QMetaObject::invokeMethod(const_cast<QmlWebWindowClass*>(this), "isVisible", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result));
        return result;
    }

    return asQuickItem()->isEnabled();
}


glm::vec2 QmlWebWindowClass::getPosition() const {
    if (QThread::currentThread() != thread()) {
        glm::vec2 result;
        QMetaObject::invokeMethod(const_cast<QmlWebWindowClass*>(this), "getPosition", Qt::BlockingQueuedConnection, Q_RETURN_ARG(glm::vec2, result));
        return result;
    }

    return glm::vec2(asQuickItem()->x(), asQuickItem()->y());
}


void QmlWebWindowClass::setPosition(const glm::vec2& position) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setPosition", Qt::QueuedConnection, Q_ARG(glm::vec2, position));
        return;
    }

    asQuickItem()->setPosition(QPointF(position.x, position.y));
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
    
    return glm::vec2(asQuickItem()->width(), asQuickItem()->height());
}

void QmlWebWindowClass::setSize(const glm::vec2& size) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setSize", Qt::QueuedConnection, Q_ARG(glm::vec2, size));
    }

    asQuickItem()->setSize(QSizeF(size.x, size.y));
}

void QmlWebWindowClass::setSize(int width, int height) {
    setSize(glm::vec2(width, height));
}

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
    // FIXME
}

#include "QmlWebWindowClass.moc"
