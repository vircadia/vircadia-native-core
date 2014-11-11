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
#include <QWebFrame>
#include <QWebView>

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

WebWindowClass::WebWindowClass(const QString& url, int width, int height)
    : QObject(NULL),
      _window(new QWidget(NULL, Qt::Tool)),
      _eventBridge(new ScriptEventBridge(this)) {

    QWebView* webView = new QWebView(_window);
    webView->page()->mainFrame()->addToJavaScriptWindowObject("EventBridge", _eventBridge);
    webView->setUrl(url);
    QVBoxLayout* layout = new QVBoxLayout(_window);
    _window->setLayout(layout);
    layout->addWidget(webView);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    _window->setGeometry(0, 0, width, height);

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
            Q_ARG(int, context->argument(1).toInteger()),
            Q_ARG(int, context->argument(2).toInteger()));

    connect(engine, &QScriptEngine::destroyed, retVal, &WebWindowClass::deleteLater);

    return engine->newQObject(retVal);
}
