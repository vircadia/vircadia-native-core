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
static const char* const TOOLWINDOW_PROPERTY = "toolWindow";

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
    std::function<QmlWindowClass*(QObject*)> builder) 
{
    const auto argumentCount = context->argumentCount();
    QString url;
    QString title;
    int width = -1, height = -1;
    bool visible = true;
    bool toolWindow = false;
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
        if (context->argument(4).isBool()) {
            toolWindow = context->argument(4).toBool();
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
        if (argumentObject.property(TOOLWINDOW_PROPERTY).isBool()) {
            toolWindow = argumentObject.property(TOOLWINDOW_PROPERTY).toBool();
        }
    }

    if (!url.startsWith("http") && !url.startsWith("file://") && !url.startsWith("about:")) {
        url = QUrl::fromLocalFile(url).toString();
    }

    if (width != -1 || height != -1) {
        width = std::max(100, std::min(1280, width));
        height = std::max(100, std::min(720, height));
    }

    QmlWindowClass* retVal{ nullptr };
    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    if (toolWindow) {
        auto toolWindow = offscreenUi->getToolWindow();
        QVariantMap properties;
        properties.insert(TITLE_PROPERTY, title);
        properties.insert(SOURCE_PROPERTY, url);
        if (width != -1 && height != -1) {
            properties.insert(WIDTH_PROPERTY, width);
            properties.insert(HEIGHT_PROPERTY, height);
        }

        // Build the event bridge and wrapper on the main thread
        QVariant newTabVar;
        bool invokeResult = QMetaObject::invokeMethod(toolWindow, "addWebTab", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QVariant, newTabVar),
            Q_ARG(QVariant, QVariant::fromValue(properties)));

        QQuickItem* newTab = qvariant_cast<QQuickItem*>(newTabVar);
        if (!invokeResult || !newTab) {
            return QScriptValue();
        }

        offscreenUi->returnFromUiThread([&] {
            setupServer();
            retVal = builder(newTab);
            retVal->_toolWindow = true;
            offscreenUi->getRootContext()->engine()->setObjectOwnership(retVal->_qmlWindow, QQmlEngine::CppOwnership);
            registerObject(url.toLower(), retVal);
            return QVariant();
        });
    } else {
        // Build the event bridge and wrapper on the main thread
        QMetaObject::invokeMethod(offscreenUi.data(), "load", Qt::BlockingQueuedConnection,
            Q_ARG(const QString&, qmlSource),
            Q_ARG(std::function<void(QQmlContext*, QObject*)>, [&](QQmlContext* context, QObject* object) {
            setupServer();
            retVal = builder(object);
            context->engine()->setObjectOwnership(retVal->_qmlWindow, QQmlEngine::CppOwnership);
            registerObject(url.toLower(), retVal);
            if (!title.isEmpty()) {
                retVal->setTitle(title);
            }
            if (width != -1 && height != -1) {
                retVal->setSize(width, height);
            }
            object->setProperty(SOURCE_PROPERTY, url);
            if (visible) {
                object->setProperty("visible", true);
            }
        }));
    }

    retVal->_source = url;
    connect(engine, &QScriptEngine::destroyed, retVal, &QmlWindowClass::deleteLater);
    return engine->newQObject(retVal);
}


// Method called by Qt scripts to create a new web window in the overlay
QScriptValue QmlWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    return internalConstructor("QmlWindow.qml", context, engine, [&](QObject* object){
        return new QmlWindowClass(object);
    });
}

QmlWindowClass::QmlWindowClass(QObject* qmlWindow)
    : _windowId(++nextWindowId), _qmlWindow(qmlWindow)
{
    qDebug() << "Created window with ID " << _windowId;
    Q_ASSERT(_qmlWindow);
    Q_ASSERT(dynamic_cast<const QQuickItem*>(_qmlWindow));
    // Forward messages received from QML on to the script
    connect(_qmlWindow, SIGNAL(sendToScript(QVariant)), this, SIGNAL(fromQml(const QVariant&)), Qt::QueuedConnection);
}

void QmlWindowClass::sendToQml(const QVariant& message) {
    // Forward messages received from the script on to QML
    QMetaObject::invokeMethod(asQuickItem(), "fromScript", Qt::QueuedConnection, Q_ARG(QVariant, message));
}

QmlWindowClass::~QmlWindowClass() {
    close();
}

