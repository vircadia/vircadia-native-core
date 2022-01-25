//
//  Created by Anthony J. Thibault on 2016-12-12
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TabletScriptingInterface.h"

#include <QtCore/QThread>
#include <QtQml/QQmlProperty>

#include <shared/QtHelpers.h>
#include <shared/LocalFileAccessGate.h>
#include <PathUtils.h>
#include <DependencyManager.h>
#include <AccountManager.h>
#include <RegisteredMetaTypes.h>

#include "../QmlWindowClass.h"
#include "../OffscreenUi.h"
#include "../InfoView.h"
#include "ToolbarScriptingInterface.h"
#include "Logging.h"

#include <AudioInjectorManager.h>

#include "SettingHandle.h"

// FIXME move to global app properties
const QString SYSTEM_TOOLBAR = "com.highfidelity.interface.toolbar.system";
const QString SYSTEM_TABLET = "com.highfidelity.interface.tablet.system";
const QString TabletScriptingInterface::QML = "hifi/tablet/TabletRoot.qml";
const QString BUTTON_SORT_ORDER_KEY = "sortOrder";
const int DEFAULT_BUTTON_SORT_ORDER = 100;

static QString getUsername() {
    QString username = "Unknown user";
    auto accountManager = DependencyManager::get<AccountManager>();
    if (accountManager->isLoggedIn()) {
        username = accountManager->getAccountInfo().getUsername();
    } 
    return username;
}

static Setting::Handle<QStringList> tabletSoundsButtonClick("TabletSounds", QStringList { "/sounds/Button06.wav",
                                                                               "/sounds/Button04.wav",
                                                                               "/sounds/Button07.wav",
                                                                               "/sounds/Tab01.wav",
                                                                               "/sounds/Tab02.wav" });

TabletButtonListModel::TabletButtonListModel() {

}

TabletButtonListModel::~TabletButtonListModel() {

}

enum ButtonDeviceRole {
    ButtonProxyRole = Qt::UserRole,
};

QHash<int, QByteArray> TabletButtonListModel::_roles{
    { ButtonProxyRole, "buttonProxy" },
};

Qt::ItemFlags TabletButtonListModel::_flags{ Qt::ItemIsSelectable | Qt::ItemIsEnabled };

QVariant TabletButtonListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= rowCount() || role != ButtonProxyRole) {
        return QVariant();
    }

    return QVariant::fromValue(_buttons.at(index.row()).data());
}

TabletButtonProxy* TabletButtonListModel::addButton(const QVariant& properties) {
    QVariantMap newProperties = properties.toMap();
    if (newProperties.find(BUTTON_SORT_ORDER_KEY) == newProperties.end()) {
        newProperties[BUTTON_SORT_ORDER_KEY] = DEFAULT_BUTTON_SORT_ORDER;
    }
    int index = computeNewButtonIndex(newProperties);
    auto button = QSharedPointer<TabletButtonProxy>::create(newProperties);
    beginResetModel();
    int numButtons = (int)_buttons.size();
    if (index < numButtons) {
        _buttons.insert(_buttons.begin() + index, button);
    } else {
        _buttons.push_back(button);
    }
    endResetModel();
    return button.data();
}

void TabletButtonListModel::removeButton(TabletButtonProxy* button) {
    auto itr = std::find(_buttons.begin(), _buttons.end(), button);
    if (itr == _buttons.end()) {
        qCWarning(uiLogging) << "TabletProxy::removeButton() could not find button " << button;
        return;
    }
    beginResetModel();
    _buttons.erase(itr);
    endResetModel();
}

int TabletButtonListModel::computeNewButtonIndex(const QVariantMap& newButtonProperties) {
    int numButtons = (int)_buttons.size();
    int newButtonSortOrder = newButtonProperties[BUTTON_SORT_ORDER_KEY].toInt();
    if (newButtonSortOrder == DEFAULT_BUTTON_SORT_ORDER) return numButtons;
    for (int i = 0; i < numButtons; i++) {
        QVariantMap tabletButtonProperties = _buttons[i]->getProperties();
        int tabletButtonSortOrder = tabletButtonProperties[BUTTON_SORT_ORDER_KEY].toInt();
        if (newButtonSortOrder <= tabletButtonSortOrder) {
            return i;
        }
    }
    return numButtons;
}

TabletButtonsProxyModel::TabletButtonsProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
}

int TabletButtonsProxyModel::pageIndex() const {
    return _pageIndex;
}

int TabletButtonsProxyModel::buttonIndex(const QString &uuid) {
    if (!sourceModel() || _pageIndex < 0) {
        return -1;
    }
    TabletButtonListModel* model = static_cast<TabletButtonListModel*>(sourceModel());
    for (int i = 0; i < model->rowCount(); i++) {
        TabletButtonProxy* bproxy = model->data(model->index(i), ButtonProxyRole).value<TabletButtonProxy*>();
        if (bproxy && bproxy->getUuid().toString().contains(uuid)) {
            return i - (_pageIndex*TabletScriptingInterface::ButtonsOnPage);
        }
    }
    return -1;
}

