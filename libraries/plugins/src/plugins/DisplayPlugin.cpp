#include "DisplayPlugin.h"

#include <NumericalConstants.h>

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
