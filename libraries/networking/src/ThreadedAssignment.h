//
//  ThreadedAssignment.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 12/3/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ThreadedAssignment_h
#define hifi_ThreadedAssignment_h

#include <QtCore/QSharedPointer>

#include "ReceivedMessage.h"

#include "Assignment.h"

class ThreadedAssignment : public Assignment {
    Q_OBJECT
public:
    ThreadedAssignment(ReceivedMessage& message);
    ~ThreadedAssignment() { stop(); }

    void setFinished(bool isFinished);
    virtual void aboutToFinish() { };
    void addPacketStatsAndSendStatsPacket(QJsonObject statsObject);

public slots:
    // JSDoc: Overridden in Agent.h.
    /// threaded run of assignment
    virtual void run() = 0;

    /**jsdoc
     * @function Agent.stop
     * @deprecated This function is being removed from the API.
     */
    Q_INVOKABLE virtual void stop() { setFinished(true); }

    /**jsdoc
     * @function Agent.sendStatsPacket
     * @deprecated This function is being removed from the API.
     */
    virtual void sendStatsPacket();

    /**jsdoc
     * @function Agent.clearQueuedCheckIns
     * @deprecated This function is being removed from the API.
     */
    void clearQueuedCheckIns() { _numQueuedCheckIns = 0; }

signals:
    /**jsdoc
     * @function Agent.finished
     * @returns {Signal}
     * @deprecated This function is being removed from the API.
     */
    void finished();

protected:
    void commonInit(const QString& targetName, NodeType_t nodeType);

    bool _isFinished;
    QTimer _domainServerTimer;
    QTimer _statsTimer;
    int _numQueuedCheckIns { 0 };

protected slots:
    /**jsdoc
     * @function Agent.domainSettingsRequestFailed
     * @deprecated This function is being removed from the API.
     */
    void domainSettingsRequestFailed();

private slots:
    void checkInWithDomainServerOrExit();
};

typedef QSharedPointer<ThreadedAssignment> SharedAssignmentPointer;

#endif // hifi_ThreadedAssignment_h
