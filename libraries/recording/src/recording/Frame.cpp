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



FrameType Frame::registerFrameType(const QString& frameTypeName) {
    Locker lock(mutex);
    std::call_once(once, [&] {
        auto headerType = frameTypes.registerValue("com.highfidelity.recording.Header");
        Q_ASSERT(headerType == Frame::TYPE_HEADER);
    });
    return frameTypes.registerValue(frameTypeName);
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
