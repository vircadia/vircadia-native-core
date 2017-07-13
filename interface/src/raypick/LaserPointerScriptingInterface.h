//
//  LaserPointerScriptingInterface.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LaserPointerScriptingInterface_h
#define hifi_LaserPointerScriptingInterface_h

#include <QtCore/QObject>

#include "LaserPointerManager.h"
#include "RegisteredMetaTypes.h"

class LaserPointerScriptingInterface : public QObject {
    Q_OBJECT

public:
    static LaserPointerScriptingInterface* getInstance();

public slots:
    Q_INVOKABLE unsigned int createLaserPointer(const QVariant& properties);
    Q_INVOKABLE void enableLaserPointer(unsigned int uid) { LaserPointerManager::getInstance().enableLaserPointer(uid); }
    Q_INVOKABLE void disableLaserPointer(unsigned int uid) { LaserPointerManager::getInstance().disableLaserPointer(uid); }
    Q_INVOKABLE void removeLaserPointer(unsigned int uid) { LaserPointerManager::getInstance().removeLaserPointer(uid); }
    Q_INVOKABLE RayPickResult getPrevRayPickResult(unsigned int uid) { return LaserPointerManager::getInstance().getPrevRayPickResult(uid); }

};

#endif hifi_LaserPointerScriptingInterface_h