//
//  RatesScriptingInterface.h 
//  interface/src/scripting
//
//  Created by Zach Pomerantz on 4/20/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef HIFI_RATES_SCRIPTING_INTERFACE_H
#define HIFI_RATES_SCRIPTING_INTERFACE_H

#include <display-plugins/DisplayPlugin.h>

class RatesScriptingInterface : public QObject {
    Q_OBJECT

    Q_PROPERTY(float render READ getRenderRate)
    Q_PROPERTY(float present READ getPresentRate)
    Q_PROPERTY(float newFrame READ getNewFrameRate)
    Q_PROPERTY(float dropped READ getDropRate)
    Q_PROPERTY(float simulation READ getSimulationRate)
    Q_PROPERTY(float avatar READ getAvatarRate)

public:
    RatesScriptingInterface(QObject* parent) : QObject(parent) {}
    float getRenderRate() { return qApp->getFps(); }
    float getPresentRate() { return qApp->getActiveDisplayPlugin()->presentRate(); }
    float getNewFrameRate() { return qApp->getActiveDisplayPlugin()->newFramePresentRate(); }
    float getDropRate() { return qApp->getActiveDisplayPlugin()->droppedFrameRate(); }
    float getSimulationRate() { return qApp->getAverageSimsPerSecond(); }
    float getAvatarRate() { return qApp->getAvatarSimrate(); }
};

#endif // HIFI_INTERFACE_RATES_SCRIPTING_INTERFACE_H
