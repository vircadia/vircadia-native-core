//
//  WebWindow.h
//  interface/src/scripting
//
//  Created by Ryan Huffman on 11/06/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WebWindow_h
#define hifi_WebWindow_h

#include <QScriptContext>
#include <QScriptEngine>

class ScriptEventBridge : public QObject {
    Q_OBJECT
public:
    ScriptEventBridge(QObject* parent = NULL);

public slots:
    void emitWebEvent(const QString& data);
    void emitScriptEvent(const QString& data);

signals:
    void webEventReceived(const QString& data);
    void scriptEventReceived(const QString& data);

};

class WebWindow : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* eventBridge READ getEventBridge)
public:
    WebWindow(const QString& url, int width, int height);
    ~WebWindow();

    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);

public slots:
    void setVisible(bool visible);
    ScriptEventBridge* getEventBridge() const { return _eventBridge; }

private:
    QWidget* _window;
    ScriptEventBridge* _eventBridge;
};

#endif
