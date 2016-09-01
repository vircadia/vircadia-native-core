//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Frame_h
#define hifi_Recording_Frame_h

#include "Forward.h"

#include <functional>

#ifdef Q_OS_WIN
#include <stdint.h>
#else
#include <limits>
#endif

#include <QtCore/QObject>

namespace recording {

struct FrameHeader {
    using Time = uint32_t;

// until we use a version of visual studio that has constexpr support, we can't use numeric_limits at compile time
#ifdef Q_OS_WIN
    static const Time INVALID_TIME = UINT32_MAX;
#else
    static const Time INVALID_TIME = std::numeric_limits<uint32_t>::max();
#endif

    static const FrameType TYPE_INVALID = 0xFFFF;
    static const FrameType TYPE_HEADER = 0x0;

    static Time secondsToFrameTime(float seconds);
    static float frameTimeToSeconds(Time frameTime);

    static uint32_t frameTimeToMilliseconds(Time frameTime);

    static Time frameTimeFromEpoch(quint64 epoch);
    static quint64 epochForFrameTime(Time frameTime);

    FrameType type { TYPE_INVALID };
    Time timeOffset { 0 }; // milliseconds

    FrameHeader() {}
    FrameHeader(FrameType type, Time timeOffset)
        : type(type), timeOffset(timeOffset) { }
};

struct Frame : public FrameHeader {
public:
    using Pointer = std::shared_ptr<Frame>;
    using ConstPointer = std::shared_ptr<const Frame>;
    using Handler = std::function<void(Frame::ConstPointer frame)>;

    QByteArray data;

    Frame() {}
    Frame(FrameType type, float timeOffset, const QByteArray& data)
        : FrameHeader(type, timeOffset), data(data) { }

    static FrameType registerFrameType(const QString& frameTypeName);
    static Handler registerFrameHandler(FrameType type, Handler handler);
    static Handler registerFrameHandler(const QString& frameTypeName, Handler handler);
    static void clearFrameHandler(FrameType type);
    static void clearFrameHandler(const QString& frameTypeName);
    static QMap<QString, FrameType> getFrameTypes();
    static QMap<FrameType, QString> getFrameTypeNames();
    static void handleFrame(const ConstPointer& frame);
};

}

#endif
