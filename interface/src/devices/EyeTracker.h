//
//  EyeTracker.h
//  interface/src/devices
//
//  Created by David Rowe on 27 Jul 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EyeTracker_h
#define hifi_EyeTracker_h

#include <QObject>

#include <DependencyManager.h>

class EyeTracker : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public slots:
    void init();
    void setEnabled(bool enabled);
    void reset();
};

#endif // hifi_EyeTracker_h
