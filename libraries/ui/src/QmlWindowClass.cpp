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

static const char* const SOURCE_PROPERTY = "source";
static const char* const TITLE_PROPERTY = "title";
static const char* const WIDTH_PROPERTY = "width";
static const char* const HEIGHT_PROPERTY = "height";
static const char* const VISIBILE_PROPERTY = "visible";
static const char* const TOOLWINDOW_PROPERTY = "toolWindow";

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
            retVal = builder(newTab);
            retVal->_toolWindow = true;
            return QVariant();
        });
    } else {
        // Build the event bridge and wrapper on the main thread
        QMetaObject::invokeMethod(offscreenUi.data(), "load", Qt::BlockingQueuedConnection,
            Q_ARG(const QString&, qmlSource),
            Q_ARG(std::function<void(QQmlContext*, QObject*)>, [&](QQmlContext* context, QObject* object) {
            retVal = builder(object);
            context->engine()->setObjectOwnership(retVal->_qmlWindow, QQmlEngine::CppOwnership);
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

QmlWindowClass::QmlWindowClass(QObject* qmlWindow) : _qmlWindow(qmlWindow) {
    Q_ASSERT(_qmlWindow);
    Q_ASSERT(dynamic_cast<const QQuickItem*>(_qmlWindow.data()));
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

QQuickItem* QmlWindowClass::asQuickItem() const {
    if (_toolWindow) {
        return DependencyManager::get<OffscreenUi>()->getToolWindow();
    }
    return _qmlWindow.isNull() ? nullptr : dynamic_cast<QQuickItem*>(_qmlWindow.data());
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
    return DependencyManager::get<OffscreenUi>()->returnFromUiThread([&] {
        if (_qmlWindow.isNull()) {
            return QVariant::fromValue(false);
        }
        if (_toolWindow) {
            return QVariant::fromValue(dynamic_cast<QQuickItem*>(_qmlWindow.data())->isEnabled());
        } else {
            return QVariant::fromValue(asQuickItem()->isVisible());
        }
    }).toBool();
}

glm::vec2 QmlWindowClass::getPosition() const {
    QVariant result = DependencyManager::get<OffscreenUi>()->returnFromUiThread([&]()->QVariant {
        if (_qmlWindow.isNull()) {
            return QVariant(QPointF(0, 0));
        }
        return asQuickItem()->position();
    });
    return toGlm(result.toPointF());
}


void QmlWindowClass::setPosition(const glm::vec2& position) {
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        if (!_qmlWindow.isNull()) {
            asQuickItem()->setPosition(QPointF(position.x, position.y));
        }
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
    QVariant result = DependencyManager::get<OffscreenUi>()->returnFromUiThread([&]()->QVariant {
        if (_qmlWindow.isNull()) {
            return QVariant(QSizeF(0, 0));
        }
        QQuickItem* targetWindow = asQuickItem();
        return QSizeF(targetWindow->width(), targetWindow->height());
    });
    return toGlm(result.toSizeF());
}

void QmlWindowClass::setSize(const glm::vec2& size) {
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        if (!_qmlWindow.isNull()) {
            asQuickItem()->setSize(QSizeF(size.x, size.y));
        }
    });
}

void QmlWindowClass::setSize(int width, int height) {
    setSize(glm::vec2(width, height));
}

void QmlWindowClass::setTitle(const QString& title) {
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        if (!_qmlWindow.isNull()) {
            asQuickItem()->setProperty(TITLE_PROPERTY, title);
        }
    });
}

void QmlWindowClass::close() {
    if (_qmlWindow) {
        if (_toolWindow) {
            auto offscreenUi = DependencyManager::get<OffscreenUi>();
            offscreenUi->executeOnUiThread([=] {
                auto toolWindow = offscreenUi->getToolWindow();
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
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->executeOnUiThread([=] {
        if (!_qmlWindow.isNull()) {
            QMetaObject::invokeMethod(asQuickItem(), "raise", Qt::DirectConnection);
        }
    });
}

#include "QmlWindowClass.moc"
