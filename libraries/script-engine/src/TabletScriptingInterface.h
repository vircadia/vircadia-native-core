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

    void setQmlTabletRoot(QString tabletId, QQuickItem* qmlTabletRoot, QObject* qmlOffscreenSurface);

protected:
    std::mutex _mutex;
    std::map<QString, QSharedPointer<TabletProxy>> _tabletProxies;
};

/**jsdoc
 * @class TabletProxy
 * @property name {string} READ_ONLY: name of this tablet
 */
class TabletProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)
public:
    TabletProxy(QString name);

    void setQmlTabletRoot(QQuickItem* qmlTabletRoot, QObject* qmlOffscreenSurface);

    /**jsdoc
     * transition to the home screen
     * @function TabletProxy#gotoHomeScreen
     */
    Q_INVOKABLE void gotoHomeScreen();

    /**jsdoc
     * show the specified web url on the tablet.
     * @function TabletProxy#gotoWebScreen
     * @param url {string}
     */
    Q_INVOKABLE void gotoWebScreen(const QString& url);

    /**jsdoc
     * Creates a new button, adds it to this and returns it.
     * @function TabletProxy#addButton
     * @param properties {Object} button properties UI_TABLET_HACK: enumerate these when we figure out what they should be!
     * @returns {TabletButtonProxy}
     */
    Q_INVOKABLE QObject* addButton(const QVariant& properties);

    /**jsdoc
     * removes button from the tablet
     * @function TabletProxy.removeButton
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

    /**jsdoc
     * Used to send an event to the html/js embedded in the tablet
     * @function TabletProxy#emitScriptEvent
     * @param msg {object|string}
     */
    Q_INVOKABLE void emitScriptEvent(QVariant msg);

signals:
    /**jsdoc
     * Signaled when this tablet receives an event from the html/js embedded in the tablet
     * @function TabletProxy#webEventReceived
     * @param msg {object|string}
     * @returns {Signal}
     */
    void webEventReceived(QVariant msg);

private slots:
    void addButtonsToHomeScreen();
    
protected:
    
    void removeButtonsFromHomeScreen();
    QQuickItem* getQmlTablet() const;

    QString _name;
    std::mutex _mutex;
    std::vector<QSharedPointer<TabletButtonProxy>> _tabletButtonProxies;
    QQuickItem* _qmlTabletRoot { nullptr };
    QObject* _qmlOffscreenSurface { nullptr };
};

/**jsdoc
 * @class TabletButtonProxy
 * @property uuid {QUuid} READ_ONLY: uniquely identifies this button
 */
class TabletButtonProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUuid uuid READ getUuid)
public:
    TabletButtonProxy(const QVariantMap& properties);

    void setQmlButton(QQuickItem* qmlButton);

    QUuid getUuid() const { return _uuid; }

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
