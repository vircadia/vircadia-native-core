//
//  OctreeServerDatagramProcessor.h
//  assignment-client/src
//
//  Created by Brad Hefta-Gaub on 2014-09-05
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeServerDatagramProcessor_h
#define hifi_OctreeServerDatagramProcessor_h

#include <qobject.h>
#include <qudpsocket.h>

class OctreeServerDatagramProcessor : public QObject {
    Q_OBJECT
public:
    OctreeServerDatagramProcessor(QUdpSocket& nodeSocket, QThread* previousNodeSocketThread);
    ~OctreeServerDatagramProcessor();
public slots:
    void readPendingDatagrams();
signals:
    void packetRequiresProcessing(const QByteArray& receivedPacket, const HifiSockAddr& senderSockAddr);
private:
    QUdpSocket& _nodeSocket;
    QThread* _previousNodeSocketThread;
};

#endif // hifi_OctreeServerDatagramProcessor_h