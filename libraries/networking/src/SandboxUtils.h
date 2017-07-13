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

#include <QtCore/QString>

class QNetworkReply;

namespace SandboxUtils {
    const QString SANDBOX_STATUS_URL = "http://localhost:60332/status";

    QNetworkReply* getStatus();
    bool readStatus(QByteArray statusData);
    void runLocalSandbox(QString contentPath, bool autoShutdown, bool noUpdater);
};

#endif // hifi_SandboxUtils_h
