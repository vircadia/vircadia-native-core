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
#include <iViewHMDAPI.h>


class EyeTracker : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    ~EyeTracker();
    
    void processData(smi_CallbackDataStruct* data);

public slots:
    void init();
    void setEnabled(bool enabled);
    void reset();

private:
    bool _isInitialized = false;
    bool _isStreaming = false;
    bool _isEnabled = false;
};

#endif // hifi_EyeTracker_h
