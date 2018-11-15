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
#include <QSortFilterProxyModel>

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
 *
 * @hifi-interface
 * @hifi-client-entity
 */
/**jsdoc
 * @namespace tabletInterface
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @deprecated This API is deprecated and will be removed. Use {@link Tablet} instead.
 */
class TabletScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:

    /**jsdoc
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>Button click.</td></tr>
     *     <tr><td><code>1</code></td><td>Button hover.</td></tr>
     *     <tr><td><code>2</code></td><td>Tablet open.</td></tr>
     *     <tr><td><code>3</code></td><td>Tablet hands in.</td></tr>
     *     <tr><td><code>4</code></td><td>Tablet hands out.</td></tr>
     *     <tr><td><code>5</code></td><td>Last.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} Tablet.AudioEvents
     */
    enum TabletAudioEvents { ButtonClick, ButtonHover, TabletOpen, TabletHandsIn, TabletHandsOut, Last};
    Q_ENUM(TabletAudioEvents)

    //Different useful constants
    enum TabletConstants { ButtonsColumnsOnPage = 3, ButtonsRowsOnPage = 4, ButtonsOnPage = 12 };
    Q_ENUM(TabletConstants)

    TabletScriptingInterface();
    virtual ~TabletScriptingInterface();
    static const QString QML;

    void setToolbarScriptingInterface(ToolbarScriptingInterface* toolbarScriptingInterface) { _toolbarScriptingInterface = toolbarScriptingInterface; }

    /**jsdoc
     * Creates or returns a new TabletProxy and returns it.
     * @function Tablet.getTablet
     * @param {string} name - Tablet name.
     * @returns {TabletProxy} Tablet instance.
     */
    /**jsdoc
     * Creates or returns a new TabletProxy and returns it.
     * @function tabletInterface.getTablet
     * @param {string} name - Tablet name.
     * @returns {TabletProxy} Tablet instance.
     * @deprecated This function is deprecated and will be removed. Use {@link Tablet.getTablet} instead.
     */
    Q_INVOKABLE TabletProxy* getTablet(const QString& tabletId);

    void preloadSounds();

    /**jsdoc
     * @function Tablet.playSound
     * @param {Tablet.AudioEvents} sound
     */
    /**jsdoc
     * @function tabletInterface.playSound
     * @param {Tablet.AudioEvents} sound
     * @deprecated This function is deprecated and will be removed. Use {@link Tablet.playSound} instead.
     */
    Q_INVOKABLE void playSound(TabletAudioEvents aEvent);

    void setToolbarMode(bool toolbarMode);

    void setQmlTabletRoot(QString tabletId, OffscreenQmlSurface* offscreenQmlSurface);

    void processEvent(const QKeyEvent* event);

    QQuickWindow* getTabletWindow();

    QObject* getFlags();
signals:
    /**jsdoc
     * Triggered when a tablet message or dialog is created.
     * @function Tablet.tabletNotification
     * @returns {Signal}
     */
    /**jsdoc
     * Triggered when a tablet message or dialog is created.
     * @function tabletInterface.tabletNotification
     * @returns {Signal}
     * @deprecated This function is deprecated and will be removed. Use {@link Tablet.tabletNotification} instead.
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

/**jsdoc
 * @typedef {object} TabletProxy#ButtonList
 */
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
    int computeNewButtonIndex(const QVariantMap& newButtonProperties);
    using List = std::list<QSharedPointer<TabletButtonProxy>>;
    static QHash<int, QByteArray> _roles;
    static Qt::ItemFlags _flags;
    std::vector<QSharedPointer<TabletButtonProxy>> _buttons;
};

Q_DECLARE_METATYPE(TabletButtonListModel*);

class TabletButtonsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(int pageIndex READ pageIndex WRITE setPageIndex NOTIFY pageIndexChanged)
public:
    TabletButtonsProxyModel(QObject* parent = 0);
    int pageIndex() const;
    Q_INVOKABLE int buttonIndex(const QString& uuid);

public slots:
    void setPageIndex(int pageIndex);

signals:
    void pageIndexChanged(int pageIndex);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    int _pageIndex { -1 };
};

Q_DECLARE_METATYPE(TabletButtonsProxyModel*);

