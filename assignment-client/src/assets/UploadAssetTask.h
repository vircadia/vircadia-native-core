//
//  UploadAssetTask.h
//  assignment-client/src/assets
//
//  Created by Stephen Birarda on 2015-08-28.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_UploadAssetTask_h
#define hifi_UploadAssetTask_h

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QSharedPointer>

class NLPacketList;
class Node;

class UploadAssetTask : public QRunnable {
public:
    UploadAssetTask(QSharedPointer<NLPacketList> packetList, QSharedPointer<Node> senderNode, const QDir& resourcesDir);
    
    void run();
    
private:
    QSharedPointer<NLPacketList> _packetList;
    QSharedPointer<Node> _senderNode;
    QDir _resourcesDir;
};

#endif // hifi_UploadAssetTask_h
