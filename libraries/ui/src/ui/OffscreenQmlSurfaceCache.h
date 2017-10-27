//
//  Created by Bradley Austin Davis on 2017-01-11
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_OffscreenQmlSurfaceCache_h
#define hifi_OffscreenQmlSurfaceCahce_h

#include "DependencyManager.h"

#include <QtCore/QSharedPointer>

class OffscreenQmlSurface;

class OffscreenQmlSurfaceCache : public Dependency {
    SINGLETON_DEPENDENCY

public:
    OffscreenQmlSurfaceCache();
    virtual ~OffscreenQmlSurfaceCache();

    QSharedPointer<OffscreenQmlSurface> acquire(const QString& rootSource);
    void release(const QString& rootSource, const QSharedPointer<OffscreenQmlSurface>& surface);
    void reserve(const QString& rootSource, int count = 1);

private:
    QSharedPointer<OffscreenQmlSurface> buildSurface(const QString& rootSource);
    QHash<QString, QList<QSharedPointer<OffscreenQmlSurface>>> _cache;
};

#endif
