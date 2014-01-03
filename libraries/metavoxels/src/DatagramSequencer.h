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
    
    DatagramSequencer(const QByteArray& datagramHeader = QByteArray());
    
    /// Returns the packet number of the last packet sent.
    int getOutgoingPacketNumber() const { return _outgoingPacketNumber; }
    
    /// Returns the packet number of the last packet received (or the packet currently being assembled).
    int getIncomingPacketNumber() const { return _incomingPacketNumber; }
    
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
    
    /// Emitted when a sent packet has been acknowledged by the remote side.
    /// \param index the index of the packet in our list of send records
    void sendAcknowledged(int index);
    
    /// Emitted when our acknowledgement of a received packet has been acknowledged by the remote side.
    /// \param index the index of the packet in our list of receive records
    void receiveAcknowledged(int index);
    
private:
    
    class SendRecord {
    public:
        int packetNumber;
        int lastReceivedPacketNumber;
        Bitstream::WriteMappings mappings;
    };
    
    class ReceiveRecord {
    public:
        int packetNumber;
        Bitstream::ReadMappings mappings;
    
        bool operator<(const ReceiveRecord& other) const { return packetNumber < other.packetNumber; }
    };
    
    /// Notes that the described send was acknowledged by the other party.
    void sendRecordAcknowledged(const SendRecord& record);
    
    /// Sends a packet to the other party, fragmenting it into multiple datagrams (and emitting
    /// readyToWrite) as necessary.
    void sendPacket(const QByteArray& packet);
    
    QList<SendRecord> _sendRecords;
    QList<ReceiveRecord> _receiveRecords;
    
    QByteArray _outgoingPacketData;
    QDataStream _outgoingPacketStream;
    Bitstream _outputStream;
    
    QBuffer _incomingDatagramBuffer;
    QDataStream _incomingDatagramStream;
    int _datagramHeaderSize;
    
    int _outgoingPacketNumber;
    QByteArray _outgoingDatagram;
    QBuffer _outgoingDatagramBuffer;
    QDataStream _outgoingDatagramStream;
    
    int _incomingPacketNumber;
    QByteArray _incomingPacketData;
    QDataStream _incomingPacketStream;
    Bitstream _inputStream;
    QSet<int> _offsetsReceived;
    int _remainingBytes;
};

#endif /* defined(__interface__DatagramSequencer__) */
