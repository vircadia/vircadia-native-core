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
#include <QtCore/QSharedPointer>

#include <NodeList.h>

/*@jsdoc
 * The <code>EntityScriptServerLog</code> API makes server log file output written by server entity scripts available to client 
 * scripts.
 *
 * @namespace EntityScriptServerLog
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */
class EntityScriptServerLogClient : public QObject, public Dependency {
    Q_OBJECT

public:
    EntityScriptServerLogClient();

signals:

    /*@jsdoc
     * Triggered when one or more lines are written to the server log by server entity scripts.
     * @function EntityScriptServerLog.receivedNewLogLines
     * @param {string} logLines - The server log lines written by server entity scripts. If there are multiple lines they are 
     *     separated by <code>"\n"</code>s.
     * @example <caption>Echo server entity script program log output to Interface's program log.</caption>
     * EntityScriptServerLog.receivedNewLogLines.connect(function (logLines) {
     *     print("Log lines from server entity scripts:", logLines);
     * });
     * @example <caption>A server entity script to test with. Copy the code into an entity's "Server Script" property.</caption>
     * (function () {
     *     print("Hello from a server entity script!");
     * })
     */
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