void QmlWindowClass::registerObject(const QString& name, QObject* object) {
    webChannel.registerObject(name, object);
}

void QmlWindowClass::deregisterObject(QObject* object) {
    webChannel.deregisterObject(object);
}

QQuickItem* QmlWindowClass::asQuickItem() const {
    if (_toolWindow) {
        return DependencyManager::get<OffscreenUi>()->getToolWindow();
    }
    return dynamic_cast<QQuickItem*>(_qmlWindow);
}

void QmlWindowClass::setVisible(bool visible) {
    QQuickItem* targetWindow = asQuickItem();
    if (_toolWindow) {
        // For tool window tabs we special case visibility as a function call on the tab parent
        // The tool window itself has special logic based on whether any tabs are visible
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        QMetaObject::invokeMethod(targetWindow, "showTabForUrl", Qt::QueuedConnection, Q_ARG(QVariant, _source), Q_ARG(QVariant, visible));
    } else {
        DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
            targetWindow->setVisible(visible);
            //emit visibilityChanged(visible);
        });
    }
}

bool QmlWindowClass::isVisible() const {
    // The tool window itself has special logic based on whether any tabs are enabled
    if (_toolWindow) {
        auto targetTab = dynamic_cast<QQuickItem*>(_qmlWindow);
        return DependencyManager::get<OffscreenUi>()->returnFromUiThread([&] {
            return QVariant::fromValue(targetTab->isEnabled());
        }).toBool();
    } else {
        QQuickItem* targetWindow = asQuickItem();
        return DependencyManager::get<OffscreenUi>()->returnFromUiThread([&] {
            return QVariant::fromValue(targetWindow->isVisible());
        }).toBool();
    }
}

glm::vec2 QmlWindowClass::getPosition() const {
    QQuickItem* targetWindow = asQuickItem();
    QVariant result = DependencyManager::get<OffscreenUi>()->returnFromUiThread([&]()->QVariant {
        return targetWindow->position();
    });
    return toGlm(result.toPointF());
}


void QmlWindowClass::setPosition(const glm::vec2& position) {
    QQuickItem* targetWindow = asQuickItem();
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        targetWindow->setPosition(QPointF(position.x, position.y));
    });
}

void QmlWindowClass::setPosition(int x, int y) {
    setPosition(glm::vec2(x, y));
}

// FIXME move to GLM helpers
glm::vec2 toGlm(const QSizeF& size) {
    return glm::vec2(size.width(), size.height());
}

glm::vec2 QmlWindowClass::getSize() const {
    QQuickItem* targetWindow = asQuickItem();
    QVariant result = DependencyManager::get<OffscreenUi>()->returnFromUiThread([&]()->QVariant {
        return QSizeF(targetWindow->width(), targetWindow->height());
    });
    return toGlm(result.toSizeF());
}

void QmlWindowClass::setSize(const glm::vec2& size) {
    QQuickItem* targetWindow = asQuickItem();
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        targetWindow->setSize(QSizeF(size.x, size.y));
    });
}

void QmlWindowClass::setSize(int width, int height) {
    setSize(glm::vec2(width, height));
}

void QmlWindowClass::setTitle(const QString& title) {
    QQuickItem* targetWindow = asQuickItem();
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        targetWindow->setProperty(TITLE_PROPERTY, title);
    });
}

void QmlWindowClass::close() {
    if (_qmlWindow) {
        if (_toolWindow) {
            auto offscreenUi = DependencyManager::get<OffscreenUi>();
            auto qmlWindow = _qmlWindow;
            offscreenUi->executeOnUiThread([=] {
                auto toolWindow = offscreenUi->getToolWindow();
                offscreenUi->getRootContext()->engine()->setObjectOwnership(qmlWindow, QQmlEngine::JavaScriptOwnership);
                auto invokeResult = QMetaObject::invokeMethod(toolWindow, "removeTabForUrl", Qt::DirectConnection,
                    Q_ARG(QVariant, _source));
                Q_ASSERT(invokeResult);
            });
        } else {
            _qmlWindow->deleteLater();
        }
        _qmlWindow = nullptr;
    }
}

void QmlWindowClass::hasClosed() {
}

void QmlWindowClass::raise() {
    QMetaObject::invokeMethod(asQuickItem(), "raise", Qt::QueuedConnection);
}

#include "QmlWindowClass.moc"
