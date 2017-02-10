//
//  EntityScriptServerLogClient.h
//  interface/src
//
//  Created by Clement Brisset on 2/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityScriptServerLogClient_h
#define hifi_EntityScriptServerLogClient_h

#include <QObject>

#include <NodeList.h>

class EntityScriptServerLogClient : public QObject, public Dependency {
    Q_OBJECT

public:
    EntityScriptServerLogClient();

signals:
    void receivedNewLogLines(QString logLines);

protected:
    void connectNotify(const QMetaMethod& signal) override;
    void disconnectNotify(const QMetaMethod& signal) override;

private slots:
    void enableToEntityServerScriptLog(bool enable);
    void handleEntityServerScriptLogPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);

    void nodeActivated(SharedNodePointer activatedNode);
    void nodeKilled(SharedNodePointer killedNode);
    void canRezChanged(bool canRez);

    void connectionsChanged();

private:
    bool _subscribed { false };
};

#endif // hifi_EntityScriptServerLogClient_h
