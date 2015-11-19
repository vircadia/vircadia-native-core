//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Frame.h"

#include <mutex>

#include <QtCore/QMap>

#include <NumericalConstants.h>
#include <SharedUtil.h>

using namespace recording;

// FIXME move to shared
template <typename Key, typename Value> 
class Registry {
public:
    using ForwardMap = QMap<Value, Key>;
    using BackMap = QMap<Key, Value>;
    static const Key INVALID_KEY = static_cast<Key>(-1);

    Key registerValue(const Value& value) {
        Locker lock(_mutex);
        Key result = INVALID_KEY;
        if (_forwardMap.contains(value)) {
            result = _forwardMap[value];
        } else {
            _forwardMap[value] = result = _nextKey++;
            _backMap[result] = value;
        }
        return result;
    }

    Key getKey(const Value& value) {
        Locker lock(_mutex);
        Key result = INVALID_KEY;
        if (_forwardMap.contains(value)) {
            result = _forwardMap[value];
        }
        return result;
    }

    ForwardMap getKeysByValue() {
        Locker lock(_mutex);
        ForwardMap result = _forwardMap;
        return result;
    }

    BackMap getValuesByKey() {
        Locker lock(_mutex);
        BackMap result = _backMap;
        return result;
    }

private:
    using Mutex = std::mutex;
    using Locker = std::unique_lock<Mutex>;

    Mutex _mutex;

    ForwardMap _forwardMap;
    BackMap _backMap;
    Key _nextKey { 0 };
};

static Registry<FrameType, QString> frameTypes;
static QMap<FrameType, Frame::Handler> handlerMap;
using Mutex = std::mutex;
using Locker = std::unique_lock<Mutex>;
static Mutex mutex;
static std::once_flag once;

float FrameHeader::frameTimeToSeconds(Frame::Time frameTime) {
    float result = frameTime;
    result /= MSECS_PER_SECOND;
    return result;
}

uint32_t FrameHeader::frameTimeToMilliseconds(Frame::Time frameTime) {
    return frameTime;
}

Frame::Time FrameHeader::frameTimeFromEpoch(quint64 epoch) {
    auto intervalMicros = (usecTimestampNow() - epoch);
    intervalMicros /= USECS_PER_MSEC;
    return (Frame::Time)(intervalMicros);
}

quint64 FrameHeader::epochForFrameTime(Time frameTime) {
    auto epoch = usecTimestampNow();
    epoch -= (frameTime * USECS_PER_MSEC);
    return epoch;
}

Frame::Time FrameHeader::secondsToFrameTime(float seconds) {
    return (Time)(seconds * MSECS_PER_SECOND);
}

FrameType Frame::registerFrameType(const QString& frameTypeName) {
    Locker lock(mutex);
    std::call_once(once, [&] {
        auto headerType = frameTypes.registerValue("com.highfidelity.recording.Header");
        Q_ASSERT(headerType == Frame::TYPE_HEADER);
        Q_UNUSED(headerType); // FIXME - build system on unix still not upgraded to Qt 5.5.1 so Q_ASSERT still produces warnings
    });
    auto result = frameTypes.registerValue(frameTypeName);
    return result;
}

QMap<QString, FrameType> Frame::getFrameTypes() {
    return frameTypes.getKeysByValue();
}

QMap<FrameType, QString> Frame::getFrameTypeNames() {
    return frameTypes.getValuesByKey();
}

Frame::Handler Frame::registerFrameHandler(FrameType type, Handler handler) {
    Locker lock(mutex);
    Handler result;
    if (handlerMap.contains(type)) {
        result = handlerMap[type];
    }
    handlerMap[type] = handler;
    return result;
}

Frame::Handler Frame::registerFrameHandler(const QString& frameTypeName, Handler handler) {
    auto frameType = registerFrameType(frameTypeName);
    return registerFrameHandler(frameType, handler);
}

void Frame::clearFrameHandler(FrameType type) {
    Locker lock(mutex);
    auto iterator = handlerMap.find(type);
    if (iterator != handlerMap.end()) {
        handlerMap.erase(iterator);
    }
}

void Frame::clearFrameHandler(const QString& frameTypeName) {
    auto frameType = registerFrameType(frameTypeName);
    clearFrameHandler(frameType); 
}


void Frame::handleFrame(const Frame::ConstPointer& frame) {
    Handler handler; 
    {
        Locker lock(mutex);
        auto iterator = handlerMap.find(frame->type);
        if (iterator == handlerMap.end()) {
            return;
        }
        handler = *iterator;
    }
    handler(frame);
}
