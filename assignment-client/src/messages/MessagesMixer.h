//
//  MessagesMixer.h
//  assignment-client/src/messages
//
//  Created by Brad hefta-Gaub on 11/16/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  The avatar mixer receives head, hand and positional data from all connected
//  nodes, and broadcasts that data back to them, every BROADCAST_INTERVAL ms.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MessagesMixer_h
#define hifi_MessagesMixer_h

#include <QtCore/QSharedPointer>

#include <ThreadedAssignment.h>

/// Handles assignments of type MessagesMixer - distribution of avatar data to various clients
class MessagesMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    MessagesMixer(ReceivedMessage& message);

public slots:
    void run() override;
    void nodeKilled(SharedNodePointer killedNode);
    void sendStatsPacket() override;

private slots:
    void handleMessages(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleMessagesSubscribe(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleMessagesUnsubscribe(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void parseDomainServerSettings(const QJsonObject& domainSettings);
    void domainSettingsRequestComplete();

    void startMaxMessagesProcessor();
    void stopMaxMessagesProcessor();
    void processMaxMessagesContainer();

private:
    QHash<QString, QSet<QUuid>> _channelSubscribers;
    QHash<QUuid, int> _allSubscribers;

    const int DEFAULT_NODE_MESSAGES_PER_SECOND = 1000;
    int _maxMessagesPerSecond { 0 };

    QTimer* _maxMessagesTimer { nullptr };
};

#endif // hifi_MessagesMixer_h
