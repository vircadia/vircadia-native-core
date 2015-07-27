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
#include <QWebFrame>
#include <QWebView>
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

WebWindowClass::WebWindowClass(const QString& title, const QString& url, int width, int height, bool isToolWindow)
    : QObject(NULL),
      _eventBridge(new ScriptEventBridge(this)),
      _isToolWindow(isToolWindow) {

    if (_isToolWindow) {
        ToolWindow* toolWindow = Application::getInstance()->getToolWindow();

        auto dockWidget = new QDockWidget(title, toolWindow);
        dockWidget->setFeatures(QDockWidget::DockWidgetMovable);

        _webView = new QWebView(dockWidget);
        addEventBridgeToWindowObject();

        dockWidget->setWidget(_webView);

        toolWindow->addDockWidget(Qt::RightDockWidgetArea, dockWidget);

        _windowWidget = dockWidget;
    } else {
        auto dialogWidget = new QDialog(Application::getInstance()->getWindow(), Qt::Window);
        dialogWidget->setWindowTitle(title);
        dialogWidget->resize(width, height);
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
}

WebWindowClass::~WebWindowClass() {
}

void WebWindowClass::hasClosed() {
    emit closed();
}

void WebWindowClass::addEventBridgeToWindowObject() {
    _webView->page()->mainFrame()->addToJavaScriptWindowObject("EventBridge", _eventBridge);
}

void WebWindowClass::setVisible(bool visible) {
    if (visible) {
        if (_isToolWindow) {
            QMetaObject::invokeMethod(
                Application::getInstance()->getToolWindow(), "setVisible", Qt::AutoConnection, Q_ARG(bool, visible));
        } else {
            QMetaObject::invokeMethod(_windowWidget, "showNormal", Qt::AutoConnection);
            QMetaObject::invokeMethod(_windowWidget, "raise", Qt::AutoConnection);
        }
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

void WebWindowClass::raise() {
    QMetaObject::invokeMethod(_windowWidget, "showNormal", Qt::AutoConnection);
    QMetaObject::invokeMethod(_windowWidget, "raise", Qt::AutoConnection);
}

QScriptValue WebWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    WebWindowClass* retVal;
    QString file = context->argument(0).toString();
    QMetaObject::invokeMethod(DependencyManager::get<WindowScriptingInterface>().data(), "doCreateWebWindow", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(WebWindowClass*, retVal),
            Q_ARG(const QString&, file),
            Q_ARG(QString, context->argument(1).toString()),
            Q_ARG(int, context->argument(2).toInteger()),
            Q_ARG(int, context->argument(3).toInteger()),
            Q_ARG(bool, context->argument(4).toBool()));

    connect(engine, &QScriptEngine::destroyed, retVal, &WebWindowClass::deleteLater);

    return engine->newQObject(retVal);
}

void WebWindowClass::setTitle(const QString& title) {
    _windowWidget->setWindowTitle(title);
}
