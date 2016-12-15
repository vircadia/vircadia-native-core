//
//  Created by Anthony J. Thibault on 2016-12-12
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TabletScriptingInterface_h
#define hifi_TabletScriptingInterface_h

#include <mutex>

#include <QtCore/QObject>
#include <QVariant>
#include <QScriptValue>
#include <QQuickItem>

#include <DependencyManager.h>


class TabletProxy;
class TabletButtonProxy;

/**jsdoc
 * @namespace Tablet
 */
class TabletScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    /**jsdoc
     * Creates or retruns a new TabletProxy and returns it.
     * @function Tablet.getTablet
     * @param name {String} tablet name
     * @return {TabletProxy} tablet instance
     */
    Q_INVOKABLE QObject* getTablet(const QString& tabletId);

    void setQmlTablet(QString tabletId, QQuickItem* qmlTablet);

protected:
    std::mutex _tabletProxiesMutex;
    std::map<QString, QSharedPointer<TabletProxy>> _tabletProxies;
};

/**jsdoc
 * @class TabletProxy
 * @property name {string} name of this tablet
 */
class TabletProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)
public:
    TabletProxy(QString name);

    void setQmlTablet(QQuickItem* qmlTablet);

    /**jsdoc
     * @function TabletProxy#addButton
     * Creates a new button, adds it to this and returns it.
     * @param properties {Object} button properties UI_TABLET_HACK: enumerate these when we figure out what they should be!
     * @returns {TabletButtonProxy}
     */
    Q_INVOKABLE QObject* addButton(const QVariant& properties);

    /**jsdoc
     * @function TabletProxy#removeButton
     * removes button from the tablet
     * @param tabletButtonProxy {TabletButtonProxy} button to be removed
     */
    Q_INVOKABLE void removeButton(QObject* tabletButtonProxy);

    QString getName() const { return _name; }
protected:
    QString _name;
    std::mutex _tabletButtonProxiesMutex;
    std::vector<QSharedPointer<TabletButtonProxy>> _tabletButtonProxies;
    QQuickItem* _qmlTablet { nullptr };
};

/**jsdoc
 * @class TabletButtonProxy
 * @property imageUrl {string}
 */
class TabletButtonProxy : public QObject {
    Q_OBJECT
public:
    TabletButtonProxy(const QVariantMap& properties);

    /**jsdoc
     * @function TabletButtonProxy#setInitRequestHandler
     * @param handler {Function} A function used by the system to request the current button state from JavaScript.
     */
    Q_INVOKABLE void setInitRequestHandler(const QScriptValue& handler);

    const QVariantMap& getProperties() const { return _properties; }

public slots:
    void clickedSlot() { emit clicked(); }

signals:
    /**jsdoc
     * Signaled when this button has been clicked on by the user.
     * @function TabletButtonProxy#clicked
     * @returns {Signal}
     */
    void clicked();

protected:
    mutable std::mutex _propertiesMutex;
    QVariantMap _properties;
    QScriptValue _initRequestHandler;
};

#endif // hifi_TabletScriptingInterface_h