void TabletButtonsProxyModel::setPageIndex(int pageIndex) {
    if (_pageIndex == pageIndex)
        return;

    _pageIndex = pageIndex;
    invalidateFilter();
    emit pageIndexChanged(_pageIndex);
}

bool TabletButtonsProxyModel::filterAcceptsRow(int sourceRow,
                                               const QModelIndex &sourceParent) const {
    Q_UNUSED(sourceParent);
    return (sourceRow >= _pageIndex*TabletScriptingInterface::ButtonsOnPage
            && sourceRow < (_pageIndex + 1)*TabletScriptingInterface::ButtonsOnPage);
}

TabletScriptingInterface::TabletScriptingInterface() {
    qmlRegisterType<TabletScriptingInterface>("TabletScriptingInterface", 1, 0, "TabletEnums");
    qmlRegisterType<TabletButtonsProxyModel>("TabletScriptingInterface", 1, 0, "TabletButtonsProxyModel");
}

TabletScriptingInterface::~TabletScriptingInterface() {
    tabletSoundsButtonClick.set(tabletSoundsButtonClick.get());
}

ToolbarProxy* TabletScriptingInterface::getSystemToolbarProxy() {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    return _toolbarScriptingInterface->getToolbar(SYSTEM_TOOLBAR);
}

TabletProxy* TabletScriptingInterface::getTablet(const QString& tabletId) {
    TabletProxy* tabletProxy = nullptr;
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "getTablet", Q_RETURN_ARG(TabletProxy*, tabletProxy), Q_ARG(QString, tabletId));
        return tabletProxy;
    } 

    auto iter = _tabletProxies.find(tabletId);
    if (iter != _tabletProxies.end()) {
        // tablet already exists
        return iter->second;
    } else {
        // tablet must be created
        tabletProxy = new TabletProxy(this, tabletId);
        _tabletProxies[tabletId] = tabletProxy;
    }

    assert(tabletProxy);
    // initialize new tablet
    tabletProxy->setToolbarMode(_toolbarMode);
    return tabletProxy;
}

void TabletScriptingInterface::preloadSounds() {
    //preload audio events
    const QStringList &audioSettings = tabletSoundsButtonClick.get();
    for (int i = 0; i < TabletAudioEvents::Last; i++) {
        SharedSoundPointer sound = DependencyManager::get<SoundCache>()->
                getSound(PathUtils::resourcesUrl(audioSettings.at(i)));
        _audioEvents.insert(static_cast<TabletAudioEvents>(i), sound);
    }
}

void TabletScriptingInterface::playSound(TabletAudioEvents aEvent) {
    SharedSoundPointer sound = _audioEvents[aEvent];
    if (sound) {
        AudioInjectorOptions options;
        options.stereo = sound->isStereo();
        options.ambisonic = sound->isAmbisonic();
        options.localOnly = true;
        options.positionSet = false;    // system sound

        DependencyManager::get<AudioInjectorManager>()->playSound(sound, options, true);
    }
}

void TabletScriptingInterface::setToolbarMode(bool toolbarMode) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    _toolbarMode = toolbarMode;
    for (auto& iter : _tabletProxies) {
        iter.second->setToolbarMode(toolbarMode);
    }
}

void TabletScriptingInterface::setQmlTabletRoot(QString tabletId, OffscreenQmlSurface* qmlOffscreenSurface) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    TabletProxy* tablet = qobject_cast<TabletProxy*>(getTablet(tabletId));
    if (tablet) {
        tablet->setQmlTabletRoot(qmlOffscreenSurface);
    } else {
        qCWarning(uiLogging) << "TabletScriptingInterface::setupTablet() bad tablet object";
    }
}

QQuickWindow* TabletScriptingInterface::getTabletWindow() {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    TabletProxy* tablet = qobject_cast<TabletProxy*>(getTablet(SYSTEM_TABLET));
    if (!tablet) {
        return nullptr;
    }

    auto* qmlSurface = tablet->getTabletSurface();
    if (!qmlSurface) {
        return nullptr;
    }

    return qmlSurface->getWindow();
}

void TabletScriptingInterface::processMenuEvents(QObject* object, const QKeyEvent* event) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    switch (event->key()) {
        case Qt::Key_Down:
            QMetaObject::invokeMethod(object, "nextItem");
            break;

        case Qt::Key_Up:
            QMetaObject::invokeMethod(object, "previousItem");
            break;

        case Qt::Key_Left:
            QMetaObject::invokeMethod(object, "previousPage");
            break;

        case Qt::Key_Right:
            QMetaObject::invokeMethod(object, "selectCurrentItem");
            break;

        case Qt::Key_Return:
            QMetaObject::invokeMethod(object, "selectCurrentItem");
            break;

        default:
            break;
    }
}

