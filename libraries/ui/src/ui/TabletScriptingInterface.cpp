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
#include <PathUtils.h>
#include <DependencyManager.h>
#include <AccountManager.h>
#include <RegisteredMetaTypes.h>

#include "../QmlWindowClass.h"
#include "../OffscreenUi.h"
#include "../InfoView.h"
#include "ToolbarScriptingInterface.h"
#include "Logging.h"

#include <AudioInjector.h>

#include "SettingHandle.h"

// FIXME move to global app properties
const QString SYSTEM_TOOLBAR = "com.highfidelity.interface.toolbar.system";
const QString SYSTEM_TABLET = "com.highfidelity.interface.tablet.system";
const QString TabletScriptingInterface::QML = "hifi/tablet/TabletRoot.qml";

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
    auto tabletButtonProxy = QSharedPointer<TabletButtonProxy>(new TabletButtonProxy(properties.toMap()));
    beginResetModel();
    _buttons.push_back(tabletButtonProxy);
    endResetModel();
    return tabletButtonProxy.data();
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

TabletScriptingInterface::TabletScriptingInterface() {
    qmlRegisterType<TabletScriptingInterface>("TabletScriptingInterface", 1, 0, "TabletEnums");
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
        QFileInfo inf = QFileInfo(PathUtils::resourcesPath() + audioSettings.at(i));
        SharedSoundPointer sound = DependencyManager::get<SoundCache>()->
                getSound(QUrl::fromLocalFile(inf.absoluteFilePath()));
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

        AudioInjectorPointer injector = AudioInjector::playSoundAndDelete(sound->getByteArray(), options);
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
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    return offscreenUi->getFlags();
}

//
// TabletProxy
//

static const char* TABLET_HOME_SOURCE_URL = "hifi/tablet/TabletHome.qml";
static const char* WEB_VIEW_SOURCE_URL = "hifi/tablet/TabletWebView.qml";
static const char* VRMENU_SOURCE_URL = "hifi/tablet/TabletMenu.qml";

class TabletRootWindow : public QmlWindowClass {
    virtual QString qmlSource() const override { return "hifi/tablet/WindowRoot.qml"; }
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

    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    if (toolbarMode) {
        // create new desktop window
        auto tabletRootWindow = new TabletRootWindow();
        tabletRootWindow->initQml(QVariantMap());
        auto quickItem = tabletRootWindow->asQuickItem();
        _desktopWindow = tabletRootWindow;
        QMetaObject::invokeMethod(quickItem, "setShown", Q_ARG(const QVariant&, QVariant(false)));

        QObject::connect(quickItem, SIGNAL(windowClosed()), this, SLOT(desktopWindowClosed()));

        QObject::connect(tabletRootWindow, SIGNAL(webEventReceived(QVariant)), this, SLOT(emitWebEvent(QVariant)), Qt::DirectConnection);

        // forward qml surface events to interface js
        connect(tabletRootWindow, &QmlWindowClass::fromQml, this, &TabletProxy::fromQml);
    } else {
        if (_currentPathLoaded != TABLET_HOME_SOURCE_URL) {
            loadHomeScreen(true);
        }
        //check if running scripts window opened and save it for reopen in Tablet
        if (offscreenUi->isVisible("RunningScripts")) {
            offscreenUi->hide("RunningScripts");
            _showRunningScripts = true;
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

void TabletProxy::emitWebEvent(const QVariant& msg) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "emitWebEvent", Q_ARG(QVariant, msg));
        return;
    }
    emit webEventReceived(msg);
}

void TabletProxy::onTabletShown() {
    if (_tabletShown) {
        static_cast<TabletScriptingInterface*>(parent())->playSound(TabletScriptingInterface::TabletOpen);
        if (_showRunningScripts) {
            _showRunningScripts = false;
            pushOntoStack("hifi/dialogs/TabletRunningScripts.qml");
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
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    _qmlOffscreenSurface = qmlOffscreenSurface;
    _qmlTabletRoot = qmlOffscreenSurface ? qmlOffscreenSurface->getRootItem() : nullptr;
    if (_qmlTabletRoot && _qmlOffscreenSurface) {
        QObject::connect(_qmlOffscreenSurface, SIGNAL(webEventReceived(QVariant)), this, SLOT(emitWebEvent(QVariant)));

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

    if (root) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        QObject* menu = offscreenUi->getRootMenu();
        QMetaObject::invokeMethod(root, "setMenuProperties", Q_ARG(QVariant, QVariant::fromValue(menu)), Q_ARG(const QVariant&, QVariant(submenu)));
        QMetaObject::invokeMethod(root, "loadSource", Q_ARG(const QVariant&, QVariant(VRMENU_SOURCE_URL)));
        _state = State::Menu;
        emit screenChanged(QVariant("Menu"), QVariant(VRMENU_SOURCE_URL));
        _currentPathLoaded = VRMENU_SOURCE_URL;
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        if (_toolbarMode && _desktopWindow) {
            QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(false)));
        }
    }
}