/**jsdoc
 * @class TabletProxy
  *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {string} name - Name of this tablet. <em>Read-only.</em>
 * @property {boolean} toolbarMode - Used to transition this tablet into and out of toolbar mode.
 *     When tablet is in toolbar mode, all its buttons will appear in a floating toolbar.
 * @property {boolean} landscape
 * @property {boolean} tabletShown <em>Read-only.</em>
 * @property {TabletProxy#ButtonList} buttons <em>Read-only.</em>
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
    void unfocus();

    /**jsdoc
     * @function TabletProxy#gotoMenuScreen
     * @param {string} [submenu=""]
     */
    Q_INVOKABLE void gotoMenuScreen(const QString& submenu = "");

    /**jsdoc
     * @function TabletProxy#initialScreen
     * @param {string} url
     */
    Q_INVOKABLE void initialScreen(const QVariant& url);

    /**jsdoc
     * Transition to the home screen.
     * @function TabletProxy#gotoHomeScreen
     */
    Q_INVOKABLE void gotoHomeScreen();

    /**jsdoc
     * Show the specified Web url on the tablet.
     * @function TabletProxy#gotoWebScreen
     * @param {string} url - URL of web page.
     * @param {string} [injectedJavaScriptUrl=""] - URL to an additional JS script to inject into the web page.
     * @param {boolean} [loadOtherBase=false]
     */
    Q_INVOKABLE void gotoWebScreen(const QString& url);
    Q_INVOKABLE void gotoWebScreen(const QString& url, const QString& injectedJavaScriptUrl, bool loadOtherBase = false);

    /**jsdoc
     * @function TabletProxy#loadQMLSource
     * @param {string} path
     * @param {boolean} [resizable=false]
     */
    Q_INVOKABLE void loadQMLSource(const QVariant& path, bool resizable = false);
    // FIXME: This currently relies on a script initializing the tablet (hence the bool denoting success);
    //        it should be initialized internally so it cannot fail

    /**jsdoc
     * @function TabletProxy#pushOntoStack
     * @param {string} path
     * @returns {boolean}
     */
    Q_INVOKABLE bool pushOntoStack(const QVariant& path);

    /**jsdoc
     * @function TabletProxy#popFromStack
     */
    Q_INVOKABLE void popFromStack();

    /**jsdoc
     * @function TabletProxy#loadQMLOnTop
     * @param {string} path
     */
    Q_INVOKABLE void loadQMLOnTop(const QVariant& path);

    /**jsdoc
     * @function TabletProxy#loadWebScreenOnTop
     * @param {string} path
     * @param {string} [injectedJavaScriptURL=""]
     */
    Q_INVOKABLE void loadWebScreenOnTop(const QVariant& url);
    Q_INVOKABLE void loadWebScreenOnTop(const QVariant& url, const QString& injectedJavaScriptUrl);

    /**jsdoc
     * @function TabletProxy#returnToPreviousApp
     */
    Q_INVOKABLE void returnToPreviousApp();

    /**jsdoc
     * Check if the tablet has a message dialog open.
     * @function TabletProxy#isMessageDialogOpen
     * @returns {boolean}
     */
    Q_INVOKABLE bool isMessageDialogOpen();

    /**jsdoc
    * Close any open dialogs.
    * @function TabletProxy#closeDialog
    */
    Q_INVOKABLE void closeDialog();

    /**jsdoc
     * Creates a new button, adds it to this and returns it.
     * @function TabletProxy#addButton
     * @param {object} properties - Button properties.
     * @returns {TabletButtonProxy}
     */
    //FIXME: UI_TABLET_HACK: enumerate the button properties when we figure out what they should be!
    Q_INVOKABLE TabletButtonProxy* addButton(const QVariant& properties);

    /**jsdoc
     * Removes a button from the tablet.
     * @function TabletProxy#removeButton
     * @param {TabletButtonProxy} button - The button to be removed
     */
    Q_INVOKABLE void removeButton(TabletButtonProxy* tabletButtonProxy);

    /**jsdoc
     * Used to send an event to the HTML/JavaScript embedded in the tablet.
     * @function TabletProxy#emitScriptEvent
     * @param {object|string} message
     */
    Q_INVOKABLE void emitScriptEvent(const QVariant& msg);

    /**jsdoc
     * Used to send an event to the QML embedded in the tablet.
     * @function TabletProxy#sendToQml
     * @param {object|string} message
     */
    Q_INVOKABLE void sendToQml(const QVariant& msg);

    /**jsdoc
     * Check if the tablet is on the home screen.
     * @function TabletProxy#onHomeScreen
     * @returns {boolean}
     */
    Q_INVOKABLE bool onHomeScreen();

    /**jsdoc
     * Set tablet into or out of landscape mode.
     * @function TabletProxy#setLandscape
     * @param {boolean} landscape - <code>true</code> for landscape, <ode>false</code> for portrait.
     */
    Q_INVOKABLE void setLandscape(bool landscape) { _landscape = landscape; }

    /**jsdoc
     * @function TabletProxy#getLandscape
     * @returns {boolean}
     */
    Q_INVOKABLE bool getLandscape() { return _landscape; }

    /**jsdoc
     * @function TabletProxy#isPathLoaded
     * @param {string} path
     * @returns {boolean}
     */
    Q_INVOKABLE bool isPathLoaded(const QVariant& path);

    QQuickItem* getTabletRoot() const { return _qmlTabletRoot; }

    OffscreenQmlSurface* getTabletSurface();

    QQuickItem* getQmlTablet() const;

    QQuickItem* getQmlMenu() const;

    TabletButtonListModel* getButtons() { return &_buttons; }

