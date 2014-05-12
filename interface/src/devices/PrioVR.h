//
//  PrioVR.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 5/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PrioVR_h
#define hifi_PrioVR_h

#include <QObject>

#ifdef HAVE_PRIOVR
#include <yei_threespace_api.h>
#endif

/// Handles interaction with the PrioVR skeleton tracking suit.
class PrioVR : public QObject {
    Q_OBJECT

public:
    
    PrioVR();
    virtual ~PrioVR();
    
    void update();

private:
#ifdef HAVE_PRIOVR
    TSS_Device_Id _baseStation;
    
    const int MAX_SENSOR_NODES = 20;
    TSS_Device_Id _sensorNodes[MAX_SENSOR_NODES];
#endif
};

#endif // hifi_PrioVR_h
