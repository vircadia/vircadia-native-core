//
//  InteractiveWindow.cpp
//  libraries/ui/src
//
//  Created by Thijs Wenker on 2018-06-25
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InteractiveWindow.h"

#include <QtQml/QQmlContext>
#include <QtCore/QThread>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickWindow>

#include <DependencyManager.h>
#include <RegisteredMetaTypes.h>

#include "OffscreenUi.h"
#include "shared/QtHelpers.h"
#include "MainWindow.h"

#ifdef Q_OS_WIN
#include <WinUser.h>
#endif

static auto CONTENT_WINDOW_QML = QUrl("InteractiveWindow.qml");

static const char* const FLAGS_PROPERTY = "flags";
static const char* const SOURCE_PROPERTY = "source";
static const char* const TITLE_PROPERTY = "title";
static const char* const POSITION_PROPERTY = "position";
static const char* const INTERACTIVE_WINDOW_POSITION_PROPERTY = "interactiveWindowPosition";
static const char* const SIZE_PROPERTY = "size";
static const char* const INTERACTIVE_WINDOW_SIZE_PROPERTY = "interactiveWindowSize";
static const char* const VISIBLE_PROPERTY = "visible";
static const char* const INTERACTIVE_WINDOW_VISIBLE_PROPERTY = "interactiveWindowVisible";
static const char* const EVENT_BRIDGE_PROPERTY = "eventBridge";
static const char* const PRESENTATION_MODE_PROPERTY = "presentationMode";

static const QStringList KNOWN_SCHEMES = QStringList() << "http" << "https" << "file" << "about" << "atp" << "qrc";

void registerInteractiveWindowMetaType(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, interactiveWindowPointerToScriptValue, interactiveWindowPointerFromScriptValue);
}

QScriptValue interactiveWindowPointerToScriptValue(QScriptEngine* engine, const InteractiveWindowPointer& in) {
    return engine->newQObject(in, QScriptEngine::ScriptOwnership);
}

void interactiveWindowPointerFromScriptValue(const QScriptValue& object, InteractiveWindowPointer& out) {
    if (const auto interactiveWindow = qobject_cast<InteractiveWindowPointer>(object.toQObject())) {
        out = interactiveWindow;
    }
}

InteractiveWindow::InteractiveWindow(const QString& sourceUrl, const QVariantMap& properties) {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    // Build the event bridge and wrapper on the main thread
    offscreenUi->loadInNewContext(CONTENT_WINDOW_QML, [&](QQmlContext* context, QObject* object) {
        _qmlWindow = object;
        context->setContextProperty(EVENT_BRIDGE_PROPERTY, this);
        if (properties.contains(FLAGS_PROPERTY)) {
            object->setProperty(FLAGS_PROPERTY, properties[FLAGS_PROPERTY].toUInt());
        }
        if (properties.contains(PRESENTATION_MODE_PROPERTY)) {
            object->setProperty(PRESENTATION_MODE_PROPERTY, properties[PRESENTATION_MODE_PROPERTY].toInt());
        }
        if (properties.contains(TITLE_PROPERTY)) {
            object->setProperty(TITLE_PROPERTY, properties[TITLE_PROPERTY].toString());
        }
        if (properties.contains(SIZE_PROPERTY)) {
            const auto size = vec2FromVariant(properties[SIZE_PROPERTY]);
            object->setProperty(INTERACTIVE_WINDOW_SIZE_PROPERTY, QSize(size.x, size.y));
        }
        if (properties.contains(POSITION_PROPERTY)) {
            const auto position = vec2FromVariant(properties[POSITION_PROPERTY]);
            object->setProperty(INTERACTIVE_WINDOW_POSITION_PROPERTY, QPointF(position.x, position.y));
        }
        if (properties.contains(VISIBLE_PROPERTY)) {
            object->setProperty(VISIBLE_PROPERTY, properties[INTERACTIVE_WINDOW_VISIBLE_PROPERTY].toBool());
        }

        connect(object, SIGNAL(sendToScript(QVariant)), this, SLOT(qmlToScript(const QVariant&)), Qt::QueuedConnection);
        connect(object, SIGNAL(interactiveWindowPositionChanged()), this, SIGNAL(positionChanged()), Qt::QueuedConnection);
        connect(object, SIGNAL(interactiveWindowSizeChanged()), this, SIGNAL(sizeChanged()), Qt::QueuedConnection);
        connect(object, SIGNAL(interactiveWindowVisibleChanged()), this, SIGNAL(visibleChanged()), Qt::QueuedConnection);
        connect(object, SIGNAL(presentationModeChanged()), this, SIGNAL(presentationModeChanged()), Qt::QueuedConnection);
        connect(object, SIGNAL(titleChanged()), this, SIGNAL(titleChanged()), Qt::QueuedConnection);
        connect(object, SIGNAL(windowClosed()), this, SIGNAL(closed()), Qt::QueuedConnection);
        connect(object, SIGNAL(selfDestruct()), this, SLOT(close()), Qt::QueuedConnection);

#ifdef Q_OS_WIN
        connect(object, SIGNAL(nativeWindowChanged()), this, SLOT(parentNativeWindowToMainWindow()), Qt::QueuedConnection);
        connect(object, SIGNAL(interactiveWindowVisibleChanged()), this, SLOT(parentNativeWindowToMainWindow()), Qt::QueuedConnection);
        connect(object, SIGNAL(presentationModeChanged()), this, SLOT(parentNativeWindowToMainWindow()), Qt::QueuedConnection);
#endif

        QUrl sourceURL{ sourceUrl };
        // If the passed URL doesn't correspond to a known scheme, assume it's a local file path
        if (!KNOWN_SCHEMES.contains(sourceURL.scheme(), Qt::CaseInsensitive)) {
            sourceURL = QUrl::fromLocalFile(sourceURL.toString()).toString();
        }
        object->setProperty(SOURCE_PROPERTY, sourceURL);
    });
}

