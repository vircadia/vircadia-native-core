//
//  Created by Ryan Huffman on 2016-12-14
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#ifndef HIFI_PROFILE_
#define HIFI_PROFILE_

#include "Trace.h"
#include "SharedUtil.h"

Q_DECLARE_LOGGING_CATEGORY(trace_app)
Q_DECLARE_LOGGING_CATEGORY(trace_network)
Q_DECLARE_LOGGING_CATEGORY(trace_render)
Q_DECLARE_LOGGING_CATEGORY(trace_render_gpu)
Q_DECLARE_LOGGING_CATEGORY(trace_resource)
Q_DECLARE_LOGGING_CATEGORY(trace_resource_parse)
Q_DECLARE_LOGGING_CATEGORY(trace_resource_network)
Q_DECLARE_LOGGING_CATEGORY(trace_simulation)
Q_DECLARE_LOGGING_CATEGORY(trace_simulation_animation)
Q_DECLARE_LOGGING_CATEGORY(trace_simulation_physics)

class Duration {
public:
    Duration(const QLoggingCategory& category, const QString& name, uint32_t argbColor = 0xff0000ff, uint64_t payload = 0, const QVariantMap& args = QVariantMap());
    ~Duration();

    static uint64_t beginRange(const QLoggingCategory& category, const char* name, uint32_t argbColor);
    static void endRange(const QLoggingCategory& category, uint64_t rangeId);

private:
    QString _name;
    const QLoggingCategory& _category;
};

inline void asyncBegin(const QLoggingCategory& category, const QString& name, const QString& id, const QVariantMap& args = QVariantMap(), const QVariantMap& extra = QVariantMap()) {
    if (category.isDebugEnabled()) {
        tracing::traceEvent(category, name, tracing::AsyncNestableStart, id, args, extra);
    }
}


inline void asyncEnd(const QLoggingCategory& category, const QString& name, const QString& id, const QVariantMap& args = QVariantMap(), const QVariantMap& extra = QVariantMap()) {
    if (category.isDebugEnabled()) {
        tracing::traceEvent(category, name, tracing::AsyncNestableEnd, id, args, extra);
    }
}

inline void instant(const QLoggingCategory& category, const QString& name, const QString& scope = "t", const QVariantMap& args = QVariantMap(), QVariantMap extra = QVariantMap()) {
    if (category.isDebugEnabled()) {
        extra["s"] = scope;
        tracing::traceEvent(category, name, tracing::Instant, "", args, extra);
    }
}

inline void counter(const QLoggingCategory& category, const QString& name, const QVariantMap& args, const QVariantMap& extra = QVariantMap()) {
    if (category.isDebugEnabled()) {
        tracing::traceEvent(category, name, tracing::Counter, "", args, extra);
    }
}

#define PROFILE_RANGE(category, name) Duration profileRangeThis(trace_##category(), name);
#define PROFILE_RANGE_EX(category, name, argbColor, payload, ...) Duration profileRangeThis(trace_##category(), name, argbColor, (uint64_t)payload, ##__VA_ARGS__);
#define PROFILE_RANGE_BEGIN(category, rangeId, name, argbColor) rangeId = Duration::beginRange(trace_##category(), name, argbColor)
#define PROFILE_RANGE_END(category, rangeId) Duration::endRange(trace_##category(), rangeId)
#define PROFILE_ASYNC_BEGIN(category, name, id, ...) asyncBegin(trace_##category(), name, id, ##__VA_ARGS__);
#define PROFILE_ASYNC_END(category, name, id, ...) asyncEnd(trace_##category(), name, id, ##__VA_ARGS__);
#define PROFILE_COUNTER(category, name, ...) counter(trace_##category(), name, ##__VA_ARGS__);
#define PROFILE_INSTANT(category, name, ...) instant(trace_##category(), name, ##__VA_ARGS__);

#define SAMPLE_PROFILE_RANGE(chance, category, name, ...) if (randFloat() <= chance) { PROFILE_RANGE(category, name); }
#define SAMPLE_PROFILE_RANGE_EX(chance, category, name, ...) if (randFloat() <= chance) { PROFILE_RANGE_EX(category, name, argbColor, payload, ##__VA_ARGS__); }
#define SAMPLE_PROFILE_COUNTER(chance, category, name, ...) if (randFloat() <= chance) { PROFILE_COUNTER(category, name, ##__VA_ARGS__); }
#define SAMPLE_PROFILE_INSTANT(chance, category, name, ...) if (randFloat() <= chance) { PROFILE_INSTANT(category, name, ##__VA_ARGS__); }

#endif
