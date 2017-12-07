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
#include <atomic>

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QVariant>
#include <QtCore/QAbstractListModel>

#include <QtScript/QScriptValue>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValueIterator>

#include <QtQuick/QQuickItem>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "SoundCache.h"
#include <DependencyManager.h>

class ToolbarProxy;
class ToolbarScriptingInterface;

class TabletProxy;
class TabletButtonProxy;
class QmlWindowClass;
class OffscreenQmlSurface;

/**jsdoc
 * @namespace Tablet
 */
class TabletScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    enum TabletAudioEvents { ButtonClick, ButtonHover, TabletOpen, TabletHandsIn, TabletHandsOut, Last};
    Q_ENUM(TabletAudioEvents)

    TabletScriptingInterface();
    virtual ~TabletScriptingInterface();
    static const QString QML;

    void setToolbarScriptingInterface(ToolbarScriptingInterface* toolbarScriptingInterface) { _toolbarScriptingInterface = toolbarScriptingInterface; }

    /**jsdoc
     * Creates or retruns a new TabletProxy and returns it.
     * @function Tablet.getTablet
     * @param name {String} tablet name
     * @return {TabletProxy} tablet instance
     */
    Q_INVOKABLE TabletProxy* getTablet(const QString& tabletId);

    void preloadSounds();
    Q_INVOKABLE void playSound(TabletAudioEvents aEvent);

    void setToolbarMode(bool toolbarMode);

    void setQmlTabletRoot(QString tabletId, OffscreenQmlSurface* offscreenQmlSurface);

    void processEvent(const QKeyEvent* event);

    QQuickWindow* getTabletWindow();

    QObject* getFlags();
signals:
    /** jsdoc
     * Signaled when a tablet message or dialog is created
     * @function TabletProxy#tabletNotification
     * @returns {Signal}
     */
    void tabletNotification();

private:
    friend class TabletProxy;
    void processMenuEvents(QObject* object, const QKeyEvent* event);
    void processTabletEvents(QObject* object, const QKeyEvent* event);
    ToolbarProxy* getSystemToolbarProxy();

    QMap<TabletAudioEvents, SharedSoundPointer> _audioEvents;
protected:
    std::map<QString, TabletProxy*> _tabletProxies;
    ToolbarScriptingInterface* _toolbarScriptingInterface { nullptr };
    bool _toolbarMode { false };
};

class TabletButtonListModel : public QAbstractListModel {
    Q_OBJECT

public:
    TabletButtonListModel();
    ~TabletButtonListModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override { Q_UNUSED(parent); return (int)_buttons.size(); }
    QHash<int, QByteArray> roleNames() const override { return _roles; }
    Qt::ItemFlags flags(const QModelIndex& index) const override { return _flags; }
    QVariant data(const QModelIndex& index, int role) const override;


protected:
    friend class TabletProxy;
    TabletButtonProxy* addButton(const QVariant& properties);
    void removeButton(TabletButtonProxy* button);
    using List = std::list<QSharedPointer<TabletButtonProxy>>;
    static QHash<int, QByteArray> _roles;
    static Qt::ItemFlags _flags;
    std::vector<QSharedPointer<TabletButtonProxy>> _buttons;
};

Q_DECLARE_METATYPE(TabletButtonListModel*);

/**jsdoc
 * @class TabletProxy
 * @property name {string} READ_ONLY: name of this tablet
 * @property toolbarMode {bool} - used to transition this tablet into and out of toolbar mode.
 *     When tablet is in toolbar mode, all its buttons will appear in a floating toolbar.
 */
class TabletProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)
    Q_PROPERTY(bool toolbarMode READ getToolbarMode WRITE setToolbarMode NOTIFY toolbarModeChanged)
    Q_PROPERTY(bool landscape READ getLandscape WRITE setLandscape)
    Q_PROPERTY(bool tabletShown MEMBER _tabletShown NOTIFY tabletShownChanged)
    Q_PROPERTY(TabletButtonListModel* buttons READ getButtons CONSTANT)
