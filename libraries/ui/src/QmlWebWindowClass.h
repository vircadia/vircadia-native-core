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
 * @augments OverlayWindow
 * @param {OverlayWindow.Properties} [properties=null]
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {string} url - <em>Read-only.</em>
 */

// FIXME refactor this class to be a QQuickItem derived type and eliminate the needless wrapping 
class QmlWebWindowClass : public QmlWindowClass {
    Q_OBJECT
    Q_PROPERTY(QString url READ getURL CONSTANT)

public:
    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);

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