void TabletScriptingInterface::processTabletEvents(QObject* object, const QKeyEvent* event) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    switch (event->key()) {
        case Qt::Key_Down:
            QMetaObject::invokeMethod(object, "downItem");
            break;

        case Qt::Key_Up:
            QMetaObject::invokeMethod(object, "upItem");
            break;

        case Qt::Key_Left:
            QMetaObject::invokeMethod(object, "previousItem");
            break;

        case Qt::Key_Right:
            QMetaObject::invokeMethod(object, "nextItem");
            break;

        case Qt::Key_Return:
            QMetaObject::invokeMethod(object, "selectItem");
            break;

        default:
            break;
    }
}

void TabletScriptingInterface::processEvent(const QKeyEvent* event) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    TabletProxy* tablet = qobject_cast<TabletProxy*>(getTablet(SYSTEM_TABLET));
    QObject* qmlTablet = tablet->getQmlTablet();
    QObject* qmlMenu = tablet->getQmlMenu();

    if (qmlTablet) {
        processTabletEvents(qmlTablet, event);
    } else if (qmlMenu) {
        processMenuEvents(qmlMenu, event);
    }
}

QObject* TabletScriptingInterface::getFlags() {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    auto offscreenUI = DependencyManager::get<OffscreenUi>();
    return offscreenUI ? offscreenUI->getFlags() : nullptr;
}

//
// TabletProxy
//

static const char* TABLET_HOME_SOURCE_URL = "hifi/tablet/TabletHome.qml";
static const char* VRMENU_SOURCE_URL = "hifi/tablet/TabletMenu.qml";

class TabletRootWindow : public QmlWindowClass {
    virtual QString qmlSource() const override { return "hifi/tablet/WindowRoot.qml"; }
public:
    TabletRootWindow() : QmlWindowClass(false) {}
};

TabletProxy::TabletProxy(QObject* parent, const QString& name) : QObject(parent), _name(name) {
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(uiLogging) << "Creating tablet proxy on wrong thread " << _name;
    }
    connect(this, &TabletProxy::tabletShownChanged, this, &TabletProxy::onTabletShown);
}

TabletProxy::~TabletProxy() {
    if (QThread::currentThread() != thread()) {
        qCWarning(uiLogging) << "Destroying tablet proxy on wrong thread" << _name;
    }
    disconnect(this, &TabletProxy::tabletShownChanged, this, &TabletProxy::onTabletShown);
}

void TabletProxy::setToolbarMode(bool toolbarMode) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setToolbarMode", Q_ARG(bool, toolbarMode));
        return;
    }

    if (toolbarMode == _toolbarMode) {
        return;
    }

    _toolbarMode = toolbarMode;

    if (toolbarMode) {
#if !defined(DISABLE_QML)
        closeDialog();
        // create new desktop window
        auto tabletRootWindow = new TabletRootWindow();
        tabletRootWindow->initQml(QVariantMap());
        auto quickItem = tabletRootWindow->asQuickItem();
        _desktopWindow = tabletRootWindow;
        QMetaObject::invokeMethod(quickItem, "setShown", Q_ARG(const QVariant&, QVariant(false)));

        QObject::connect(quickItem, SIGNAL(windowClosed()), this, SLOT(desktopWindowClosed()));

        QObject::connect(tabletRootWindow, SIGNAL(webEventReceived(QVariant)), this, SLOT(emitWebEvent(QVariant)), Qt::DirectConnection);
        QObject::connect(quickItem, SIGNAL(screenChanged(QVariant, QVariant)), this, SIGNAL(screenChanged(QVariant, QVariant)), Qt::DirectConnection);

        // forward qml surface events to interface js
        connect(tabletRootWindow, &QmlWindowClass::fromQml, this, &TabletProxy::fromQml);
#endif
    } else {
        if (_currentPathLoaded != TABLET_HOME_SOURCE_URL) {
            loadHomeScreen(true);
        }

        auto offscreenUI = DependencyManager::get<OffscreenUi>();
        if (offscreenUI) {
            //check if running scripts window opened and save it for reopen in Tablet
            if (offscreenUI->isVisible("RunningScripts")) {
                offscreenUI->hide("RunningScripts");
                _showRunningScripts = true;
            }

            offscreenUI->hideDesktopWindows();
        }

        // destroy desktop window
        if (_desktopWindow) {
            _desktopWindow->deleteLater();
            _desktopWindow = nullptr;
        }
    }

    emit toolbarModeChanged();
}