public:
    TabletProxy(QObject* parent, const QString& name);
    ~TabletProxy();

    void setQmlTabletRoot(OffscreenQmlSurface* offscreenQmlSurface);

    const QString getName() const { return _name; }
    bool getToolbarMode() const { return _toolbarMode; }
    void setToolbarMode(bool toolbarMode);


    Q_INVOKABLE void gotoMenuScreen(const QString& submenu = "");
    Q_INVOKABLE void initialScreen(const QVariant& url);


    /**jsdoc
     * transition to the home screen
     * @function TabletProxy#gotoHomeScreen
     */
    Q_INVOKABLE void gotoHomeScreen();

    /**jsdoc
     * show the specified web url on the tablet.
     * @function TabletProxy#gotoWebScreen
     * @param url {string} url of web page.
     * @param [injectedJavaScriptUrl] {string} optional url to an additional JS script to inject into the web page.
     */
    Q_INVOKABLE void gotoWebScreen(const QString& url);
    Q_INVOKABLE void gotoWebScreen(const QString& url, const QString& injectedJavaScriptUrl, bool loadOtherBase = false);

    Q_INVOKABLE void loadQMLSource(const QVariant& path, bool resizable = false);
    // FIXME: This currently relies on a script initializing the tablet (hence the bool denoting success);
    //        it should be initialized internally so it cannot fail
    Q_INVOKABLE bool pushOntoStack(const QVariant& path);
    Q_INVOKABLE void popFromStack();

    Q_INVOKABLE void loadQMLOnTop(const QVariant& path);
    Q_INVOKABLE void loadWebScreenOnTop(const QVariant& url);
    Q_INVOKABLE void loadWebScreenOnTop(const QVariant& url, const QString& injectedJavaScriptUrl);
    Q_INVOKABLE void returnToPreviousApp();

    

    /** jsdoc
     * Check if the tablet has a message dialog open
     * @function TabletProxy#isMessageDialogOpen
     */
    Q_INVOKABLE bool isMessageDialogOpen();

    /**jsdoc
     * Creates a new button, adds it to this and returns it.
     * @function TabletProxy#addButton
     * @param properties {Object} button properties UI_TABLET_HACK: enumerate these when we figure out what they should be!
     * @returns {TabletButtonProxy}
     */
    Q_INVOKABLE TabletButtonProxy* addButton(const QVariant& properties);

    /**jsdoc
     * removes button from the tablet
     * @function TabletProxy.removeButton
     * @param tabletButtonProxy {TabletButtonProxy} button to be removed
     */
    Q_INVOKABLE void removeButton(TabletButtonProxy* tabletButtonProxy);

    /**jsdoc
     * Used to send an event to the html/js embedded in the tablet
     * @function TabletProxy#emitScriptEvent
     * @param msg {object|string}
     */
    Q_INVOKABLE void emitScriptEvent(const QVariant& msg);

    /**jsdoc
     * Used to send an event to the qml embedded in the tablet
     * @function TabletProxy#sendToQml
     * @param msg {object|string}
     */
    Q_INVOKABLE void sendToQml(const QVariant& msg);

    /**jsdoc
     * Check if the tablet is on the homescreen
     * @function TabletProxy#onHomeScreen()
     */
    Q_INVOKABLE bool onHomeScreen();

    /**jsdoc
     * set tablet into our out of landscape mode
     * @function TabletProxy#setLandscape
     * @param landscape {bool} true for landscape, false for portrait
     */
    Q_INVOKABLE void setLandscape(bool landscape) { _landscape = landscape; }
    Q_INVOKABLE bool getLandscape() { return _landscape; }

    Q_INVOKABLE bool isPathLoaded(const QVariant& path);

    QQuickItem* getTabletRoot() const { return _qmlTabletRoot; }

    OffscreenQmlSurface* getTabletSurface();

    QQuickItem* getQmlTablet() const;

    QQuickItem* getQmlMenu() const;

    TabletButtonListModel* getButtons() { return &_buttons; }
