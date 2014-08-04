//
//  DdeFaceTracker.h
//
//
//  Created by Clement on 8/2/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DdeFaceTracker_h
#define hifi_DdeFaceTracker_h

#include <QUdpSocket>

#include "FaceTracker.h"

class DdeFaceTracker : public FaceTracker {
    Q_OBJECT
    
public:
    DdeFaceTracker();
    DdeFaceTracker(const QHostAddress& host, quint16 port);
    ~DdeFaceTracker();
    
    //initialization
    void init();
    void reset();
    void update();
    
    //sockets
    void bindTo(quint16 port);
    void bindTo(const QHostAddress& host, quint16 port);
    bool isActive() const;
    
private slots:
    
    //sockets
    void socketErrorOccurred(QAbstractSocket::SocketError socketError);
    void readPendingDatagrams();
    void socketStateChanged(QAbstractSocket::SocketState socketState);
    
private:
    void decodePacket(const QByteArray& buffer);
    
    // sockets
    QUdpSocket _udpSocket;
    quint64 _lastReceiveTimestamp;
    
    bool _reset;
    glm::vec3 _referenceTranslation;
    glm::quat _referenceRotation;
};

#endif // hifi_DdeFaceTracker_h