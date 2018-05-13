//
//  Created by Bradley Austin Davis on 2016-06-16
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ToolbarScriptingInterface_h
#define hifi_ToolbarScriptingInterface_h

#include <mutex>

#include <QtCore/QObject>
#include <QtScript/QScriptValue>

#include <DependencyManager.h>
#include "QmlWrapper.h"

class QQuickItem;

/**jsdoc
 * @class ToolbarButtonProxy
 *
 * @hifi-interface
 * @hifi-client-entity
 */
class ToolbarButtonProxy : public QmlWrapper {
    Q_OBJECT

public:
    ToolbarButtonProxy(QObject* qmlObject, QObject* parent = nullptr);

    /**jsdoc
     * @function ToolbarButtonProxy#editProperties
     * @param {object} properties
     */
    Q_INVOKABLE void editProperties(const QVariantMap& properties);

signals:

    /**jsdoc
     * @function ToolbarButtonProxy#clicked
     * @returns {Signal}
     */
    void clicked();

protected:
    QQuickItem* _qmlButton { nullptr };
    QVariantMap _properties;
};

Q_DECLARE_METATYPE(ToolbarButtonProxy*);

/**jsdoc
 * @class ToolbarProxy
 *
 * @hifi-interface
 * @hifi-client-entity
 */
class ToolbarProxy : public QmlWrapper {
    Q_OBJECT
public:
    ToolbarProxy(QObject* qmlObject, QObject* parent = nullptr);

    /**jsdoc
     * @function ToolbarProxy#addButton
     * @property {object} properties
     * @returns {ToolbarButtonProxy}
     */
    Q_INVOKABLE ToolbarButtonProxy* addButton(const QVariant& properties);

    /**jsdoc
     * @function ToolbarProxy#removeButton
     * @property {string} name
     */
    Q_INVOKABLE void removeButton(const QVariant& name);
};

Q_DECLARE_METATYPE(ToolbarProxy*);

/**jsdoc
 * @namespace Toolbars
 *
 * @hifi-interface
 * @hifi-client-entity
 */
class ToolbarScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:

    /**jsdoc
     * @function Toolbars.getToolbar
     * @param {string} toolbarID
     * @returns {ToolbarProxy}
     */
    Q_INVOKABLE ToolbarProxy* getToolbar(const QString& toolbarId);
};


#endif // hifi_ToolbarScriptingInterface_h