signals:
    /**jsdoc
     * Signaled when this tablet receives an event from the html/js embedded in the tablet
     * @function TabletProxy#webEventReceived
     * @param msg {object|string}
     * @returns {Signal}
     */
    void webEventReceived(QVariant msg);

    /**jsdoc
     * Signaled when this tablet receives an event from the qml embedded in the tablet
     * @function TabletProxy#fromQml
     * @param msg {object|string}
     * @returns {Signal}
     */
    void fromQml(QVariant msg);

    /**jsdoc
     * Signaled when this tablet screen changes.
     * @function TabletProxy#screenChanged
     * @param type {string} - "Home", "Web", "Menu", "QML", "Closed"
     * @param url {string} - only valid for Web and QML.
     */
    void screenChanged(QVariant type, QVariant url);

    /** jsdoc
    * Signaled when the tablet becomes visible or becomes invisible
    * @function TabletProxy#isTabletShownChanged
    * @returns {Signal}
    */
    void tabletShownChanged();

    void toolbarModeChanged();

protected slots:
    void desktopWindowClosed();
    void emitWebEvent(const QVariant& msg);
    void onTabletShown();

protected:
    void loadHomeScreen(bool forceOntoHomeScreen);

    bool _initialScreen { false };
    QVariant _currentPathLoaded { "" };
    QString _name;
    QQuickItem* _qmlTabletRoot { nullptr };
    OffscreenQmlSurface* _qmlOffscreenSurface { nullptr };
    QmlWindowClass* _desktopWindow { nullptr };
    bool _toolbarMode { false };
    bool _tabletShown { false };

    enum class State { Uninitialized, Home, Web, Menu, QML };
    State _state { State::Uninitialized };
    std::pair<QVariant, State> _initialPath { "", State::Uninitialized };
    std::pair<QVariant, bool> _initialWebPathParams;
    bool _landscape { false };
    bool _showRunningScripts { false };

    TabletButtonListModel _buttons;
};

Q_DECLARE_METATYPE(TabletProxy*);

/**jsdoc
 * @class TabletButtonProxy
 * @property uuid {QUuid} READ_ONLY: uniquely identifies this button
 */
class TabletButtonProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUuid uuid READ getUuid)
    Q_PROPERTY(QVariantMap properties READ getProperties NOTIFY propertiesChanged)
public:
    TabletButtonProxy(const QVariantMap& properties);
    ~TabletButtonProxy();

    QUuid getUuid() const { return _uuid; }

    /**jsdoc
     * Returns the current value of this button's properties
     * @function TabletButtonProxy#getProperties
     * @returns {ButtonProperties}
     */
    Q_INVOKABLE QVariantMap getProperties();

    /**jsdoc
     * Replace the values of some of this button's properties
     * @function TabletButtonProxy#editProperties
     * @param {ButtonProperties} properties - set of properties to change
     */
    Q_INVOKABLE void editProperties(const QVariantMap& properties);

signals:
    /**jsdoc
     * Signaled when this button has been clicked on by the user.
     * @function TabletButtonProxy#clicked
     * @returns {Signal}
     */
    void clicked();
    void propertiesChanged();

protected:
    QUuid _uuid;
    int _stableOrder;
    QVariantMap _properties;
};

Q_DECLARE_METATYPE(TabletButtonProxy*);

/**jsdoc
 * @typedef TabletButtonProxy.ButtonProperties
 * @property {string} icon - url to button icon. (50 x 50)
 * @property {string} hoverIcon - url to button icon, displayed during mouse hover. (50 x 50)
 * @property {string} activeHoverIcon - url to button icon used when button is active, and during mouse hover. (50 x 50)
 * @property {string} activeIcon - url to button icon used when button is active. (50 x 50)
 * @property {string} text - button caption
 * @property {string} hoverText - button caption when button is not-active but during mouse hover.
 * @property {string} activeText - button caption when button is active
 * @property {string} activeHoverText - button caption when button is active and during mouse hover.
 * @property {string} isActive - true when button is active.
 * @property {number} sortOrder - determines sort order on tablet.  lower numbers will appear before larger numbers.  default is 100
 */

#endif // hifi_TabletScriptingInterface_h