signals:
    /**jsdoc
     * Signaled when this tablet receives an event from the html/js embedded in the tablet.
     * @function TabletProxy#webEventReceived
     * @param {object|string} message
     * @returns {Signal}
     */
    void webEventReceived(QVariant msg);

    /**jsdoc
     * Signaled when this tablet receives an event from the qml embedded in the tablet.
     * @function TabletProxy#fromQml
     * @param {object|string} message
     * @returns {Signal}
     */
    void fromQml(QVariant msg);

    /**jsdoc
     * Signaled when this tablet screen changes.
     * @function TabletProxy#screenChanged
     * @param type {string} - "Home", "Web", "Menu", "QML", "Closed".
     * @param url {string} - Only valid for Web and QML.
     */
    void screenChanged(QVariant type, QVariant url);

    /**jsdoc
     * Signaled when the tablet becomes visible or becomes invisible.
     * @function TabletProxy#isTabletShownChanged
     * @returns {Signal}
     */
    void tabletShownChanged();

    /**jsdoc
     * @function TabletProxy#toolbarModeChanged
     */
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

private:
    void stopQMLSource();
};

Q_DECLARE_METATYPE(TabletProxy*);

/**jsdoc
 * @class TabletButtonProxy
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {Uuid} uuid - Uniquely identifies this button. <em>Read-only.</em>
 * @property {TabletButtonProxy.ButtonProperties} properties
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
     * Returns the current value of this button's properties.
     * @function TabletButtonProxy#getProperties
     * @returns {TabletButtonProxy.ButtonProperties}
     */
    Q_INVOKABLE QVariantMap getProperties();

    /**jsdoc
     * Replace the values of some of this button's properties.
     * @function TabletButtonProxy#editProperties
     * @param {TabletButtonProxy.ButtonProperties} properties - Set of properties to change.
     */
    Q_INVOKABLE void editProperties(const QVariantMap& properties);

signals:
    /**jsdoc
     * Triggered when this button has been clicked on by the user.
     * @function TabletButtonProxy#clicked
     * @returns {Signal}
     */
    void clicked();

    /**jsdoc
     * @function TabletButtonProxy#propertiesChanged
     * @returns {Signal}
     */
    void propertiesChanged();

protected:
    QUuid _uuid;
    int _stableOrder;

    /**jsdoc
     * @typedef {object} TabletButtonProxy.ButtonProperties
     * @property {string} icon - URL to button icon. (50 x 50)
     * @property {string} hoverIcon - URL to button icon, displayed during mouse hover. (50 x 50)
     * @property {string} activeHoverIcon - URL to button icon used when button is active, and during mouse hover. (50 x 50)
     * @property {string} activeIcon - URL to button icon used when button is active. (50 x 50)
     * @property {string} text - Button caption.
     * @property {string} hoverText - Button caption when button is not-active but during mouse hover.
     * @property {string} activeText - Button caption when button is active.
     * @property {string} activeHoverText - Button caption when button is active and during mouse hover.
     * @property {boolean} isActive - <code>true</code> when button is active.
     * @property {number} sortOrder - Determines sort order on tablet.  lower numbers will appear before larger numbers. 
     *     Default is 100.
     */
    // FIXME: There are additional properties.
    QVariantMap _properties;
};

Q_DECLARE_METATYPE(TabletButtonProxy*);

#endif // hifi_TabletScriptingInterface_h