void TabletProxy::initialScreen(const QVariant& url) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "initialScreen", Q_ARG(QVariant, url));
        return;
    }

    if (_qmlTabletRoot) {
        pushOntoStack(url);
    } else {
        _initialScreen = true;
        _initialPath.first = url;
        _initialPath.second = State::QML;
    }
}

bool TabletProxy::isMessageDialogOpen() {
    if (QThread::currentThread() != thread()) {
        bool result = false;
        BLOCKING_INVOKE_METHOD(this, "isMessageDialogOpen", Q_RETURN_ARG(bool, result));
        return result;
    }

    if (!_qmlTabletRoot) {
        return false;
    }

    QVariant result;
    QMetaObject::invokeMethod(_qmlTabletRoot, "isDialogOpen",Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, result));
    return result.toBool();
}

void TabletProxy::closeDialog() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "closeDialog");
        return;
    }

    if (!_qmlTabletRoot) {
        return;
    }

    QMetaObject::invokeMethod(_qmlTabletRoot, "closeDialog");
}

void TabletProxy::emitWebEvent(const QVariant& msg) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitWebEvent", Q_ARG(QVariant, msg));
        return;
    }
    emit webEventReceived(msg);
}

void TabletProxy::onTabletShown() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "onTabletShown");
        return;
    }

    if (_tabletShown) {
        Setting::Handle<bool> notificationSounds{ QStringLiteral("play_notification_sounds"), true};
        Setting::Handle<bool> notificationSoundTablet{ QStringLiteral("play_notification_sounds_tablet"), true};
        if (notificationSounds.get() && notificationSoundTablet.get()) {
            dynamic_cast<TabletScriptingInterface*>(parent())->playSound(TabletScriptingInterface::TabletOpen);
        }
        if (_showRunningScripts) {
            _showRunningScripts = false;
            pushOntoStack("hifi/dialogs/TabletRunningScripts.qml");
        }
        if (_currentPathLoaded == TABLET_HOME_SOURCE_URL) {
            loadHomeScreen(true);
        }
    }
}

bool TabletProxy::isPathLoaded(const QVariant& path) {
    if (QThread::currentThread() != thread()) {
        bool result = false;
        BLOCKING_INVOKE_METHOD(this, "isPathLoaded", Q_RETURN_ARG(bool, result), Q_ARG(QVariant, path));
        return result;
    }

    return path.toString() == _currentPathLoaded.toString();
}

void TabletProxy::setQmlTabletRoot(OffscreenQmlSurface* qmlOffscreenSurface) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setQmlTabletRoot", Q_ARG(OffscreenQmlSurface*, qmlOffscreenSurface));
        return;
    }

    _qmlOffscreenSurface = qmlOffscreenSurface;
    _qmlTabletRoot = qmlOffscreenSurface ? qmlOffscreenSurface->getRootItem() : nullptr;
    if (_qmlTabletRoot && _qmlOffscreenSurface) {
        QObject::connect(_qmlOffscreenSurface, SIGNAL(webEventReceived(QVariant)), this, SLOT(emitWebEvent(QVariant)));
        QObject::connect(_qmlTabletRoot, SIGNAL(screenChanged(QVariant, QVariant)), this, SIGNAL(screenChanged(QVariant, QVariant)));

        // forward qml surface events to interface js
        connect(_qmlOffscreenSurface, &OffscreenQmlSurface::fromQml, [this](QVariant message) {
            if (message.canConvert<QJSValue>()) {
                emit fromQml(qvariant_cast<QJSValue>(message).toVariant());
            } else if (message.canConvert<QString>()) {
                emit fromQml(message.toString());
            } else {
                qWarning() << "fromQml: Unsupported message type " << message;
            }
        });

        if (_toolbarMode) {
            QMetaObject::invokeMethod(_qmlTabletRoot, "loadSource", Q_ARG(const QVariant&, QVariant(TABLET_HOME_SOURCE_URL)));
        }

        // force to the tablet to go to the homescreen
        loadHomeScreen(true);

        QMetaObject::invokeMethod(_qmlTabletRoot, "setUsername", Q_ARG(const QVariant&, QVariant(getUsername())));

        // hook up username changed signal.
        auto accountManager = DependencyManager::get<AccountManager>();
        QObject::connect(accountManager.data(), &AccountManager::profileChanged, [this]() {
            if (_qmlTabletRoot) {
                QMetaObject::invokeMethod(_qmlTabletRoot, "setUsername", Q_ARG(const QVariant&, QVariant(getUsername())));
            }
        });

        if (_initialScreen) {
            if (!_showRunningScripts && _initialPath.second == State::QML) {
                pushOntoStack(_initialPath.first);
            } else if (_initialPath.second == State::Web) {
                QVariant webUrl = _initialPath.first;
                QVariant scriptUrl = _initialWebPathParams.first;
                gotoWebScreen(webUrl.toString(), scriptUrl.toString(), _initialWebPathParams.second);
            }
            _initialScreen = false;
            _initialPath.first = "";
            _initialPath.second = State::Uninitialized;
            _initialWebPathParams.first = "";
            _initialWebPathParams.second = false;
        }

        if (_showRunningScripts) {
            //show Tablet. Make sure, setShown implemented in TabletRoot.qml
            QMetaObject::invokeMethod(_qmlTabletRoot, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        }
    } else {
        _state = State::Uninitialized;
        emit screenChanged(QVariant("Closed"), QVariant(""));
        _currentPathLoaded = "";
    }
}

