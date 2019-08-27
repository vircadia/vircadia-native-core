//
//  Created by Bradley Austin Davis on 2019/08/22
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_shared_PlatformHelper_h
#define hifi_shared_PlatformHelper_h

#include <atomic>
#include <QtCore/QtGlobal>
#include "../DependencyManager.h"

class PlatformHelper : public QObject, public Dependency {
    Q_OBJECT
public:
    PlatformHelper() {}
    virtual ~PlatformHelper() {}

    void onSleep();
    void onWake();

signals:
    void systemWillSleep();
    void systemWillWake();

public:
    // Run the per-platform code to instantiate a platform-dependent PlatformHelper dependency object
    static void setup();
    // Run the per-platform code to cleanly shutdown a platform-dependent PlatformHelper dependency object
    static void shutdown();
    // Fetch the platform specific instance of the helper
    static PlatformHelper* instance();
    
    std::atomic<bool> _awake{ true };
};



#endif
