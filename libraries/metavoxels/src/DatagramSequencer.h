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

class ReliableChannel;

/// Performs simple datagram sequencing, packet fragmentation and reassembly.
class DatagramSequencer : public QObject {
    Q_OBJECT

public:
    
    class HighPriorityMessage {
    public:
        QVariant data;
        int firstPacketNumber;
    };
    
    DatagramSequencer(const QByteArray& datagramHeader = QByteArray());
    
    /// Returns the packet number of the last packet sent.
    int getOutgoingPacketNumber() const { return _outgoingPacketNumber; }
    
    /// Returns the packet number of the last packet received (or the packet currently being assembled).
    int getIncomingPacketNumber() const { return _incomingPacketNumber; }
    
    /// Returns the packet number of the sent packet at the specified index.
    int getSentPacketNumber(int index) const { return _sendRecords.at(index).packetNumber; }
    
    /// Sends a normal-priority reliable message.
    void sendReliableMessage(const QVariant& data, int channel = 0);
    
    /// Adds a message to the high priority queue.  Will be sent with every outgoing packet until received.
    void sendHighPriorityMessage(const QVariant& data);
    
    /// Returns a reference to the list of high priority messages not yet acknowledged.
    const QList<HighPriorityMessage>& getHighPriorityMessages() const { return _highPriorityMessages; }
    
    /// Sets the maximum packet size.  This is a soft limit that determines how much
    /// reliable data we include with each transmission.
    void setMaxPacketSize(int maxPacketSize) { _maxPacketSize = maxPacketSize; }
    
    int getMaxPacketSize() const { return _maxPacketSize; }
    
    /// Returns the output channel at the specified index, creating it if necessary.
    ReliableChannel* getReliableOutputChannel(int index = 0);
    
    /// Returns the intput channel at the 
    ReliableChannel* getReliableInputChannel(int index = 0);
    
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
    
    /// Emitted when we've received a high-priority message
    void receivedHighPriorityMessage(const QVariant& data);
    
    /// Emitted when a sent packet has been acknowledged by the remote side.
    /// \param index the index of the packet in our list of send records
    void sendAcknowledged(int index);
    
    /// Emitted when our acknowledgement of a received packet has been acknowledged by the remote side.
    /// \param index the index of the packet in our list of receive records
    void receiveAcknowledged(int index);

private slots:

    void sendClearSharedObjectMessage(int id);

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
        int newHighPriorityMessages;
    
        bool operator<(const ReceiveRecord& other) const { return packetNumber < other.packetNumber; }
    };
    
    /// Notes that the described send was acknowledged by the other party.
    void sendRecordAcknowledged(const SendRecord& record);
    
    /// Appends some reliable data to the outgoing packet.
    void appendReliableData(int bytes);
    
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
    
    QList<HighPriorityMessage> _highPriorityMessages;
    int _receivedHighPriorityMessages;
    
    int _maxPacketSize;
    
    QHash<int, ReliableChannel*> _reliableOutputChannels;
    QHash<int, ReliableChannel*> _reliableInputChannels;
};

/// Represents a single reliable channel multiplexed onto the datagram sequence.
class ReliableChannel : public QObject {
    Q_OBJECT
    
public:

    QDataStream& getDataStream() { return _dataStream; }
    Bitstream& getBitstream() { return _bitstream; }

    void setPriority(float priority) { _priority = priority; }
    float getPriority() const { return _priority; }

    void sendMessage(const QVariant& message);

private slots:

    void sendClearSharedObjectMessage(int id);

private:
    
    friend class DatagramSequencer;
    
    ReliableChannel(DatagramSequencer* sequencer);
    
    void readData(QDataStream& in);
    
    QBuffer _buffer;
    QDataStream _dataStream;
    Bitstream _bitstream;
    float _priority;
};

#endif /* defined(__interface__DatagramSequencer__) */
