//
//  DatagramSequencer.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/20/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DatagramSequencer_h
#define hifi_DatagramSequencer_h

#include <QBuffer>
#include <QDataStream>
#include <QByteArray>
#include <QList>
#include <QSet>
#include <QVector>

#include "AttributeRegistry.h"

class ReliableChannel;

/// Performs datagram sequencing, packet fragmentation and reassembly.  Works with Bitstream to provide methods to send and
/// receive data over UDP with varying reliability and latency characteristics.  To use, create a DatagramSequencer with the
/// fixed-size header that will be included with all outgoing datagrams and expected in all incoming ones (the contents of the
/// header are not checked on receive, only skipped over, and may be modified by the party that actually send the
/// datagram--this means that the header may include dynamically generated data, as long as its size remains fixed).  Connect
/// the readyToWrite signal to a slot that will actually transmit the datagram to the remote party.  When a datagram is
/// received from that party, call receivedDatagram with its contents.
///
/// A "packet" represents a batch of data sent at one time (split into one or more datagrams sized below the MTU).  Packets are
/// received in full and in order or not at all (that is, a packet being assembled is dropped as soon as a fragment from the
/// next packet is received).  Packets can be any size, but the larger a packet is, the more likely it is to be dropped--so,
/// it's better to keep packet sizes close to the MTU.  To write a packet, call startPacket, write data to the returned
/// Bitstream, then call endPacket (which will result in one or more firings of readyToWrite).  Data written in this way is not
/// guaranteed to be received, but if it is received, it will arrive in order.  This is a good way to transmit delta state:
/// state that represents the change between the last acknowledged state and the current state (which, if not received, will
/// not be resent as-is; instead, it will be replaced by up-to-date new deltas).
///
/// There are two methods for sending reliable data.  The first, for small messages that require minimum-latency processing, is
/// the high priority messaging system.  When you call sendHighPriorityMessage, the message that you send will be included with
/// every outgoing packet until it is acknowledged.  When the receiving party first sees the message, it will fire a
/// receivedHighPriorityMessage signal.
///
/// The second method employs a set of independent reliable channels multiplexed onto the packet stream.  These channels are
/// created lazily through the getReliableOutputChannel/getReliableInputChannel functions.  Output channels contain buffers
/// to which one may write either arbitrary data (as a QIODevice) or messages (as QVariants), or switch between the two.
/// Each time a packet is sent, data pending for reliable output channels is added, in proportion to their relative priorities,
/// until the packet size limit set by setMaxPacketSize is reached.  On the receive side, the streams are reconstructed and
/// (again, depending on whether messages are enabled) either the QIODevice reports that data is available, or, when a complete
/// message is decoded, the receivedMessage signal is fired.
class DatagramSequencer : public QObject {
    Q_OBJECT

public:
    
    /// Contains the content of a high-priority message along with the number of the first packet in which it was sent.
    class HighPriorityMessage {
    public:
        QVariant data;
        int firstPacketNumber;
    };
    
    /// Creates a new datagram sequencer.
    /// \param datagramHeader the content of the header that will be prepended to each outgoing datagram and whose length
    /// will be skipped over in each incoming datagram
    DatagramSequencer(const QByteArray& datagramHeader = QByteArray(), QObject* parent = NULL);
    
    /// Returns a reference to the weak hash mapping remote ids to shared objects.
    const WeakSharedObjectHash& getWeakSharedObjectHash() const { return _inputStream.getWeakSharedObjectHash(); }
    
    /// Returns the packet number of the last packet sent.
    int getOutgoingPacketNumber() const { return _outgoingPacketNumber; }
    
    /// Returns the packet number of the last packet received (or the packet currently being assembled).
    int getIncomingPacketNumber() const { return _incomingPacketNumber; }
    
    /// Returns a reference to the stream used to read packets.
    Bitstream& getInputStream() { return _inputStream; }
    
    /// Returns a reference to the stream used to write packets.
    Bitstream& getOutputStream() { return _outputStream; }
    
    /// Returns a reference to the outgoing packet data.
    const QByteArray& getOutgoingPacketData() const { return _outgoingPacketData; }
    
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
    
    /// Returns a reference to the stored receive mappings at the specified index.
    const Bitstream::ReadMappings& getReadMappings(int index) const { return _receiveRecords.at(index).mappings; }
    
    /// Adds stats for all reliable channels to the referenced variables.
    void addReliableChannelStats(int& sendProgress, int& sendTotal, int& receiveProgress, int& receiveTotal) const;
    
    /// Notes that we're sending a group of packets.
    /// \param desiredPackets the number of packets we'd like to write in the group
    /// \return the number of packets to write in the group
    int notePacketGroup(int desiredPackets = 1);
    
    /// Starts a new packet for transmission.
    /// \return a reference to the Bitstream to use for writing to the packet
    Bitstream& startPacket();
    
    /// Sends the packet currently being written. 
    void endPacket();
    
    /// Cancels the packet currently being written.
    void cancelPacket();
    
    /// Processes a datagram received from the other party, emitting readyToRead when the entire packet
    /// has been successfully assembled.
    Q_INVOKABLE void receivedDatagram(const QByteArray& datagram);

signals:
    