void TabletProxy::gotoHomeScreen() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "gotoHomeScreen");
        return;
    }
    loadHomeScreen(false);
}

void TabletProxy::gotoMenuScreen(const QString& submenu) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "gotoMenuScreen", Q_ARG(QString, submenu));
        return;
    }

    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    auto offscreenUI = DependencyManager::get<OffscreenUi>();
    if (root && offscreenUI) {
        QObject* menu = offscreenUI->getRootMenu();
        QMetaObject::invokeMethod(root, "setMenuProperties", Q_ARG(QVariant, QVariant::fromValue(menu)), Q_ARG(const QVariant&, QVariant(submenu)));
        QMetaObject::invokeMethod(root, "loadSource", Q_ARG(const QVariant&, QVariant(VRMENU_SOURCE_URL)));
        _state = State::Menu;
        _currentPathLoaded = VRMENU_SOURCE_URL;
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        if (_toolbarMode && _desktopWindow) {
            QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(false)));
        }
    }
}

void TabletProxy::loadQMLOnTopImpl(const QVariant& path, bool localSafeContext) {
    if (QThread::currentThread() != thread()) {
        qCWarning(uiLogging) << __FUNCTION__ << "may not be called directly by scripts";
        return;
    }

     QObject* root = nullptr;
     if (!_toolbarMode && _qmlTabletRoot) {
         root = _qmlTabletRoot;
     } else if (_toolbarMode && _desktopWindow) {
         root = _desktopWindow->asQuickItem();
     }

     if (root) {
         if (localSafeContext) {
             hifi::scripting::setLocalAccessSafeThread(true);
         }
         QMetaObject::invokeMethod(root, "loadQMLOnTop", Q_ARG(const QVariant&, path));
         QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
         if (_toolbarMode && _desktopWindow) {
             QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(false)));
         }
         hifi::scripting::setLocalAccessSafeThread(false);
     } else {
         qCDebug(uiLogging) << "tablet cannot load QML because _qmlTabletRoot is null";
     }
}

void TabletProxy::loadQMLOnTop(const QVariant& path) {
    bool localSafeContext = hifi::scripting::isLocalAccessSafeThread();
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadQMLOnTopImpl", Q_ARG(QVariant, path), Q_ARG(bool, localSafeContext));
        return;
    }

    loadQMLOnTopImpl(path, localSafeContext);
}

void TabletProxy::returnToPreviousAppImpl(bool localSafeContext) {
    if (QThread::currentThread() != thread()) {
        qCWarning(uiLogging) << __FUNCTION__ << "may not be called directly by scripts";
        return;

    }

    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        if (localSafeContext) {
            hifi::scripting::setLocalAccessSafeThread(true);
        }
        QMetaObject::invokeMethod(root, "returnToPreviousApp");
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        hifi::scripting::setLocalAccessSafeThread(false);
    } else {
        qCDebug(uiLogging) << "tablet cannot load QML because _qmlTabletRoot is null";
    }
}

void TabletProxy::returnToPreviousApp() {
    bool localSafeContext = hifi::scripting::isLocalAccessSafeThread();
    qDebug() << "TabletProxy::returnToPreviousApp -> localSafeContext: " << localSafeContext;
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "returnToPreviousAppImpl", Q_ARG(bool, localSafeContext));
        return;
    }

    returnToPreviousAppImpl(localSafeContext);
}

void TabletProxy::loadQMLSource(const QVariant& path, bool resizable) {
    // Capture whether the current script thread is allowed to load local HTML content, 
    // pass the information along to the real function
    bool localSafeContext = hifi::scripting::isLocalAccessSafeThread();
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadQMLSourceImpl", Q_ARG(QVariant, path), Q_ARG(bool, resizable), Q_ARG(bool, localSafeContext));
        return;
    }
    loadQMLSourceImpl(path, resizable, localSafeContext);
}

