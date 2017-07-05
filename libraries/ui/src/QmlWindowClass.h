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
#include <QtCore/QPointer>
#include <QtScript/QScriptValue>
#include <QtQuick/QQuickItem>

#include <GLMHelpers.h>

class QScriptEngine;
class QScriptContext;

#define QML_TOOL_WINDOW 0

// FIXME refactor this class to be a QQuickItem derived type and eliminate the needless wrapping 
class QmlWindowClass : public QObject {
    Q_OBJECT
    Q_PROPERTY(glm::vec2 position READ getPosition WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(glm::vec2 size READ getSize WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)

public:
    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);
    QmlWindowClass();
    ~QmlWindowClass();

    Q_INVOKABLE virtual void initQml(QVariantMap properties);
    QQuickItem* asQuickItem() const;

public slots:
    bool isVisible();
    void setVisible(bool visible);

    glm::vec2 getPosition();
    void setPosition(const glm::vec2& position);
    void setPosition(int x, int y);

    glm::vec2 getSize();
    void setSize(const glm::vec2& size);
    void setSize(int width, int height);
    void setTitle(const QString& title);

    Q_INVOKABLE void raise();
    Q_INVOKABLE void close();
    Q_INVOKABLE QObject* getEventBridge() { return this; };

    // Scripts can use this to send a message to the QML object
    void sendToQml(const QVariant& message);
    void clearDebugWindow();

    // QmlWindow content may include WebView requiring EventBridge.
    void emitScriptEvent(const QVariant& scriptMessage);
    void emitWebEvent(const QVariant& webMessage);

signals:
    void visibleChanged();
    void positionChanged();
    void sizeChanged();
    void moved(glm::vec2 position);
    void resized(QSizeF size);
    void closed();
    // Scripts can connect to this signal to receive messages from the QML object
    void fromQml(const QVariant& message);

    // QmlWindow content may include WebView requiring EventBridge.
    void scriptEventReceived(const QVariant& message);
    void webEventReceived(const QVariant& message);

protected slots:
    void hasMoved(QVector2D);
    void hasClosed();
    void qmlToScript(const QVariant& message);

protected:
    static QVariantMap parseArguments(QScriptContext* context);
    static QScriptValue internalConstructor(QScriptContext* context, QScriptEngine* engine, 
        std::function<QmlWindowClass*(QVariantMap)> function);

    virtual QString qmlSource() const { return "QmlWindow.qml"; }

#if QML_TOOL_WINDOW
    // FIXME needs to be initialized in the ctor once we have support
    // for tool window panes in QML
    bool _toolWindow { false };
#endif

    QPointer<QObject> _qmlWindow;
    QString _source;

private:
    // QmlWindow content may include WebView requiring EventBridge.
    void setKeyboardRaised(QObject* object, bool raised, bool numeric = false);
};

#endif
