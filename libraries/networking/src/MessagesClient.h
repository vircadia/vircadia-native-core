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
#include "ReceivedMessage.h"

class MessagesClient : public QObject, public Dependency {
    Q_OBJECT
public:
    MessagesClient();
    
    Q_INVOKABLE void init();

    Q_INVOKABLE void sendMessage(QString channel, QString message, bool localOnly = false);
    Q_INVOKABLE void sendLocalMessage(QString channel, QString message);
    Q_INVOKABLE void subscribe(QString channel);
    Q_INVOKABLE void unsubscribe(QString channel);

    static void decodeMessagesPacket(QSharedPointer<ReceivedMessage> receivedMessage, QString& channel, QString& message, QUuid& senderID);
    static std::unique_ptr<NLPacketList> encodeMessagesPacket(QString channel, QString message, QUuid senderID);


signals:
    void messageReceived(QString channel, QString message, QUuid senderUUID, bool localOnly);

private slots:
    void handleMessagesPacket(QSharedPointer<ReceivedMessage> receivedMessage, SharedNodePointer senderNode);
    void handleNodeActivated(SharedNodePointer node);

protected:
    QSet<QString> _subscribedChannels;
};

#endif
