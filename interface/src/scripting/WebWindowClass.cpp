//
//  WebWindowClass.cpp
//  interface/src/scripting
//
//  Created by Ryan Huffman on 11/06/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QVBoxLayout>
#include <QApplication>
#include <QMainWindow>
#include <QDockWidget>
#include <QWebEngineView>
#include <QListWidget>
#include <QStyleFactory>

#include "Application.h"
#include "ui/DataWebPage.h"
#include "MainWindow.h"
#include "WebWindowClass.h"
#include "WindowScriptingInterface.h"

ScriptEventBridge::ScriptEventBridge(QObject* parent) : QObject(parent) {
}

void ScriptEventBridge::emitWebEvent(const QString& data) {
    emit webEventReceived(data);
}

void ScriptEventBridge::emitScriptEvent(const QString& data) {
    emit scriptEventReceived(data);
}

WebWindowClass::WebWindowClass(const QString& title, const QString& url, int width, int height)
    : QObject(NULL), _eventBridge(new ScriptEventBridge(this)) {
    auto dialogWidget = new QDialog(qApp->getWindow(), Qt::Window);
    dialogWidget->setWindowTitle(title);
    dialogWidget->resize(width, height);
    dialogWidget->installEventFilter(this);
    connect(dialogWidget, &QDialog::finished, this, &WebWindowClass::hasClosed);

    auto layout = new QVBoxLayout(dialogWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    dialogWidget->setLayout(layout);

    _webView = new QWebEngineView(dialogWidget);

    layout->addWidget(_webView);

    addEventBridgeToWindowObject();

    _windowWidget = dialogWidget;

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
    //connect(_webView->page()->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared,
    //        this, &WebWindowClass::addEventBridgeToWindowObject);
}

WebWindowClass::~WebWindowClass() {
}

bool WebWindowClass::eventFilter(QObject* sender, QEvent* event) {
    if (sender == _windowWidget) {
        if (event->type() == QEvent::Move) {
            emit moved(getPosition());
        }
        if (event->type() == QEvent::Resize) {
            emit resized(getSize());
        }
    }

    return false;
}

void WebWindowClass::hasClosed() {
    emit closed();
}

void WebWindowClass::addEventBridgeToWindowObject() {
//    _webView->page()->mainFrame()->addToJavaScriptWindowObject("EventBridge", _eventBridge);
}

void WebWindowClass::setVisible(bool visible) {
    if (visible) {
        QMetaObject::invokeMethod(_windowWidget, "showNormal", Qt::AutoConnection);
        QMetaObject::invokeMethod(_windowWidget, "raise", Qt::AutoConnection);
    }
    QMetaObject::invokeMethod(_windowWidget, "setVisible", Qt::AutoConnection, Q_ARG(bool, visible));
}

void WebWindowClass::setURL(const QString& url) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setURL", Qt::AutoConnection, Q_ARG(QString, url));
        return;
    }
    _webView->setUrl(url);
}

QSizeF WebWindowClass::getSize() const {
    QSizeF size = _windowWidget->size();
    return size;
}

void WebWindowClass::setSize(QSizeF size) {
    setSize(size.width(), size.height());
}

void WebWindowClass::setSize(int width, int height) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setSize", Qt::AutoConnection, Q_ARG(int, width), Q_ARG(int, height));
        return;
    }
    _windowWidget->resize(width, height);
}

glm::vec2 WebWindowClass::getPosition() const {
    QPoint position = _windowWidget->pos();
    return glm::vec2(position.x(), position.y());
}

void WebWindowClass::setPosition(glm::vec2 position) {
    setPosition(position.x, position.y);
}

void WebWindowClass::setPosition(int x, int y) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setPosition", Qt::AutoConnection, Q_ARG(int, x), Q_ARG(int, y));
        return;
    }
    _windowWidget->move(x, y);
}

void WebWindowClass::raise() {
    QMetaObject::invokeMethod(_windowWidget, "showNormal", Qt::AutoConnection);
    QMetaObject::invokeMethod(_windowWidget, "raise", Qt::AutoConnection);
}

QScriptValue WebWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    WebWindowClass* retVal;
    QString file = context->argument(0).toString();
    if (context->argument(4).toBool()) {
        qWarning() << "ToolWindow views with WebWindow are no longer supported.  Use OverlayWebWindow instead";
        return QScriptValue();
    } else {
        qWarning() << "WebWindow views are deprecated.  Use OverlayWebWindow instead";
    }
    QMetaObject::invokeMethod(DependencyManager::get<WindowScriptingInterface>().data(), "doCreateWebWindow", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(WebWindowClass*, retVal),
            Q_ARG(const QString&, file),
            Q_ARG(QString, context->argument(1).toString()),
            Q_ARG(int, context->argument(2).toInteger()),
            Q_ARG(int, context->argument(3).toInteger()));

    connect(engine, &QScriptEngine::destroyed, retVal, &WebWindowClass::deleteLater);

    return engine->newQObject(retVal);
}

void WebWindowClass::setTitle(const QString& title) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setTitle", Qt::AutoConnection, Q_ARG(QString, title));
        return;
    }
    _windowWidget->setWindowTitle(title);
}
