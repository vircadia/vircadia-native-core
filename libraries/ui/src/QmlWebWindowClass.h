//
//  Created by Bradley Austin Davis on 2015-12-15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ui_QmlWebWindowClass_h
#define hifi_ui_QmlWebWindowClass_h

#include "QmlWindowClass.h"

/**jsdoc
 * @class OverlayWebWindow
 * @param {OverlayWindow.Properties} [properties=null]
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {string} url - <em>Read-only.</em>
 * @property {Vec2} position
 * @property {Vec2} size
 * @property {boolean} visible
 *
 * @borrows OverlayWindow.initQml as initQml
 * @borrows OverlayWindow.isVisible as isVisible
 * @borrows OverlayWindow.setVisible as setVisible
 * @borrows OverlayWindow.getPosition as getPosition
 * @borrows OverlayWindow.setPosition as setPosition
 * @borrows OverlayWindow.getSize as getSize
 * @borrows OverlayWindow.setSize as setSize
 * @borrows OverlayWindow.setTitle as setTitle
 * @borrows OverlayWindow.raise as raise
 * @borrows OverlayWindow.close as close
 * @borrows OverlayWindow.getEventBridge as getEventBridge
 * @borrows OverlayWindow.sendToQml as sendToQml
 * @borrows OverlayWindow.clearDebugWindow as clearDebugWindow
 * @borrows OverlayWindow.emitScriptEvent as emitScriptEvent
 * @borrows OverlayWindow.emitWebEvent as emitWebEvent
 * @borrows OverlayWindow.visibleChanged as visibleChanged
 * @borrows OverlayWindow.positionChanged as positionChanged
 * @borrows OverlayWindow.sizeChanged as sizeChanged
 * @borrows OverlayWindow.moved as moved
 * @borrows OverlayWindow.resized as resized
 * @borrows OverlayWindow.closed as closed
 * @borrows OverlayWindow.fromQml as fromQml
 * @borrows OverlayWindow.scriptEventReceived as scriptEventReceived
 * @borrows OverlayWindow.webEventReceived as webEventReceived
 * @borrows OverlayWindow.hasMoved as hasMoved
 * @borrows OverlayWindow.hasClosed as hasClosed
 * @borrows OverlayWindow.qmlToScript as qmlToScript
 */

// FIXME refactor this class to be a QQuickItem derived type and eliminate the needless wrapping 
class QmlWebWindowClass : public QmlWindowClass {
    Q_OBJECT
    Q_PROPERTY(QString url READ getURL CONSTANT)

private:
    static QScriptValue internal_constructor(QScriptContext* context, QScriptEngine* engine, bool restricted);
public:
    QmlWebWindowClass(bool restricted) : QmlWindowClass(restricted) {}

    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine) {
        return internal_constructor(context, engine, false);
    }

    static QScriptValue restricted_constructor(QScriptContext* context, QScriptEngine* engine ){
        return internal_constructor(context, engine, true);
    }

public slots:

    /**jsdoc
     * @function OverlayWebWindow.getURL
     * @returns {string}
     */
    QString getURL();
    /**jsdoc
     * @function OverlayWebWindow.setURL
     * @param {string} url
     */
    void setURL(const QString& url);

    /**jsdoc
     * @function OverlayWebWindow.setScriptURL
     * @param {string} script
     */
    void setScriptURL(const QString& script);

signals:
    /**jsdoc
     * @function OverlayWebWindow.urlChanged
     * @returns {Signal}
     */
    void urlChanged();

protected:
    QString qmlSource() const override { return "QmlWebWindow.qml"; }
};

#endif