void TabletProxy::loadQMLSourceImpl(const QVariant& path, bool resizable, bool localSafeContext) {
    if (QThread::currentThread() != thread()) {
        qCWarning(uiLogging) << __FUNCTION__ << "may not be called directly by scripts";
        return;

    }
    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        // BUGZ-1398: tablet access to local HTML files from client scripts
        // Here we TEMPORARILY mark the main thread as allowed to load local file content, 
        // because the thread that originally made the call is so marked.  
        if (localSafeContext) {
            hifi::scripting::setLocalAccessSafeThread(true);
        }
        QMetaObject::invokeMethod(root, "loadSource", Q_ARG(const QVariant&, path));
        hifi::scripting::setLocalAccessSafeThread(false);
        _state = State::QML;
        _currentPathLoaded = path;
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        if (_toolbarMode && _desktopWindow) {
            QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(resizable)));
        }

    } else {
        qCDebug(uiLogging) << "tablet cannot load QML because _qmlTabletRoot is null";
    }
}

void TabletProxy::stopQMLSource() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopQMLSource");
        return;
    }

    // For desktop toolbar mode dialogs.
    if (!_toolbarMode || !_desktopWindow) {
        qCDebug(uiLogging) << "tablet cannot clear QML because not desktop toolbar mode";
        return;
    }

    auto root = _desktopWindow->asQuickItem();
    if (root) {
        QMetaObject::invokeMethod(root, "loadSource", Q_ARG(const QVariant&, ""));
        if (!_currentPathLoaded.toString().isEmpty()) {
            emit screenChanged(QVariant("QML"), "");
        }
        _currentPathLoaded = "";
        _state = State::Home;
    } else {
        qCDebug(uiLogging) << "tablet cannot clear QML because _desktopWindow is null";
    }
}

bool TabletProxy::pushOntoStack(const QVariant& path) {
    if (QThread::currentThread() != thread()) {
        bool result = false;
        BLOCKING_INVOKE_METHOD(this, "pushOntoStack", Q_RETURN_ARG(bool, result), Q_ARG(QVariant, path));
        return result;
    }

    //set landscape off when pushing menu items while in Create mode
    if (_landscape) {
        setLandscape(false);
    }

    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        auto stack = root->findChild<QQuickItem*>("stack");
        if (stack) {
            QMetaObject::invokeMethod(stack, "pushSource", Q_ARG(const QVariant&, path));
        } else {
            loadQMLSource(path);
        }
        if (_toolbarMode && _desktopWindow) {
            QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(false)));
        }
    } else {
        qCDebug(uiLogging) << "tablet cannot push QML because _qmlTabletRoot or _desktopWindow is null";
    }

    return (root != nullptr);
}

void TabletProxy::popFromStack() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "popFromStack");
        return;
    }

    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        auto stack = root->findChild<QQuickItem*>("stack");
        QMetaObject::invokeMethod(stack, "popSource");
    } else {
        qCDebug(uiLogging) << "tablet cannot pop QML because _qmlTabletRoot or _desktopWindow is null";
    }
}

void TabletProxy::loadHomeScreen(bool forceOntoHomeScreen) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadHomeScreen", Q_ARG(bool, forceOntoHomeScreen));
        return;
    }

    if ((_state != State::Home && _state != State::Uninitialized) || forceOntoHomeScreen) {
        if (!_toolbarMode && _qmlTabletRoot) {
            QMetaObject::invokeMethod(_qmlTabletRoot, "loadSource", Q_ARG(const QVariant&, QVariant(TABLET_HOME_SOURCE_URL)));
            QMetaObject::invokeMethod(_qmlTabletRoot, "playButtonClickSound");
        } else if (_toolbarMode && _desktopWindow) {
            // close desktop window
            if (_desktopWindow->asQuickItem()) {
                QMetaObject::invokeMethod(_desktopWindow->asQuickItem(), "setShown", Q_ARG(const QVariant&, QVariant(false)));
                stopQMLSource();  // Stop the currently loaded QML running.
            }
        }
        _state = State::Home;
        _currentPathLoaded = TABLET_HOME_SOURCE_URL;
    }
}

void TabletProxy::gotoWebScreen(const QString& url) {
    gotoWebScreen(url, "");
}

void TabletProxy::loadWebScreenOnTop(const QVariant& url) {
    loadWebScreenOnTop(url, "");
}

void TabletProxy::loadWebScreenOnTop(const QVariant& url, const QString& injectJavaScriptUrl) {
    bool localSafeContext = hifi::scripting::isLocalAccessSafeThread();
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadHTMLSourceOnTopImpl", Q_ARG(QString, url.toString()), Q_ARG(QString, injectJavaScriptUrl), Q_ARG(bool, false), Q_ARG(bool, localSafeContext));
        return;
    }

    loadHTMLSourceOnTopImpl(url.toString(), injectJavaScriptUrl, false, localSafeContext);
}

void TabletProxy::gotoWebScreen(const QString& url, const QString& injectedJavaScriptUrl, bool loadOtherBase) {
    bool localSafeContext = hifi::scripting::isLocalAccessSafeThread();
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadHTMLSourceOnTopImpl", Q_ARG(QString, url), Q_ARG(QString, injectedJavaScriptUrl), Q_ARG(bool, loadOtherBase), Q_ARG(bool, localSafeContext));
        return;
    }

    loadHTMLSourceOnTopImpl(url, injectedJavaScriptUrl, loadOtherBase, localSafeContext);
}