void TabletProxy::loadQMLOnTop(const QVariant& path) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadQMLOnTop", Q_ARG(QVariant, path));
        return;
    }

    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        QMetaObject::invokeMethod(root, "loadQMLOnTop", Q_ARG(const QVariant&, path));
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        if (_toolbarMode && _desktopWindow) {
            QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(false)));
        }
    } else {
        qCDebug(uiLogging) << "tablet cannot load QML because _qmlTabletRoot is null";
    }
}

void TabletProxy::returnToPreviousApp() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "returnToPreviousApp");
        return;
    }

    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        QMetaObject::invokeMethod(root, "returnToPreviousApp");
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
    } else {
        qCDebug(uiLogging) << "tablet cannot load QML because _qmlTabletRoot is null";
    }
}
    
void TabletProxy::loadQMLSource(const QVariant& path, bool resizable) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadQMLSource", Q_ARG(QVariant, path), Q_ARG(bool, resizable));
        return;
    }

    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        QMetaObject::invokeMethod(root, "loadSource", Q_ARG(const QVariant&, path));
        _state = State::QML;
        if (path != _currentPathLoaded) {
            emit screenChanged(QVariant("QML"), path);
        }
        _currentPathLoaded = path;
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        if (_toolbarMode && _desktopWindow) {
            QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(resizable)));
        }

    } else {
        qCDebug(uiLogging) << "tablet cannot load QML because _qmlTabletRoot is null";
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
            }
        }
        _state = State::Home;
        emit screenChanged(QVariant("Home"), QVariant(TABLET_HOME_SOURCE_URL));
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
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadWebScreenOnTop", Q_ARG(QVariant, url), Q_ARG(QString, injectJavaScriptUrl));
        return;
    }

    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        QMetaObject::invokeMethod(root, "loadQMLOnTop", Q_ARG(const QVariant&, QVariant(WEB_VIEW_SOURCE_URL)));
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        if (_toolbarMode && _desktopWindow) {
            QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(false)));
        }
        QMetaObject::invokeMethod(root, "loadWebOnTop", Q_ARG(const QVariant&, QVariant(url)), Q_ARG(const QVariant&, QVariant(injectJavaScriptUrl)));
    }
    _state = State::Web;
}

void TabletProxy::gotoWebScreen(const QString& url, const QString& injectedJavaScriptUrl, bool loadOtherBase) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "gotoWebScreen", Q_ARG(QString, url), Q_ARG(QString, injectedJavaScriptUrl), Q_ARG(bool, loadOtherBase));
        return;
    }

    QObject* root = nullptr;
    if (!_toolbarMode && _qmlTabletRoot) {
        root = _qmlTabletRoot;
    } else if (_toolbarMode && _desktopWindow) {
        root = _desktopWindow->asQuickItem();
    }

    if (root) {
        if (loadOtherBase) {
            QMetaObject::invokeMethod(root, "loadTabletWebBase", Q_ARG(const QVariant&, QVariant(url)), Q_ARG(const QVariant&, QVariant(injectedJavaScriptUrl)));
        } else {
            QMetaObject::invokeMethod(root, "loadWebBase", Q_ARG(const QVariant&, QVariant(url)), Q_ARG(const QVariant&, QVariant(injectedJavaScriptUrl)));
        }
        QMetaObject::invokeMethod(root, "setShown", Q_ARG(const QVariant&, QVariant(true)));
        if (_toolbarMode && _desktopWindow) {
            QMetaObject::invokeMethod(root, "setResizable", Q_ARG(const QVariant&, QVariant(false)));
        }
        _state = State::Web;
        emit screenChanged(QVariant("Web"), QVariant(url));
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
    return _qmlOffscreenSurface;
}


void TabletProxy::desktopWindowClosed() {
    gotoHomeScreen();
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

TabletButtonProxy::TabletButtonProxy(const QVariantMap& properties) :
    _uuid(QUuid::createUuid()),
    _stableOrder(++s_stableOrder),
    _properties(properties) {
    // this is used to uniquely identify this button.
    _properties[UUID_KEY] = _uuid;
    _properties[OBJECT_NAME_KEY] = _uuid.toString();
    _properties[STABLE_ORDER_KEY] = _stableOrder;
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
