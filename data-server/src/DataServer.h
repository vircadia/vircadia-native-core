//
//  DataServer.h
//  hifi
//
//  Created by Stephen Birarda on 1/20/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DataServer__
#define __hifi__DataServer__

#include <QtCore/QCoreApplication>
#include <QtCore/QUuid>
#include <QtNetwork/QUdpSocket>

#include <hiredis.h>

class DataServer : public QCoreApplication {
    Q_OBJECT
public:
    DataServer(int argc, char* argv[]);
    ~DataServer();
private:
    QUdpSocket _socket;
    redisContext* _redis;
    QUuid _uuid;
private slots:
    void readPendingDatagrams();
};

#endif /* defined(__hifi__DataServer__) */
