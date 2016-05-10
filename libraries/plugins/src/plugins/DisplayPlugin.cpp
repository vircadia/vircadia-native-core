#include "DisplayPlugin.h"

#include <NumericalConstants.h>
#include <ui/Menu.h>

#include "PluginContainer.h"

bool DisplayPlugin::activate() {
    if (isHmd() && (getHmdScreen() >= 0)) {
        _container->showDisplayPluginsTools();
    }
    return Parent::activate();
}

void DisplayPlugin::deactivate() {
    _container->showDisplayPluginsTools(false);
    if (!_container->currentDisplayActions().isEmpty()) {
        auto menu = _container->getPrimaryMenu();
        foreach(auto itemInfo, _container->currentDisplayActions()) {
            menu->removeMenuItem(itemInfo.first, itemInfo.second);
        }
        _container->currentDisplayActions().clear();
    }
    Parent::deactivate();
}

int64_t DisplayPlugin::getPaintDelayUsecs() const {
    std::lock_guard<std::mutex> lock(_paintDelayMutex);
    return _paintDelayTimer.isValid() ? _paintDelayTimer.nsecsElapsed() / NSECS_PER_USEC : 0;
}

void DisplayPlugin::incrementPresentCount() {
#ifdef DEBUG_PAINT_DELAY
    // Avoid overhead if we are not debugging
    {
        std::lock_guard<std::mutex> lock(_paintDelayMutex);
        _paintDelayTimer.start();
    }
#endif

    ++_presentedFrameIndex;

    // Alert the app that it needs to paint a new presentation frame
    qApp->postEvent(qApp, new QEvent(static_cast<QEvent::Type>(Present)), Qt::HighEventPriority);
}