InteractiveWindow::~InteractiveWindow() {
    close();
}

void InteractiveWindow::sendToQml(const QVariant& message) {
    // Forward messages received from the script on to QML
    QMetaObject::invokeMethod(_qmlWindow, "fromScript", Qt::QueuedConnection, Q_ARG(QVariant, message));
}

void InteractiveWindow::emitScriptEvent(const QVariant& scriptMessage) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitScriptEvent", Qt::QueuedConnection, Q_ARG(QVariant, scriptMessage));
    } else {
        emit scriptEventReceived(scriptMessage);
    }
}

void InteractiveWindow::emitWebEvent(const QVariant& webMessage) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitWebEvent", Qt::QueuedConnection, Q_ARG(QVariant, webMessage));
    } else {
        emit webEventReceived(webMessage);
    }
}

void InteractiveWindow::close() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "close");
        return;
    }

    if (_qmlWindow) {
        _qmlWindow->deleteLater();
    }
    _qmlWindow = nullptr;
}

void InteractiveWindow::show() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "show");
        return;
    }

    if (_qmlWindow) {
        QMetaObject::invokeMethod(_qmlWindow, "show", Qt::DirectConnection);
    }
}

void InteractiveWindow::raise() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "raise");
        return;
    }

    if (_qmlWindow) {
        QMetaObject::invokeMethod(_qmlWindow, "raiseWindow", Qt::DirectConnection);
    }
}

void InteractiveWindow::qmlToScript(const QVariant& message) {
    if (message.canConvert<QJSValue>()) {
        emit fromQml(qvariant_cast<QJSValue>(message).toVariant());
    } else if (message.canConvert<QString>()) {
        emit fromQml(message.toString());
    } else {
        qWarning() << "Unsupported message type " << message;
    }
}

void InteractiveWindow::setVisible(bool visible) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setVisible", Q_ARG(bool, visible));
        return;
    }

    if (!_qmlWindow.isNull()) {
        _qmlWindow->setProperty(INTERACTIVE_WINDOW_VISIBLE_PROPERTY, visible);
    }
}

bool InteractiveWindow::isVisible() const {
    if (QThread::currentThread() != thread()) {
        bool result = false;
        BLOCKING_INVOKE_METHOD(const_cast<InteractiveWindow*>(this), "isVisible", Q_RETURN_ARG(bool, result));
        return result;
    }

    if (_qmlWindow.isNull()) {
        return false;
    }

    return _qmlWindow->property(INTERACTIVE_WINDOW_VISIBLE_PROPERTY).toBool();
}

