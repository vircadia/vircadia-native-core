//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QtCore>
#include <gpu/Format.h>
#include <gpu/Stream.h>
#include "Geometry.h"

template <typename T>
using DebugEnums = QMap<T, QString>;

namespace graphics {
    extern DebugEnums<Mesh::Topology> TOPOLOGIES;
    inline QDebug operator<<(QDebug dbg, Mesh::Topology type) { return dbg << TOPOLOGIES.value(type);}
    inline const QString toString(Mesh::Topology v) { return TOPOLOGIES.value(v); }
}

namespace gpu {
    extern DebugEnums<Type> TYPES;
    extern DebugEnums<Dimension> DIMENSIONS;
    extern DebugEnums<Semantic> SEMANTICS;
    extern DebugEnums<Stream::InputSlot> SLOTS;
    inline QDebug operator<<(QDebug dbg, gpu::Type type) { return dbg << TYPES.value(type); }
    inline QDebug operator<<(QDebug dbg, gpu::Dimension type) { return dbg << DIMENSIONS.value(type); }
    inline QDebug operator<<(QDebug dbg, gpu::Semantic type) { return dbg << SEMANTICS.value(type); }
    inline QDebug operator<<(QDebug dbg, gpu::Stream::InputSlot type) { return dbg << SLOTS.value(type); }
    inline const QString toString(gpu::Type v) { return TYPES.value(v); }
    inline const QString toString(gpu::Dimension v) { return DIMENSIONS.value(v); }
    inline const QString toString(gpu::Semantic v) { return SEMANTICS.value(v); }
    inline const QString toString(gpu::Stream::InputSlot v) { return SLOTS.value(v); }
    inline const QString toString(gpu::Element v) {
        return QString("[Element semantic=%1 type=%1 dimension=%2]")
            .arg(toString(v.getSemantic()))
            .arg(toString(v.getType()))
            .arg(toString(v.getDimension()));
    }
}

Q_DECLARE_METATYPE(gpu::Type)
Q_DECLARE_METATYPE(gpu::Dimension)
Q_DECLARE_METATYPE(gpu::Semantic)
Q_DECLARE_METATYPE(graphics::Mesh::Topology)
