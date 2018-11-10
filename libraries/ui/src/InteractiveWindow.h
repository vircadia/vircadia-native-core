//
//  InteractiveWindow.h
//  libraries/ui/src
//
//  Created by Thijs Wenker on 2018-06-25
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_InteractiveWindow_h
#define hifi_InteractiveWindow_h

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtScript/QScriptValue>
#include <QQmlEngine>

#include <glm/glm.hpp>
#include <GLMHelpers.h>

namespace InteractiveWindowEnums {
    Q_NAMESPACE

    enum InteractiveWindowFlags : uint8_t {
        AlwaysOnTop = 1 << 0,
        CloseButtonHides = 1 << 1
    };
    Q_ENUM_NS(InteractiveWindowFlags);

    enum InteractiveWindowPresentationMode {
        Virtual,
        Native
    };
    Q_ENUM_NS(InteractiveWindowPresentationMode);
}

using namespace InteractiveWindowEnums;

/**jsdoc
 * @class InteractiveWindow
 *
 * @hifi-interface
 * @hifi-client-en
 *
 * @property {string} title
 * @property {Vec2} position
 * @property {Vec2} size
 * @property {boolean} visible
 * @property {Desktop.PresentationMode} presentationMode
 * 
 */
class InteractiveWindow : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(glm::vec2 position READ getPosition WRITE setPosition)
    Q_PROPERTY(glm::vec2 size READ getSize WRITE setSize)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible)
    Q_PROPERTY(int presentationMode READ getPresentationMode WRITE setPresentationMode)

public:
    InteractiveWindow(const QString& sourceUrl, const QVariantMap& properties);

    ~InteractiveWindow();

private:
    // define property getters and setters as private to not expose them to the JS API
    Q_INVOKABLE QString getTitle() const;
    Q_INVOKABLE void setTitle(const QString& title);

    Q_INVOKABLE glm::vec2 getPosition() const;
    Q_INVOKABLE void setPosition(const glm::vec2& position);

    Q_INVOKABLE glm::vec2 getSize() const;
    Q_INVOKABLE void setSize(const glm::vec2& size);

    Q_INVOKABLE void setVisible(bool visible);
    Q_INVOKABLE bool isVisible() const;

    Q_INVOKABLE void setPresentationMode(int presentationMode);
    Q_INVOKABLE int getPresentationMode() const;

    Q_INVOKABLE void parentNativeWindowToMainWindow();

public slots:

    /**jsdoc
     * @function InteractiveWindow.sendToQml
     * @param {object} message
     */
    // Scripts can use this to send a message to the QML object
    void sendToQml(const QVariant& message);

    /**jsdoc
     * @function InteractiveWindow.emitScriptEvent
     * @param {object} message
     */
    // QmlWindow content may include WebView requiring EventBridge.
    void emitScriptEvent(const QVariant& scriptMessage);

    /**jsdoc
     * @function InteractiveWindow.emitWebEvent
     * @param {object} message
     */
    void emitWebEvent(const QVariant& webMessage);

    /**jsdoc
     * @function InteractiveWindow.close
     */
    Q_INVOKABLE void close();

    /**jsdoc
     * @function InteractiveWindow.show
     */
    Q_INVOKABLE void show();

    /**jsdoc
     * @function InteractiveWindow.raise
     */
    Q_INVOKABLE void raise();

signals:

    /**jsdoc
     * @function InteractiveWindow.visibleChanged
     * @returns {Signal}
     */
    void visibleChanged();

    /**jsdoc
     * @function InteractiveWindow.positionChanged
     * @returns {Signal}
     */
    void positionChanged();

    /**jsdoc
     * @function InteractiveWindow.sizeChanged
     * @returns {Signal}
     */
    void sizeChanged();

    /**jsdoc
     * @function InteractiveWindow.presentationModeChanged
     * @returns {Signal}
     */
    void presentationModeChanged();

    /**jsdoc
     * @function InteractiveWindow.titleChanged
     * @returns {Signal}
     */
    void titleChanged();

    /**jsdoc
     * @function InteractiveWindow.closed
     * @returns {Signal}
     */
    void closed();

    /**jsdoc
     * @function InteractiveWindow.fromQml
     * @param {object} message
     * @returns {Signal}
     */
    // Scripts can connect to this signal to receive messages from the QML object
    void fromQml(const QVariant& message);

    /**jsdoc
     * @function InteractiveWindow.scriptEventReceived
     * @param {object} message
     * @returns {Signal}
     */
    // InteractiveWindow content may include WebView requiring EventBridge.
    void scriptEventReceived(const QVariant& message);

    /**jsdoc
     * @function InteractiveWindow.webEventReceived
     * @param {object} message
     * @returns {Signal}
     */
    void webEventReceived(const QVariant& message);

protected slots:
    /**jsdoc
     * @function InteractiveWindow.qmlToScript
     * @param {object} message
     * @returns {Signal}
     */
    void qmlToScript(const QVariant& message);

private:
    QPointer<QObject> _qmlWindow;
};

typedef InteractiveWindow* InteractiveWindowPointer;

QScriptValue interactiveWindowPointerToScriptValue(QScriptEngine* engine, const InteractiveWindowPointer& in);
void interactiveWindowPointerFromScriptValue(const QScriptValue& object, InteractiveWindowPointer& out);

void registerInteractiveWindowMetaType(QScriptEngine* engine);

Q_DECLARE_METATYPE(InteractiveWindowPointer)

#endif // hifi_InteractiveWindow_h
