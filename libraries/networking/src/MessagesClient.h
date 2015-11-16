//
//  MessagesClient.h
//  libraries/networking/src
//
//  Created by Brad hefta-Gaub on 11/16/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_MessagesClient_h
#define hifi_MessagesClient_h

#include <QString>

#include <DependencyManager.h>

#include "LimitedNodeList.h"
#include "NLPacket.h"
#include "Node.h"

class MessagesClient : public QObject, public Dependency {
    Q_OBJECT
public:
    MessagesClient();
    
    Q_INVOKABLE void init();

    Q_INVOKABLE void sendMessage(const QString& channel, const QString& message);

private slots:
    void handleMessagesPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode);
    void handleNodeKilled(SharedNodePointer node);

private:
};

#endif
