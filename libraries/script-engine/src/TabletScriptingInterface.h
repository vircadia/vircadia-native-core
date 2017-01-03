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

#include <QObject>
#include <QVariant>
#include <QScriptValue>
#include <QQuickItem>
#include <QUuid>

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

    void setQmlTabletRoot(QString tabletId, QQuickItem* qmlTabletRoot);

protected:
    std::mutex _mutex;
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

    void setQmlTabletRoot(QQuickItem* qmlTabletRoot);

    /**jsdoc
     * @function TabletProxy#gotoHomeScreen
     * transition to the home screen
     */
    Q_INVOKABLE void gotoHomeScreen();

    /**jsdoc
     * @function TabletProxy#gotoWebScreen
     * show the specified web url on the tablet.
     * @param url {string}
     */
    Q_INVOKABLE void gotoWebScreen(const QString& url);

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

    /**jsdoc
     * @function TabletProxy#updateAudioBar
     * Updates the audio bar in tablet to reflect latest mic level
     * @param micLevel {double} mic level value between 0 and 1
     */
    Q_INVOKABLE void updateAudioBar(const double micLevel);
    
    QString getName() const { return _name; }
protected:

    void addButtonsToHomeScreen();
    void removeButtonsFromHomeScreen();
    QQuickItem* getQmlTablet() const;

    QString _name;
    std::mutex _mutex;
    std::vector<QSharedPointer<TabletButtonProxy>> _tabletButtonProxies;
    QQuickItem* _qmlTabletRoot { nullptr };
};

/**jsdoc
 * @class TabletButtonProxy
 * @property imageUrl {string}
 */
class TabletButtonProxy : public QObject {
    Q_OBJECT
public:
    TabletButtonProxy(const QVariantMap& properties);

    void setQmlButton(QQuickItem* qmlButton);

    /**jsdoc
     * Returns the current value of this button's properties
     * @function TabletButtonProxy#getProperties
     * @returns {object}
     */
    Q_INVOKABLE QVariantMap getProperties() const;

    /**jsdoc
     * Replace the values of some of this button's properties
     * @function TabletButtonProxy#editProperties
     * @param properties {object} set of properties to change
     */
    Q_INVOKABLE void editProperties(QVariantMap properties);

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
    QUuid _uuid;
    mutable std::mutex _mutex;
    QQuickItem* _qmlButton { nullptr };
    QVariantMap _properties;
};

#endif // hifi_TabletScriptingInterface_h
