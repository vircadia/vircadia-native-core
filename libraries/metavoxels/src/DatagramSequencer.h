//
//  DatagramSequencer.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__DatagramSequencer__
#define __interface__DatagramSequencer__

#include <QBuffer>
#include <QDataStream>
#include <QByteArray>
#include <QSet>

/// Performs simple datagram sequencing, packet fragmentation and reassembly.
class DatagramSequencer : public QObject {
    Q_OBJECT

public:
    
    DatagramSequencer();
    
    void sendPacket(const QByteArray& packet);
    
    void receivedDatagram(const QByteArray& datagram);

signals:
    
    /// Emitted when a datagram is ready to be transmitted.
    void readyToWrite(const QByteArray& datagram);    
    
    /// Emitted when a packet is available to read.
    void readyToRead(const QByteArray& packet);
    
private:
    
    QBuffer _datagramBuffer;
    QDataStream _datagramStream;
    
    int _outgoingPacketNumber;
    QByteArray _outgoingDatagram;
    
    int _incomingPacketNumber;
    QByteArray _incomingPacketData;
    QSet<int> _offsetsReceived;
    int _remainingBytes;
};

#endif /* defined(__interface__DatagramSequencer__) */
