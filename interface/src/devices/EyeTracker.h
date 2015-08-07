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

#include <glm/glm.hpp>

#include <DependencyManager.h>
#ifdef HAVE_IVIEWHMD
#include <iViewHMDAPI.h>
#endif


class EyeTracker : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    ~EyeTracker();

    void init();
    void setEnabled(bool enabled, bool simulate);
    void reset();

    bool isInitialized() const { return _isInitialized; }
    bool isTracking() const { return _isEnabled; }
    bool isSimulating() const { return _isSimulating; }

    glm::vec3 getLookAtPosition() const { return _lookAtPosition; }  // From mid eye point in head frame.
    
#ifdef HAVE_IVIEWHMD
    void processData(smi_CallbackDataStruct* data);

    void calibrate(int points);
#endif

private:
    QString smiReturnValueToString(int value);
    
    bool _isInitialized = false;
    bool _isEnabled = false;
    bool _isSimulating = false;

    glm::vec3 _lookAtPosition;
};

#endif // hifi_EyeTracker_h
