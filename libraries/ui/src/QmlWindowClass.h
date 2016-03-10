//
//  Created by Bradley Austin Davis on 2015-12-15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ui_QmlWindowClass_h
#define hifi_ui_QmlWindowClass_h

#include <QtCore/QObject>
#include <GLMHelpers.h>
#include <QtScript/QScriptValue>
#include <QtQuick/QQuickItem>
#include <QtWebChannel/QWebChannelAbstractTransport>

class QScriptEngine;
class QScriptContext;
class QmlWindowClass;
class QWebSocketServer;
class QWebSocket;

class QmlScriptEventBridge : public QObject {
    Q_OBJECT
public:
    QmlScriptEventBridge(const QmlWindowClass* webWindow) : _webWindow(webWindow) {}

public slots :
    void emitWebEvent(const QString& data);
    void emitScriptEvent(const QString& data);

signals:
    void webEventReceived(const QString& data);
    void scriptEventReceived(int windowId, const QString& data);

private:
    const QmlWindowClass* _webWindow { nullptr };
    QWebSocket *_socket { nullptr };
};

// FIXME refactor this class to be a QQuickItem derived type and eliminate the needless wrapping 
class QmlWindowClass : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* eventBridge READ getEventBridge CONSTANT)
    Q_PROPERTY(int windowId READ getWindowId CONSTANT)
    Q_PROPERTY(glm::vec2 position READ getPosition WRITE setPosition)
    Q_PROPERTY(glm::vec2 size READ getSize WRITE setSize)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibilityChanged)

public:
    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);
    QmlWindowClass(QObject* qmlWindow);
    ~QmlWindowClass();

public slots:
    bool isVisible() const;
    void setVisible(bool visible);

    glm::vec2 getPosition() const;
    void setPosition(const glm::vec2& position);
    void setPosition(int x, int y);

    glm::vec2 getSize() const;
    void setSize(const glm::vec2& size);
    void setSize(int width, int height);

    void setTitle(const QString& title);


    // Ugh.... do not want to do
    Q_INVOKABLE void raise();
    Q_INVOKABLE void close();
    Q_INVOKABLE int getWindowId() const { return _windowId; };
    Q_INVOKABLE QmlScriptEventBridge* getEventBridge() const { return _eventBridge; };

    // Scripts can use this to send a message to the QML object
    void sendToQml(const QVariant& message);

signals:
    void visibilityChanged(bool visible);  // Tool window
    void moved(glm::vec2 position);
    void resized(QSizeF size);
    void closed();
    // Scripts can connect to this signal to receive messages from the QML object
    void fromQml(const QVariant& message);

protected slots:
    void hasClosed();

protected:
    static QScriptValue internalConstructor(const QString& qmlSource, 
        QScriptContext* context, QScriptEngine* engine, 
        std::function<QmlWindowClass*(QObject*)> function);
    static void setupServer();
    static void registerObject(const QString& name, QObject* object);
    static void deregisterObject(QObject* object);
    static QWebSocketServer* _webChannelServer;

    QQuickItem* asQuickItem() const;
    QmlScriptEventBridge* const _eventBridge { new QmlScriptEventBridge(this) };

    // FIXME needs to be initialized in the ctor once we have support
    // for tool window panes in QML
    bool _toolWindow { false };
    const int _windowId;
    QObject* _qmlWindow;
    QString _source;
};

#endif
