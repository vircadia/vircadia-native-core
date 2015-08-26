//
//  AssetScriptingInterface.h
//
//  Created by Ryan Huffman on 2015/07/22
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssetScriptingInterface_h
#define hifi_AssetScriptingInterface_h

#include <QString>
#include <QScriptValue>

#include <DependencyManager.h>
#include <LimitedNodeList.h>
#include <NLPacket.h>

class AssetScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    AssetScriptingInterface();

public slots:
    QScriptValue getAsset(QString hash, QScriptValue callback);
    QScriptValue uploadAsset(QString data, QString extension, QScriptValue callback);
};

#endif
