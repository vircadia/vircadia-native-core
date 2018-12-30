//
//  Created by Nissim Hadar on 2018/12/28
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PlatformInfoScriptingInterface_h
#define hifi_PlatformInfoScriptingInterface_h

#include <QtCore/QObject>

class QScriptValue;

class PlatformInfoScriptingInterface : public QObject {
    Q_OBJECT

public slots:
    static PlatformInfoScriptingInterface* getInstance();

    /**jsdoc
    * Returns the Operating Sytem type
    * @function Test.getOperatingSystemType
    * @returns {string} "WINDOWS", "MACOS" or "UNKNOWN"
    */
    QString getOperatingSystemType();

    /**jsdoc
    * Returns the CPU brand
    *function PlatformInfo.getCPUBrand()
    * @returns {string} brand of CPU
    */
    QString getCPUBrand();

    /**jsdoc
    * Returns the number of logical CPU cores
    *function PlatformInfo.getNumLogicalCores()
    * @returns {int} number of logical CPU cores
    */
    unsigned int getNumLogicalCores();

    /**jsdoc
    * Returns the total system memory in megabyte
    *function PlatformInfo.getTotalSystemMemory()
    * @returns {int} size of memory in megabytes
    */
    int getTotalSystemMemoryMB();
};

#endif  // hifi_PlatformInfoScriptingInterface_h
