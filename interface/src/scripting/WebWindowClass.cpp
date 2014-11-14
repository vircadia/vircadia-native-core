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


WebWindowClass::WebWindowClass(const QString& title, const QString& url, int width, int height)
    : QObject(NULL),
      _eventBridge(new ScriptEventBridge(this)) {

    QMainWindow* toolWindow = Application::getInstance()->getToolWindow();

    _window = new QDockWidget(title, toolWindow);
    QWebView* webView = new QWebView(_window);
    webView->page()->mainFrame()->addToJavaScriptWindowObject("EventBridge", _eventBridge);
    webView->setUrl(url);
    _window->setWidget(webView);

    toolWindow->addDockWidget(Qt::RightDockWidgetArea, _window);

    connect(this, &WebWindowClass::destroyed, _window, &QWidget::deleteLater);
}

WebWindowClass::~WebWindowClass() {
}

void WebWindowClass::setVisible(bool visible) {
    QMetaObject::invokeMethod(_window, "setVisible", Qt::BlockingQueuedConnection, Q_ARG(bool, visible));
}

QScriptValue WebWindowClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    WebWindowClass* retVal;
    QString file = context->argument(0).toString();
    QMetaObject::invokeMethod(WindowScriptingInterface::getInstance(), "doCreateWebWindow", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(WebWindowClass*, retVal),
            Q_ARG(const QString&, file),
            Q_ARG(QString, context->argument(1).toString()),
            Q_ARG(int, context->argument(2).toInteger()),
            Q_ARG(int, context->argument(3).toInteger()));

    connect(engine, &QScriptEngine::destroyed, retVal, &WebWindowClass::deleteLater);

    return engine->newQObject(retVal);
}
