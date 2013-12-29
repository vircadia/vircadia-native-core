//
//  Session.h
//  hifi
//
//  Created by Andrzej Kapolka on 12/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Session__
#define __hifi__Session__

#include <DatagramSequencer.h>

class HifiSockAddr;
class MetavoxelServer;

/// Contains the state of a single client session.
class Session : public QObject {
    Q_OBJECT
    
public:
    
    Session(MetavoxelServer* server);

    void receivedData(const QByteArray& data, const HifiSockAddr& sender);

private slots:

    void sendData(const QByteArray& data);

    void readPacket(Bitstream& in);    
    
private:
    
    MetavoxelServer* _server;
    DatagramSequencer _sequencer;
};

#endif /* defined(__hifi__Session__) */
