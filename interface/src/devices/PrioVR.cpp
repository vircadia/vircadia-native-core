//
//  PrioVR.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 5/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PrioVR.h"

PrioVR::PrioVR() {
#ifdef HAVE_PRIOVR
    TSS_ComPort comPort;
    if (!tss_getComPorts(&comPort, 1, 0, PVR_FIND_BS)) {
        _baseStation = TSS_NO_DEVICE_ID;
        return;
    }
    _baseStation = tss_createTSDeviceStr(comPort.com_port, TSS_TIMESTAMP_SYSTEM);
    if (_baseStation == TSS_NO_DEVICE_ID) {
        return;
    }
    for (int i = 0; i < MAX_SENSOR_NODES; i++) {
        tss_getSensorFromDongle(_baseStation, i, &_sensorNodes[i]);
        if (_sensorNodes[i] == TSS_NO_DEVICE_ID) {
            continue;
        }
        int present;
        tss_isPresent(_sensorNodes[i], &present);
        if (!present) {
            _sensorNodes[i] = TSS_NO_DEVICE_ID;
        }
    }
    tss_startStreaming(_baseStation, NULL);
#endif
}

PrioVR::~PrioVR() {
#ifdef HAVE_PRIOVR
    if (_baseStation != TSS_NO_DEVICE_ID) {
        tss_stopStreaming(_baseStation, NULL);
    }
#endif
}

void PrioVR::update() {
#ifdef HAVE_PRIOVR
    for (int i = 0; i < MAX_SENSOR_NODES; i++) {
        if (_sensorNodes[i] == TSS_NO_DEVICE_ID) {
            continue;
        }
        glm::quat rotation;
        if (!tss_getLastStreamData(_sensorNodes[i], (char*)&rotation, sizeof(glm::quat), NULL)) {
            qDebug() << i << rotation.x << rotation.y << rotation.z << rotation.w;
        } 
    }
#endif
}