void TabletProxy::loadHTMLSourceOnTopImpl(const QString& url, const QString& injectedJavaScriptUrl, bool loadOtherBase, bool localSafeContext) {
    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        if (localSafeContext) {
            hifi::scripting::setLocalAccessSafeThread(true);
        }
        if (loadOtherBase) {
            QMetaObject::invokeMethod(root, "loadTabletWebBase", Q_ARG(const QVariant&, QVariant(url)), Q_ARG(const QVariant&, QVariant(injectedJavaScriptUrl)));
        } else {
            QMetaObject::invokeMethod(root, "loadWebBase", Q_ARG(const QVariant&, QVariant(url)), Q_ARG(const QVariant&, QVariant(injectedJavaScriptUrl)));
        }
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        if (_toolbarMode && _desktopWindow) {
            QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(false)));
        }

        hifi::scripting::setLocalAccessSafeThread(false);
        _state = State::Web;
        _currentPathLoaded = QVariant(url);
    } else {
        // tablet is not initialized yet, save information and load when
        // the tablet root is set
        _initialPath.first = url;
        _initialPath.second = State::Web;
        _initialWebPathParams.first = injectedJavaScriptUrl;
        _initialWebPathParams.second = loadOtherBase;
        _initialScreen = true;
    }
}

TabletButtonProxy* TabletProxy::addButton(const QVariant& properties) {
    if (QThread::currentThread() != thread()) {
        TabletButtonProxy* result = nullptr;
        BLOCKING_INVOKE_METHOD(this, "addButton", Q_RETURN_ARG(TabletButtonProxy*, result), Q_ARG(QVariant, properties));
        return result;
    }

    return _buttons.addButton(properties);
}

bool TabletProxy::onHomeScreen() {
    if (QThread::currentThread() != thread()) {
        bool result = false;
        BLOCKING_INVOKE_METHOD(this, "onHomeScreen", Q_RETURN_ARG(bool, result));
        return result;
    }

    return _state == State::Home;
}

void TabletProxy::removeButton(TabletButtonProxy* tabletButtonProxy) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "removeButton", Q_ARG(TabletButtonProxy*, tabletButtonProxy));
        return;
    }

    _buttons.removeButton(tabletButtonProxy);
}

void TabletProxy::emitScriptEvent(const QVariant& msg) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitScriptEvent", Q_ARG(QVariant, msg));
        return;
    }

    if (!_toolbarMode && _qmlOffscreenSurface) {
        QMetaObject::invokeMethod(_qmlOffscreenSurface, "emitScriptEvent", Qt::AutoConnection, Q_ARG(QVariant, msg));
    } else if (_toolbarMode && _desktopWindow) {
        QMetaObject::invokeMethod(_desktopWindow, "emitScriptEvent", Qt::AutoConnection, Q_ARG(QVariant, msg));
    }
}

void TabletProxy::sendToQml(const QVariant& msg) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "sendToQml", Q_ARG(QVariant, msg));
        return;
    }

    if (!_toolbarMode && _qmlOffscreenSurface) {
        QMetaObject::invokeMethod(_qmlOffscreenSurface, "sendToQml", Qt::AutoConnection, Q_ARG(QVariant, msg));
    } else if (_toolbarMode && _desktopWindow) {
        QMetaObject::invokeMethod(_desktopWindow, "sendToQml", Qt::AutoConnection, Q_ARG(QVariant, msg));
    }
}

OffscreenQmlSurface* TabletProxy::getTabletSurface() {
    if (QThread::currentThread() != thread()) {
        OffscreenQmlSurface* result = nullptr;
        BLOCKING_INVOKE_METHOD(this, "getTabletSurface", Q_RETURN_ARG(OffscreenQmlSurface*, result));
        return result;
    }

    return _qmlOffscreenSurface;
}


void TabletProxy::desktopWindowClosed() {
    gotoHomeScreen();
}

void TabletProxy::unfocus() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "unfocus");
        return;
    }

    if (_qmlOffscreenSurface) {
        _qmlOffscreenSurface->lowerKeyboard();
    }
}


QQuickItem* TabletProxy::getQmlTablet() const {
    if (!_qmlTabletRoot) {
        return nullptr;
    }

    auto loader = _qmlTabletRoot->findChild<QQuickItem*>("loader");
    if (!loader) {
        return nullptr;
    }

    auto tablet = loader->findChild<QQuickItem*>("tablet");
    if (!tablet) {
        return nullptr;
    }

    return tablet;
}

