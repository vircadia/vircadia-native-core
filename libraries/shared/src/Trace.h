//
//  Created by Ryan Huffman on 2016-12-14
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Trace_h
#define hifi_Trace_h

#include <cstdint>
#include <mutex>

#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QLoggingCategory>

#include "DependencyManager.h"

namespace tracing {

bool enabled();

using TraceTimestamp = uint64_t;

enum EventType : char {
    DurationBegin = 'B',
    DurationEnd = 'E',

    Complete = 'X',
    Instant = 'i',
    Counter = 'C',

    AsyncNestableStart = 'b',
    AsyncNestableInstant = 'n',
    AsyncNestableEnd = 'e',

    FlowStart = 's',
    FlowStep = 't',
    FlowEnd = 'f',

    Sample = 'P',

    ObjectCreated = 'N',
    ObjectSnapshot = 'O',
    ObjectDestroyed = 'D',

    Metadata = 'M',

    MemoryDumpGlobal = 'V',
    MemoryDumpProcess = 'v',

    Mark = 'R',

    ClockSync = 'c',

    ContextEnter = '(',
    ContextLeave = ')'
};

struct TraceEvent {
    QString id;
    QString name;
    EventType type;
    qint64 timestamp;
    qint64 processID;
    qint64 threadID;
    const QLoggingCategory& category;
    QVariantMap args;
    QVariantMap extra;

    void writeJson(QTextStream& out) const;
};

class Tracer : public Dependency {
public:
    void traceEvent(const QLoggingCategory& category, 
        const QString& name, EventType type,
        const QString& id = "", 
        const QVariantMap& args = QVariantMap(), const QVariantMap& extra = QVariantMap());

    void startTracing();
    void stopTracing();
    void serialize(const QString& file);
    bool isEnabled() const { return _enabled; }

private:
    void traceEvent(const QLoggingCategory& category, 
        const QString& name, EventType type,
        qint64 timestamp, qint64 processID, qint64 threadID,
        const QString& id = "",
        const QVariantMap& args = QVariantMap(), const QVariantMap& extra = QVariantMap());

    bool _enabled { false };
    std::list<TraceEvent> _events;
    std::mutex _eventsMutex;
};

inline void traceEvent(const QLoggingCategory& category, const QString& name, EventType type, const QString& id = "", const QVariantMap& args = {}, const QVariantMap& extra = {}) {
    DependencyManager::get<Tracer>()->traceEvent(category, name, type, id, args, extra);
}

inline void traceEvent(const QLoggingCategory& category, const QString& name, EventType type, int id, const QVariantMap& args = {}, const QVariantMap& extra = {}) {
    traceEvent(category, name, type, QString::number(id), args, extra);
}

}

#endif // hifi_Trace_h