glm::vec2 InteractiveWindow::getPosition() const {
    if (QThread::currentThread() != thread()) {
        glm::vec2 result;
        BLOCKING_INVOKE_METHOD(const_cast<InteractiveWindow*>(this), "getPosition", Q_RETURN_ARG(glm::vec2, result));
        return result;
    }

    if (_qmlWindow.isNull()) {
        return {};
    }

    return toGlm(_qmlWindow->property(INTERACTIVE_WINDOW_POSITION_PROPERTY).toPointF());
}

void InteractiveWindow::setPosition(const glm::vec2& position) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setPosition", Q_ARG(const glm::vec2&, position));
        return;
    }

    if (!_qmlWindow.isNull()) {
        _qmlWindow->setProperty(INTERACTIVE_WINDOW_POSITION_PROPERTY, QPointF(position.x, position.y));
        QMetaObject::invokeMethod(_qmlWindow, "updateInteractiveWindowPositionForMode", Qt::DirectConnection);
    }
}

glm::vec2 InteractiveWindow::getSize() const {
    if (QThread::currentThread() != thread()) {
        glm::vec2 result;
        BLOCKING_INVOKE_METHOD(const_cast<InteractiveWindow*>(this), "getSize", Q_RETURN_ARG(glm::vec2, result));
        return result;
    }

    if (_qmlWindow.isNull()) {
        return {};
    }
    return toGlm(_qmlWindow->property(INTERACTIVE_WINDOW_SIZE_PROPERTY).toSize());
}

void InteractiveWindow::setSize(const glm::vec2& size) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setSize", Q_ARG(const glm::vec2&, size));
        return;
    }

    if (!_qmlWindow.isNull()) {
        _qmlWindow->setProperty(INTERACTIVE_WINDOW_SIZE_PROPERTY, QSize(size.x, size.y));
        QMetaObject::invokeMethod(_qmlWindow, "updateInteractiveWindowSizeForMode", Qt::DirectConnection);
    }
}

QString InteractiveWindow::getTitle() const {
    if (QThread::currentThread() != thread()) {
        QString result;
        BLOCKING_INVOKE_METHOD(const_cast<InteractiveWindow*>(this), "getTitle", Q_RETURN_ARG(QString, result));
        return result;
    }

    if (_qmlWindow.isNull()) {
        return QString();
    }
    return _qmlWindow->property(TITLE_PROPERTY).toString();
}

void InteractiveWindow::setTitle(const QString& title) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setTitle", Q_ARG(const QString&, title));
        return;
    }

    if (!_qmlWindow.isNull()) {
        _qmlWindow->setProperty(TITLE_PROPERTY, title);
    }
}

int InteractiveWindow::getPresentationMode() const {
    if (QThread::currentThread() != thread()) {
        int result;
        BLOCKING_INVOKE_METHOD(const_cast<InteractiveWindow*>(this), "getPresentationMode",
            Q_RETURN_ARG(int, result));
        return result;
    }

    if (_qmlWindow.isNull()) {
        return Virtual;
    }
    return _qmlWindow->property(PRESENTATION_MODE_PROPERTY).toInt();
}

void InteractiveWindow::parentNativeWindowToMainWindow() {
#ifdef Q_OS_WIN
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "parentNativeWindowToMainWindow");
        return;
    }
    if (_qmlWindow.isNull()) {
        return;
    }
    const auto nativeWindowProperty = _qmlWindow->property("nativeWindow");
    if (nativeWindowProperty.isNull() || !nativeWindowProperty.isValid()) {
        return;
    }
    const auto nativeWindow = qvariant_cast<QQuickWindow*>(nativeWindowProperty);
    SetWindowLongPtr((HWND)nativeWindow->winId(), GWLP_HWNDPARENT, (LONG)MainWindow::findMainWindow()->winId());
#endif
}

void InteractiveWindow::setPresentationMode(int presentationMode) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setPresentationMode", Q_ARG(int, presentationMode));
        return;
    }

    if (!_qmlWindow.isNull()) {
        _qmlWindow->setProperty(PRESENTATION_MODE_PROPERTY, presentationMode);
    }
}