QQuickItem* TabletProxy::getQmlMenu() const {
    if (!_qmlTabletRoot) {
        return nullptr;
    }

    auto loader = _qmlTabletRoot->findChild<QQuickItem*>("loader");
    if (!loader) {
        return nullptr;
    }

    QQuickItem* VrMenu = loader->findChild<QQuickItem*>("tabletMenu");
    if (!VrMenu) {
        return nullptr;
    }

    QQuickItem* menuList = VrMenu->findChild<QQuickItem*>("tabletMenuHandlerItem");
    if (!menuList) {
        return nullptr;
    }
    return menuList;
}

//
// TabletButtonProxy
//

const QString UUID_KEY = "uuid";
const QString OBJECT_NAME_KEY = "objectName";
const QString STABLE_ORDER_KEY = "stableOrder";
static int s_stableOrder = 1;

/*@jsdoc
 * Properties of a tablet button.
 *
 * @typedef {object} TabletButtonProxy.ButtonProperties
 * 
 * @property {Uuid} uuid - The button ID. <em>Read-only.</em>
 * @property {Uuid} objectName - Synonym for <code>uuid</code>.
 * @property {number} stableOrder - The order in which the button was created: each button created gets a value incremented by 
 *     one.
 * 
 * @property {string} icon - The url of the default button icon displayed. (50 x 50 pixels. SVG, PNG, or other image format.) 
 * @property {string} hoverIcon - The url of the button icon displayed when the button is hovered and not active.
 * @property {string} activeIcon - The url of the button icon displayed when the button is active.
 * @property {string} activeHoverIcon - The url of the button icon displayed when the button is hovered and active.
 * @property {string} text - The button caption.
 * @property {string} hoverText - The button caption when the button is hovered and not active.
 * @property {string} activeText - The button caption when the button is active.
 * @property {string} activeHoverText - The button caption when the button is hovered and active.
 * @comment {string} defaultCaptionColor="#ffffff" - Internal property.
 * @property {string} captionColor="#ffffff" - The color of the button caption.
 
 * @property {boolean} isActive=false - <code>true</code> if the button is active, <code>false</code> if it isn't.
 * @property {boolean} isEntered - <code>true</code> if the button is being hovered, <code>false</code> if it isn't.
 * @property {boolean} buttonEnabled=true - <code>true</code> if the button is enabled, <code>false</code> if it is disabled.
 * @property {number} sortOrder=100 - Determines the order of the buttons: buttons with lower numbers appear before buttons
 *     with larger numbers.
 *
 * @property {boolean} inDebugMode - If <code>true</code> and the tablet is being used, the button's <code>isActive</code> 
 *     state toggles each time the button is clicked. <em>Tablet only.</em>
 *
 * @comment {object} tabletRoot - Internal tablet-only property.
 * @property {object} flickable - Internal tablet-only property.
 * @property {object} gridView - Internal tablet-only property.
 * @property {number} buttonIndex - Internal tablet-only property.
 *
 * @comment {number} imageOffOut - Internal toolbar-only property.
 * @comment {number} imageOffIn - Internal toolbar-only property.
 * @comment {number} imageOnOut - Internal toolbar-only property.
 * @comment {number} imageOnIn - Internal toolbar-only property.
 */
TabletButtonProxy::TabletButtonProxy(const QVariantMap& properties) :
    _uuid(QUuid::createUuid()),
    _stableOrder(++s_stableOrder),
    _properties(properties) {
    // this is used to uniquely identify this button.
    _properties[UUID_KEY] = _uuid;
    _properties[OBJECT_NAME_KEY] = _uuid.toString();
    _properties[STABLE_ORDER_KEY] = _stableOrder;
    // Other properties are defined in TabletButton.qml and ToolbarButton.qml.
    if (QThread::currentThread() != qApp->thread()) {
        qCWarning(uiLogging) << "Creating tablet button proxy on wrong thread";
    }
}

TabletButtonProxy::~TabletButtonProxy() {
    if (QThread::currentThread() != thread()) {
        qCWarning(uiLogging) << "Destroying tablet button proxy on wrong thread";
    }
}

QVariantMap TabletButtonProxy::getProperties() {
    if (QThread::currentThread() != thread()) {
        QVariantMap result;
        BLOCKING_INVOKE_METHOD(this, "getProperties", Q_RETURN_ARG(QVariantMap, result));
        return result;
    }

    return _properties;
}

void TabletButtonProxy::editProperties(const QVariantMap& properties) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "editProperties", Q_ARG(QVariantMap, properties));
        return;
    }

    bool changed = false;
    QVariantMap::const_iterator iter = properties.constBegin();
    while (iter != properties.constEnd()) {
        const auto& key = iter.key();
        const auto& value = iter.value();
        if (!_properties.contains(key) || _properties[key] != value) {
            _properties[key] = value;
            changed = true;
        }
        ++iter;
    }

    if (changed) {
        emit propertiesChanged();
    }
}
