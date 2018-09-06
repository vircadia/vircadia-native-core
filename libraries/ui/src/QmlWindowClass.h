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

/**jsdoc
 * @class OverlayWindow
 * @param {OverlayWindow.Properties} [properties=null]
 *
 * @hifi-interface
 * @hifi-client-en
 *
 * @property {Vec2} position
 * @property {Vec2} size
 * @property {boolean} visible
 */

// FIXME refactor this class to be a QQuickItem derived type and eliminate the needless wrapping 
class QmlWindowClass : public QObject {
    Q_OBJECT
    Q_PROPERTY(glm::vec2 position READ getPosition WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(glm::vec2 size READ getSize WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)

private:
    static QScriptValue internal_constructor(QScriptContext* context, QScriptEngine* engine, bool restricted);
public:
    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine) {
        return internal_constructor(context, engine, false);
    }

    static QScriptValue restricted_constructor(QScriptContext* context, QScriptEngine* engine ){
        return internal_constructor(context, engine, true);
    }

    QmlWindowClass(bool restricted);
    ~QmlWindowClass();

    /**jsdoc
     * @function OverlayWindow.initQml
     * @param {OverlayWindow.Properties} properties
     */
    Q_INVOKABLE virtual void initQml(QVariantMap properties);

    QQuickItem* asQuickItem() const;



public slots:

    /**jsdoc
     * @function OverlayWindow.isVisible
     * @returns {boolean}
     */
    bool isVisible();

    /**jsdoc
     * @function OverlayWindow.setVisible
     * @param {boolean} visible
     */
    void setVisible(bool visible);


    /**jsdoc
     * @function OverlayWindow.getPosition
     * @returns {Vec2}
     */
    glm::vec2 getPosition();

    /**jsdoc
     * @function OverlayWindow.setPosition
     * @param {Vec2} position
     */
    void setPosition(const glm::vec2& position);

    /**jsdoc
     * @function OverlayWindow.setPosition
     * @param {number} x
     * @param {number} y
     */
    void setPosition(int x, int y);


    /**jsdoc
     * @function OverlayWindow.getSize
     * @returns {Vec2}
     */
    glm::vec2 getSize();

    /**jsdoc
     * @function OverlayWindow.setSize
     * @param {Vec2} size
     */
    void setSize(const glm::vec2& size);

    /**jsdoc
     * @function OverlayWindow.setSize
     * @param {number} width
     * @param {number} height
     */
    void setSize(int width, int height);

    /**jsdoc
     * @function OverlayWindow.setTitle
     * @param {string} title
     */
    void setTitle(const QString& title);


    /**jsdoc
     * @function OverlayWindow.raise
     */
    Q_INVOKABLE void raise();

    /**jsdoc
     * @function OverlayWindow.close
     */
    Q_INVOKABLE void close();

    /**jsdoc
     * @function OverlayWindow.getEventBridge
     * @returns {object}
     */
    Q_INVOKABLE QObject* getEventBridge() { return this; };


    /**jsdoc
     * @function OverlayWindow.sendToQml
     * @param {object} message
     */
    // Scripts can use this to send a message to the QML object
    void sendToQml(const QVariant& message);

    /**jsdoc
     * @function OverlayWindow.clearDebugWindow
     */
    void clearDebugWindow();


    /**jsdoc
     * @function OverlayWindow.emitScriptEvent
     * @param {object} message
     */
    // QmlWindow content may include WebView requiring EventBridge.
    void emitScriptEvent(const QVariant& scriptMessage);

    /**jsdoc
     * @function OverlayWindow.emitWebEvent
     * @param {object} message
     */
    void emitWebEvent(const QVariant& webMessage);

signals:

    /**jsdoc
     * @function OverlayWindow.visibleChanged
     * @returns {Signal}
     */
    void visibleChanged();

    /**jsdoc
     * @function OverlayWindow.positionChanged
     * @returns {Signal}
     */
    void positionChanged();

    /**jsdoc
     * @function OverlayWindow.sizeChanged
     * @returns {Signal}
     */
    void sizeChanged();

    /**jsdoc
     * @function OverlayWindow.moved
     * @param {Vec2} position
     * @returns {Signal}
     */
    void moved(glm::vec2 position);

    /**jsdoc
     * @function OverlayWindow.resized
     * @param {Size} size
     * @returns {Signal}
     */
    void resized(QSizeF size);

    /**jsdoc
     * @function OverlayWindow.closed
     * @returns {Signal}
     */
    void closed();

    /**jsdoc
     * @function OverlayWindow.fromQml
     * @param {object} message
     * @returns {Signal}
     */
    // Scripts can connect to this signal to receive messages from the QML object
    void fromQml(const QVariant& message);


    /**jsdoc
     * @function OverlayWindow.scriptEventReceived
     * @param {object} message
     * @returns {Signal}
     */
    // QmlWindow content may include WebView requiring EventBridge.
    void scriptEventReceived(const QVariant& message);

    /**jsdoc
     * @function OverlayWindow.webEventReceived
     * @param {object} message
     * @returns {Signal}
     */
    void webEventReceived(const QVariant& message);

protected slots:

    /**jsdoc
     * @function OverlayWindow.hasMoved
     * @param {Vec2} position
     * @returns {Signal}
     */
    void hasMoved(QVector2D);

    /**jsdoc
     * @function OverlayWindow.hasClosed
     * @returns {Signal}
     */
    void hasClosed();

    /**jsdoc
     * @function OverlayWindow.qmlToScript
     * @param {object} message
     * @returns {Signal}
     */
    void qmlToScript(const QVariant& message);

protected:
    static QVariantMap parseArguments(QScriptContext* context);
    static QScriptValue internalConstructor(QScriptContext* context, QScriptEngine* engine, 
        std::function<QmlWindowClass*(QVariantMap)> function);

    virtual QString qmlSource() const { return "QmlWindow.qml"; }

    QPointer<QObject> _qmlWindow;
    QString _source;
    const bool _restricted;

private:
    // QmlWindow content may include WebView requiring EventBridge.
    void setKeyboardRaised(QObject* object, bool raised, bool numeric = false);

};

#endif
