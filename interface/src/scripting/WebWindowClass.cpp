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

#include "Application.h"
#include "MainWindow.h"
#include "WindowScriptingInterface.h"
#include "WebWindowClass.h"

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
        _windowWidget = new QWidget(Application::getInstance()->getWindow(), Qt::Window);
        _windowWidget->setMinimumSize(width, height);

        _webView = new QWebView(_windowWidget);
        addEventBridgeToWindowObject();
    }

    _webView->setPage(new InterfaceWebPage());
    _webView->setUrl(url);


    connect(this, &WebWindowClass::destroyed, _windowWidget, &QWidget::deleteLater);
    connect(_webView->page()->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared,
            this, &WebWindowClass::addEventBridgeToWindowObject);
}

WebWindowClass::~WebWindowClass() {
}

void WebWindowClass::addEventBridgeToWindowObject() {
    _webView->page()->mainFrame()->addToJavaScriptWindowObject("EventBridge", _eventBridge);
}

void WebWindowClass::setVisible(bool visible) {
    if (visible) {
        QMetaObject::invokeMethod(
            Application::getInstance()->getToolWindow(), "setVisible", Qt::BlockingQueuedConnection, Q_ARG(bool, visible));
    }
    QMetaObject::invokeMethod(_windowWidget, "setVisible", Qt::BlockingQueuedConnection, Q_ARG(bool, visible));
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
