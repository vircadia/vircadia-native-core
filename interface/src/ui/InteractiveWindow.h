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

    /**jsdoc
     * A set of  flags controlling <code>InteractiveWindow</code> behavior. The value is constructed by using the 
     * <code>|</code> (bitwise OR) operator on the individual flag values.<br />
     * <table>
     *   <thead>
     *     <tr><th>Flag Name</th><th>Value</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td>ALWAYS_ON_TOP</td><td><code>1</code></td><td>The window always displays on top.</td></tr>
     *     <tr><td>CLOSE_BUTTON_HIDES</td><td><code>2</code></td><td>The window hides instead of closing when the user clicks 
     *       the "close" button.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} InteractiveWindow.Flags
     */
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

    enum DockArea {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT
    };
    Q_ENUM_NS(DockArea);
}

using namespace InteractiveWindowEnums;

/**jsdoc
 * An <code>InteractiveWindow</code> can display either inside Interface or in its own window separate from the Interface 
 * window. The window content is defined by a QML file, which can optionally include a <code>WebView</code> control that embeds 
 * an HTML Web page. (The <code>WebView</code> control is defined by a "WebView.qml" file included in the Interface install.)
 *
 * <p>Create using {@link Desktop.createWindow}.</p>
 *
 * @class InteractiveWindow
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {string} title - The title of the window.
 * @property {Vec2} position - The position of the window, in pixels.
 * @property {Vec2} size - The size of the window, in pixels.
 * @property {boolean} visible - <code>true</code> if the window is visible, <code>false</code> if it isn't.
 * @property {InteractiveWindow.PresentationMode} presentationMode - The presentation mode of the window: 
 *     <code>Desktop.PresentationMode.VIRTUAL</code> to display the window inside Interface, <code>.NATIVE</code> to display it
 *     as its own separate window.
 */

class DockWidget;
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
     * Sends a message to the QML page. To receive the message, the QML page must implement a function:
     * <pre class="prettyprint"><code>function fromScript(message) {
     *   ...
     * }</code></pre>
     * @function InteractiveWindow.sendToQml
     * @param {string|object} message - The message to send to the QML page.
     * @example <caption>Send and receive messages with a QML window.</caption>
     * // JavaScript file.
     * 
     * var qmlWindow = Desktop.createWindow(Script.resolvePath("QMLWindow.qml"), {
     *     title: "QML Window",
     *     size: { x: 400, y: 300 }
     * });
     * 
     * qmlWindow.fromQml.connect(function (message) {
     *     print("Message received: " + message);
     * });
     * 
     * Script.setTimeout(function () {
     *     qmlWindow.sendToQml("Hello world!");
     * }, 2000);
     * 
     * Script.scriptEnding.connect(function () {
     *     qmlWindow.close();
     * });
     * @example
     * // QML file, "QMLWindow.qml".
     * 
     * import QtQuick 2.5
     * import QtQuick.Controls 1.4
     * 
     * Rectangle {
     * 
     *     function fromScript(message) {
     *         text.text = message;
     *         sendToScript("Hello back!");
     *     }
     * 
     *     Label {
     *         id: text
     *         anchors.centerIn: parent
     *         text: "..."
     *     }
     * }
     */
    // Scripts can use this to send a message to the QML object
    void sendToQml(const QVariant& message);

    /**jsdoc
     * Sends a message to an embedded HTML Web page. To receive the message, the HTML page's script must connect to the 
     * <code>EventBridge</code> that is automatically provided to the script:
     * <pre class="prettyprint"><code>EventBridge.scriptEventReceived.connect(function(message) {
     *     ...
     * });</code></pre>
     * @function InteractiveWindow.emitScriptEvent
     * @param {string|object} message - The message to send to the embedded HTML Web page.
     */
    // QmlWindow content may include WebView requiring EventBridge.
    void emitScriptEvent(const QVariant& scriptMessage);

    /**jsdoc
     * @function InteractiveWindow.emitWebEvent
     * @param {object|string} message - The message.
     * @deprecated This function is deprecated and will be removed from the API.
     */
    void emitWebEvent(const QVariant& webMessage);

    /**jsdoc
     * Closes the window. It can then no longer be used.
     * @function InteractiveWindow.close
     */
    Q_INVOKABLE void close();

    /**jsdoc
     * Makes the window visible and raises it to the top.
     * @function InteractiveWindow.show
     */
    Q_INVOKABLE void show();

    /**jsdoc
     * Raises the window to the top.
     * @function InteractiveWindow.raise
     */
    Q_INVOKABLE void raise();

signals:

    /**jsdoc
     * Triggered when the window is made visible or invisible, or is closed.
     * @function InteractiveWindow.visibleChanged
     * @returns {Signal}
     */
    void visibleChanged();

    /**jsdoc
     * Triggered when the window's position changes.
     * @function InteractiveWindow.positionChanged
     * @returns {Signal}
     */
    void positionChanged();

    /**jsdoc
     * Triggered when the window's' size changes.
     * @function InteractiveWindow.sizeChanged
     * @returns {Signal}
     */
    void sizeChanged();

    /**jsdoc
     * Triggered when the window's presentation mode changes.
     * @function InteractiveWindow.presentationModeChanged
     * @returns {Signal}
     */
    void presentationModeChanged();

    /**jsdoc
     * Triggered when window's title changes.
     * @function InteractiveWindow.titleChanged
     * @returns {Signal}
     */
    void titleChanged();

    /**jsdoc
     * Triggered when the window is closed.
     * @function InteractiveWindow.closed
     * @returns {Signal}
     */
    void closed();

    /**jsdoc
     * Triggered when a message from the QML page is received. The QML page can send a message (string or object) by calling:
     * <pre class="prettyprint"><code>sendToScript(message);</code></pre>
     * @function InteractiveWindow.fromQml
     * @param {string|object} message - The message received.
     * @returns {Signal}
     */
    // Scripts can connect to this signal to receive messages from the QML object
    void fromQml(const QVariant& message);

    /**jsdoc
     * @function InteractiveWindow.scriptEventReceived
     * @param {object} message - The message.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed from the API.
     */
    // InteractiveWindow content may include WebView requiring EventBridge.
    void scriptEventReceived(const QVariant& message);

    /**jsdoc
     * Trigged when a message from an embedded HTML Web page is received. The HTML Web page can send a message by calling:
     * <pre class="prettyprint"><code>EventBridge.emitWebEvent(message);</code></pre>
     * @function InteractiveWindow.webEventReceived
     * @param {string|object} message - The message received.
     * @returns {Signal}
     */
    void webEventReceived(const QVariant& message);

protected slots:
    /**jsdoc
     * @function InteractiveWindow.qmlToScript
     * @param {object} message
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed from the API.
     */
    void qmlToScript(const QVariant& message);

private:
    bool _dockedWindow { false };
    QPointer<QObject> _qmlWindow;
    std::shared_ptr<DockWidget> _dockWidget { nullptr };
};

typedef InteractiveWindow* InteractiveWindowPointer;

QScriptValue interactiveWindowPointerToScriptValue(QScriptEngine* engine, const InteractiveWindowPointer& in);
void interactiveWindowPointerFromScriptValue(const QScriptValue& object, InteractiveWindowPointer& out);

void registerInteractiveWindowMetaType(QScriptEngine* engine);

Q_DECLARE_METATYPE(InteractiveWindowPointer)

#endif // hifi_InteractiveWindow_h