    /// Emitted when a datagram is ready to be transmitted.
    void readyToWrite(const QByteArray& datagram);    
    
    /// Emitted when a packet is available to read.
    void readyToRead(Bitstream& input);
    
    /// Emitted when we've received a high-priority message.
    void receivedHighPriorityMessage(const QVariant& data);
    
    /// Emitted when we've recorded the transmission of a packet.
    void sendRecorded();
    
    /// Emitted when we've recorded the receipt of a packet (that is, at the end of packet processing).
    void receiveRecorded();
    
    /// Emitted when a sent packet has been acknowledged by the remote side.
    /// \param index the index of the packet in our list of send records
    void sendAcknowledged(int index);
    
    /// Emitted when our acknowledgement of a received packet has been acknowledged by the remote side.
    /// \param index the index of the packet in our list of receive records
    void receiveAcknowledged(int index);

private slots:

    void sendClearSharedObjectMessage(int id);
    void handleHighPriorityMessage(const QVariant& data);
    void clearReliableChannel(QObject* object);
    
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
    
    /// Notes that the described send was lost in transit.
    void sendRecordLost(const SendRecord& record);
    
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
    
    float _packetsPerGroup;
    float _packetsToWrite;
    float _slowStartThreshold;
    int _packetRateIncreasePacketNumber;
    int _packetRateDecreasePacketNumber;
    
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

    /// Returns the channel's index in the sequencer's channel map.
    int getIndex() const { return _index; }

    /// Checks whether this is an output channel. 
    bool isOutput() const { return _output; }

    /// Returns a reference to the buffer used to write/read data to/from this channel.
    CircularBuffer& getBuffer() { return _buffer; }
    
    /// Returns a reference to the data stream created on this channel's buffer.
    QDataStream& getDataStream() { return _dataStream; }
    
    /// Returns a reference to the bitstream created on this channel's data stream.
    Bitstream& getBitstream() { return _bitstream; }

    /// Sets the channel priority, which determines how much of this channel's data (in proportion to the other channels) to
    /// include in each outgoing packet.
    void setPriority(float priority) { _priority = priority; }
    float getPriority() const { return _priority; }

    /// Returns the number of bytes available to read from this channel.
    int getBytesAvailable() const;

    /// Returns the offset, which represents the total number of bytes acknowledged
    /// (on the write end) or received completely (on the read end).
    int getOffset() const { return _offset; }

    /// Returns the total number of bytes written to this channel.
    int getBytesWritten() const { return _offset + _buffer.pos(); }

    /// Sets whether we expect to write/read framed messages.
    void setMessagesEnabled(bool enabled) { _messagesEnabled = enabled; }
    bool getMessagesEnabled() const { return _messagesEnabled; }

    /// Starts a framed message on this channel.
    void startMessage();
    
    /// Ends a framed message on this channel.
    void endMessage();

    /// Sends a framed message on this channel (convenience function that calls startMessage,
    /// writes the message to the bitstream, then calls endMessage).
    void sendMessage(const QVariant& message);

    /// Determines the number of bytes uploaded towards the currently pending message.
    /// \return true if there is a message pending, in which case the sent and total arguments will be set
    bool getMessageSendProgress(int& sent, int& total) const;

    /// Determines the number of bytes downloaded towards the currently pending message.
    /// \return true if there is a message pending, in which case the received and total arguments will be set
    bool getMessageReceiveProgress(int& received, int& total) const;

signals:

    /// Fired when a framed message has been received on this channel.
    void receivedMessage(const QVariant& message, Bitstream& in);

private slots:

    void sendClearSharedObjectMessage(int id);
    void handleMessage(const QVariant& message, Bitstream& in);
    
private:
    
    friend class DatagramSequencer;
    
    ReliableChannel(DatagramSequencer* sequencer, int index, bool output);
    
    void writeData(QDataStream& out, int bytes, QVector<DatagramSequencer::ChannelSpan>& spans);
    void writeFullSpans(QDataStream& out, int bytes, int startingIndex, int position,
        QVector<DatagramSequencer::ChannelSpan>& spans);
    int writeSpan(QDataStream& out, int position, int length, QVector<DatagramSequencer::ChannelSpan>& spans);
    
    void spanAcknowledged(const DatagramSequencer::ChannelSpan& span);
    void spanLost(int packetNumber, int nextOutgoingPacketNumber);
    
    void readData(QDataStream& in);
    
    int _index;
    bool _output;
    CircularBuffer _buffer;
    CircularBuffer _assemblyBuffer;
    QDataStream _dataStream;
    Bitstream _bitstream;
    float _priority;
    
    int _offset;
    int _writePosition;
    int _writePositionResetPacketNumber;
    SpanList _acknowledged;
    bool _messagesEnabled;
    int _messageLengthPlaceholder; ///< the location in the buffer of the message length for the current message
    int _messageReceivedOffset; ///< when reached, indicates that the most recent sent message has been received
    int _messageSize; ///< the size of the most recent sent message; only valid when _messageReceivedOffset has been set
};

#endif // hifi_DatagramSequencer_h
