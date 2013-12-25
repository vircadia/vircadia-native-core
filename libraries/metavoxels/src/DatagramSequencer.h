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
#include <QList>
#include <QSet>

#include "Bitstream.h"

/// Performs simple datagram sequencing, packet fragmentation and reassembly.
class DatagramSequencer : public QObject {
    Q_OBJECT

public:
    
    DatagramSequencer();
    
    /// Starts a new packet for transmission.
    /// \return a reference to the Bitstream to use for writing to the packet
    Bitstream& startPacket();
    
    /// Sends the packet currently being written. 
    void endPacket();
    
    /// Processes a datagram received from the other party, emitting readyToRead when the entire packet
    /// has been successfully assembled.
    void receivedDatagram(const QByteArray& datagram);

signals:
    
    /// Emitted when a datagram is ready to be transmitted.
    void readyToWrite(const QByteArray& datagram);    
    
    /// Emitted when a packet is available to read.
    void readyToRead(Bitstream& input);
    
private:
    
    class SendRecord {
    public:
        int packetNumber;
        int lastReceivedPacketNumber;
        Bitstream::WriteMappings mappings;
    };
    
    /// Notes that the described send was acknowledged by the other party.
    void sendRecordAcknowledged(const SendRecord& record);
    
    /// Sends a packet to the other party, fragmenting it into multiple datagrams (and emitting
    /// readyToWrite) as necessary.
    void sendPacket(const QByteArray& packet);
    
    /// Handles the acknowledgement of a sent packet.
    void packetAcknowledged(int packetNumber);
    
    QList<SendRecord> _sendRecords;
    
    QByteArray _outgoingPacketData;
    QDataStream _outgoingPacketStream;
    Bitstream _outputStream;
    
    QBuffer _datagramBuffer;
    QDataStream _datagramStream;
    
    int _outgoingPacketNumber;
    QByteArray _outgoingDatagram;
    
    int _incomingPacketNumber;
    QByteArray _incomingPacketData;
    QDataStream _incomingPacketStream;
    Bitstream _inputStream;
    QSet<int> _offsetsReceived;
    int _remainingBytes;
    
    QList<int> _receivedPacketNumbers;
};

#endif /* defined(__interface__DatagramSequencer__) */
