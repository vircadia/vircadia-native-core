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
#include <QVector>

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
    
    DatagramSequencer(const QByteArray& datagramHeader = QByteArray(), QObject* parent = NULL);
    
    /// Returns the packet number of the last packet sent.
    int getOutgoingPacketNumber() const { return _outgoingPacketNumber; }
    
    /// Returns the packet number of the last packet received (or the packet currently being assembled).
    int getIncomingPacketNumber() const { return _incomingPacketNumber; }
    
    /// Returns the packet number of the sent packet at the specified index.
    int getSentPacketNumber(int index) const { return _sendRecords.at(index).packetNumber; }
    
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
    
    /// Returns the intput channel at the specified index, creating it if necessary.
    ReliableChannel* getReliableInputChannel(int index = 0);
    
    /// Starts a new packet for transmission.
    /// \return a reference to the Bitstream to use for writing to the packet
    Bitstream& startPacket();
    
    /// Sends the packet currently being written. 
    void endPacket();
    
    /// Processes a datagram received from the other party, emitting readyToRead when the entire packet
    /// has been successfully assembled.
    Q_INVOKABLE void receivedDatagram(const QByteArray& datagram);

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
    void handleHighPriorityMessage(const QVariant& data);
    
private:
    
    friend class ReliableChannel;
    
    class ChannelSpan {
    public:
        int channel;
        int offset;
        int length;
    };
    
    class SendRecord {
    public:
        int packetNumber;
        int lastReceivedPacketNumber;
        Bitstream::WriteMappings mappings;
        QVector<ChannelSpan> spans; 
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
    void appendReliableData(int bytes, QVector<ChannelSpan>& spans);
    
    /// Sends a packet to the other party, fragmenting it into multiple datagrams (and emitting
    /// readyToWrite) as necessary.
    void sendPacket(const QByteArray& packet, const QVector<ChannelSpan>& spans);
    
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

/// A circular buffer, where one may efficiently append data to the end or remove data from the beginning.
class CircularBuffer : public QIODevice {
public:

    CircularBuffer(QObject* parent = NULL);

    /// Appends data to the end of the buffer.
    void append(const QByteArray& data) { append(data.constData(), data.size()); }

    /// Appends data to the end of the buffer.
    void append(const char* data, int length);

    /// Removes data from the beginning of the buffer.
    void remove(int length);

    /// Reads part of the data from the buffer.
    QByteArray readBytes(int offset, int length) const;

    /// Reads part of the data from the buffer.
    void readBytes(int offset, int length, char* data) const;

    /// Writes to part of the data in the buffer.
    void writeBytes(int offset, int length, const char* data);

    /// Writes part of the buffer to the supplied stream.
    void writeToStream(int offset, int length, QDataStream& out) const;

    /// Reads part of the buffer from the supplied stream.
    void readFromStream(int offset, int length, QDataStream& in);

    /// Appends part of the buffer to the supplied other buffer.
    void appendToBuffer(int offset, int length, CircularBuffer& buffer) const;

    virtual bool atEnd() const;
    virtual qint64 bytesAvailable() const;
    virtual bool canReadLine() const;
    virtual bool open(OpenMode flags);
    virtual qint64 pos() const;
    virtual bool seek(qint64 pos);
    virtual qint64 size() const;
    
protected:

    virtual qint64 readData(char* data, qint64 length);
    virtual qint64 writeData(const char* data, qint64 length);

private:
    
    void resize(int size);
    
    QByteArray _data;
    int _position;
    int _size;
    int _offset;
};

/// A list of contiguous spans, alternating between set and unset.  Conceptually, the list is preceeded by a set
/// span of infinite length and followed by an unset span of infinite length.  Within those bounds, it alternates
/// between unset and set.
class SpanList {
public:

    class Span {
    public:
        int unset;
        int set;
    };
    
    SpanList();
    
    const QList<Span>& getSpans() const { return _spans; }
    
    /// Returns the total length set.
    int getTotalSet() const { return _totalSet; }

    /// Sets a region of the list.
    /// \return the advancement of the set length at the beginning of the list
    int set(int offset, int length);

private:
    
    /// Sets the spans starting at the specified iterator, consuming at least the given length.
    /// \return the actual amount set, which may be greater if we ran into an existing set span
    int setSpans(QList<Span>::iterator it, int length);
    
    QList<Span> _spans;
    int _totalSet;
};

/// Represents a single reliable channel multiplexed onto the datagram sequence.
class ReliableChannel : public QObject {
    Q_OBJECT
    
public:

    int getIndex() const { return _index; }

    CircularBuffer& getBuffer() { return _buffer; }
    QDataStream& getDataStream() { return _dataStream; }
    Bitstream& getBitstream() { return _bitstream; }

    void setPriority(float priority) { _priority = priority; }
    float getPriority() const { return _priority; }

    int getBytesAvailable() const;

    /// Sets whether we expect to write/read framed messages.
    void setMessagesEnabled(bool enabled) { _messagesEnabled = enabled; }
    bool getMessagesEnabled() const { return _messagesEnabled; }

    /// Sends a framed message on this channel.
    void sendMessage(const QVariant& message);

signals:

    void receivedMessage(const QVariant& message);

private slots:

    void sendClearSharedObjectMessage(int id);
    void handleMessage(const QVariant& message);
    
private:
    
    friend class DatagramSequencer;
    
    ReliableChannel(DatagramSequencer* sequencer, int index, bool output);
    
    void writeData(QDataStream& out, int bytes, QVector<DatagramSequencer::ChannelSpan>& spans);
    int getBytesToWrite(bool& first, int length) const;
    int writeSpan(QDataStream& out, bool& first, int position, int length, QVector<DatagramSequencer::ChannelSpan>& spans);
    
    void spanAcknowledged(const DatagramSequencer::ChannelSpan& span);
    
    void readData(QDataStream& in);
    
    int _index;
    CircularBuffer _buffer;
    CircularBuffer _assemblyBuffer;
    QDataStream _dataStream;
    Bitstream _bitstream;
    float _priority;
    
    int _offset;
    int _writePosition;
    SpanList _acknowledged;
    bool _messagesEnabled;
};

#endif /* defined(__interface__DatagramSequencer__) */
