//
//  SandboxUtils.h
//  libraries/networking/src
//
//  Created by Brad Hefta-Gaub on 2016-10-15.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SandboxUtils_h
#define hifi_SandboxUtils_h

#include <functional>
#include <QtCore/QObject>


const QString SANDBOX_STATUS_URL = "http://localhost:60332/status";

class SandboxUtils : public QObject {
    Q_OBJECT
public:
    /// determines if the local sandbox is likely running. It does not account for custom setups, and is only 
    /// intended to detect the standard local sandbox install.
    void ifLocalSandboxRunningElse(std::function<void()> localSandboxRunningDoThis,
                                   std::function<void()> localSandboxNotRunningDoThat);

    static void runLocalSandbox(QString contentPath, bool autoShutdown, QString runningMarkerName);
};

#endif // hifi_SandboxUtils_h
