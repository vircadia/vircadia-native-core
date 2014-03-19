//
//  ThreadedAssignment.h
//  hifi
//
//  Created by Stephen Birarda on 12/3/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ThreadedAssignment__
#define __hifi__ThreadedAssignment__

#include "Assignment.h"

class ThreadedAssignment : public Assignment {
    Q_OBJECT
public:
    ThreadedAssignment(const QByteArray& packet);
    void setFinished(bool isFinished);
    virtual void aboutToFinish() { };

public slots:
    /// threaded run of assignment
    virtual void run() = 0;
    virtual void deleteLater();
    virtual void readPendingDatagrams() = 0;

protected:
    bool readAvailableDatagram(QByteArray& destinationByteArray, HifiSockAddr& senderSockAddr);
    void commonInit(const QString& targetName, NodeType_t nodeType);
    bool _isFinished;
private slots:
    void checkInWithDomainServerOrExit();
signals:
    void finished();
};


#endif /* defined(__hifi__ThreadedAssignment__) */
