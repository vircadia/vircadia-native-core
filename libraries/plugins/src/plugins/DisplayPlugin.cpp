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

    {
        QMutexLocker locker(&_presentMutex);
        _presentCondition.wakeAll();
    }

    emit presented(_presentedFrameIndex);
}

void DisplayPlugin::waitForPresent() {
    QMutexLocker locker(&_presentMutex);
    while (isActive()) {
        if (_presentCondition.wait(&_presentMutex, MSECS_PER_SECOND)) {
            break;
        }
    }
}