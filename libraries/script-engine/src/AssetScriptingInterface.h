//
//  AssetScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-03-08.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_AssetScriptingInterface_h
#define hifi_AssetScriptingInterface_h

#include <QtCore/QObject>
#include <QtScript/QScriptValue>

#include <AssetClient.h>

class AssetScriptingInterface : public QObject {
    Q_OBJECT
public:
    AssetScriptingInterface(QScriptEngine* engine);

    Q_INVOKABLE void uploadData(QString data, QScriptValue callback);
    Q_INVOKABLE void downloadData(QString url, QScriptValue downloadComplete);
    Q_INVOKABLE void setMapping(QString path, QString hash, QScriptValue callback);

#if (PR_BUILD || DEV_BUILD)
    Q_INVOKABLE void sendFakedHandshake();
#endif

protected:
    QSet<AssetRequest*> _pendingRequests;
    QScriptEngine* _engine;
};

#endif // hifi_AssetScriptingInterface_h
