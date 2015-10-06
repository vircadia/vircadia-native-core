#include "PluginContainerProxy.h"

#include <QScreen>
#include <QWindow>

#include <plugins/Plugin.h>
#include <plugins/PluginManager.h>
#include <display-plugins/DisplayPlugin.h>

#include "Application.h"
#include "MainWindow.h"
#include "GLCanvas.h"
#include "ui/DialogsManager.h"

PluginContainerProxy::PluginContainerProxy() {
    Plugin::setContainer(this);
}

bool PluginContainerProxy::isForeground() {
    return qApp->_isForeground && !qApp->getWindow()->isMinimized();
}

void PluginContainerProxy::addMenu(const QString& menuName) {
    Menu::getInstance()->addMenu(menuName);
}

void PluginContainerProxy::removeMenu(const QString& menuName) {
    Menu::getInstance()->removeMenu(menuName);
}

extern bool _activatingDisplayPlugin;
extern QVector<QPair<QString, QString>> _currentDisplayPluginActions;
extern QVector<QPair<QString, QString>> _currentInputPluginActions;
std::map<QString, QActionGroup*> _exclusiveGroups;

QAction* PluginContainerProxy::addMenuItem(const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable, bool checked, const QString& groupName) {
    auto menu = Menu::getInstance();
    MenuWrapper* parentItem = menu->getMenu(path);
    QAction* action = menu->addActionToQMenuAndActionHash(parentItem, name);
    if (!groupName.isEmpty()) {
        QActionGroup* group{ nullptr };
        if (!_exclusiveGroups.count(groupName)) {
            group = _exclusiveGroups[groupName] = new QActionGroup(menu);
            group->setExclusive(true);
        } else {
            group = _exclusiveGroups[groupName];
        }
        group->addAction(action);
    }
    connect(action, &QAction::triggered, [=] {
        onClicked(action->isChecked());
    });
    action->setCheckable(checkable);
    action->setChecked(checked);
    if (_activatingDisplayPlugin) {
        _currentDisplayPluginActions.push_back({ path, name });
    } else {
        _currentInputPluginActions.push_back({ path, name });
    }
    return action;
}

void PluginContainerProxy::removeMenuItem(const QString& menuName, const QString& menuItem) {
    Menu::getInstance()->removeMenuItem(menuName, menuItem);
}

bool PluginContainerProxy::isOptionChecked(const QString& name) {
    return Menu::getInstance()->isOptionChecked(name);
}

void PluginContainerProxy::setIsOptionChecked(const QString& path, bool checked) {
    Menu::getInstance()->setIsOptionChecked(path, checked);
}

// FIXME there is a bug in the fullscreen setting, where leaving
// fullscreen does not restore the window frame, making it difficult
// or impossible to move or size the window.
// Additionally, setting fullscreen isn't hiding the menu on windows
// make it useless for stereoscopic modes.
void PluginContainerProxy::setFullscreen(const QScreen* target, bool hideMenu) {
    auto _window = qApp->_window;
    if (!_window->isFullScreen()) {
        _savedGeometry = _window->geometry();
    }
    if (nullptr == target) {
        // FIXME target the screen where the window currently is
        target = qApp->primaryScreen();
    }

    _window->setGeometry(target->availableGeometry());
    _window->windowHandle()->setScreen((QScreen*)target);
    _window->showFullScreen();

#ifndef Q_OS_MAC
    // also hide the QMainWindow's menuBar
    QMenuBar* menuBar = _window->menuBar();
    if (menuBar && hideMenu) {
        menuBar->setVisible(false);
    }
#endif
}

void PluginContainerProxy::unsetFullscreen(const QScreen* avoid) {
    auto _window = qApp->_window;
    _window->showNormal();

    QRect targetGeometry = _savedGeometry;
    if (avoid != nullptr) {
        QRect avoidGeometry = avoid->geometry();
        if (avoidGeometry.contains(targetGeometry.topLeft())) {
            QScreen* newTarget = qApp->primaryScreen();
            if (newTarget == avoid) {
                foreach(auto screen, qApp->screens()) {
                    if (screen != avoid) {
                        newTarget = screen;
                        break;
                    }
                }
            }
            targetGeometry = newTarget->availableGeometry();
        }
    }
#ifdef Q_OS_MAC
    QTimer* timer = new QTimer();
    timer->singleShot(2000, [=] {
        _window->setGeometry(targetGeometry);
        timer->deleteLater();
    });
#else
    _window->setGeometry(targetGeometry);
#endif

#ifndef Q_OS_MAC
    // also show the QMainWindow's menuBar
    QMenuBar* menuBar = _window->menuBar();
    if (menuBar) {
        menuBar->setVisible(true);
    }
#endif
}

void PluginContainerProxy::requestReset() {
    // We could signal qApp to sequence this, but it turns out that requestReset is only used from within the main thread anyway.
    qApp->resetSensors();
}

void PluginContainerProxy::showDisplayPluginsTools() {
    DependencyManager::get<DialogsManager>()->hmdTools(true);
}

QGLWidget* PluginContainerProxy::getPrimarySurface() {
    return qApp->_glWidget;
}
