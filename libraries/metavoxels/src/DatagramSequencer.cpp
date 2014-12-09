//
//  DatagramSequencer.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/20/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>

#include <QtDebug>

#include <LimitedNodeList.h>

#include "DatagramSequencer.h"
#include "MetavoxelMessages.h"

// in sequencer parlance, a "packet" may consist of multiple datagrams.  clarify when we refer to actual datagrams
const int MAX_DATAGRAM_SIZE = 1450;

const int DEFAULT_MAX_PACKET_SIZE = 3000;

// the default slow-start threshold, which will be lowered quickly when we first encounter packet loss
const float DEFAULT_SLOW_START_THRESHOLD = 1000.0f;

DatagramSequencer::DatagramSequencer(const QByteArray& datagramHeader, QObject* parent) :
    QObject(parent),
    _outgoingPacketStream(&_outgoingPacketData, QIODevice::WriteOnly),
    _outputStream(_outgoingPacketStream, Bitstream::NO_METADATA, Bitstream::NO_GENERICS, this),
    _incomingDatagramStream(&_incomingDatagramBuffer),
    _datagramHeaderSize(datagramHeader.size()),
    _outgoingPacketNumber(0),
    _outgoingDatagram(MAX_DATAGRAM_SIZE, 0),
    _outgoingDatagramBuffer(&_outgoingDatagram),
    _outgoingDatagramStream(&_outgoingDatagramBuffer),
    _incomingPacketNumber(0),
    _incomingPacketStream(&_incomingPacketData, QIODevice::ReadOnly),
    _inputStream(_incomingPacketStream, Bitstream::NO_METADATA, Bitstream::NO_GENERICS, this),
    _receivedHighPriorityMessages(0),
    _maxPacketSize(DEFAULT_MAX_PACKET_SIZE),
    _packetsPerGroup(1.0f),
    _packetsToWrite(0.0f),
    _slowStartThreshold(DEFAULT_SLOW_START_THRESHOLD),
    _packetRateIncreasePacketNumber(0),
    _packetRateDecreasePacketNumber(0) {

    _outgoingPacketStream.setByteOrder(QDataStream::LittleEndian);
    _incomingDatagramStream.setByteOrder(QDataStream::LittleEndian);
    _incomingPacketStream.setByteOrder(QDataStream::LittleEndian);
    _outgoingDatagramStream.setByteOrder(QDataStream::LittleEndian);

    connect(&_outputStream, SIGNAL(sharedObjectCleared(int)), SLOT(sendClearSharedObjectMessage(int)));
    connect(this, SIGNAL(receivedHighPriorityMessage(const QVariant&)), SLOT(handleHighPriorityMessage(const QVariant&)));

    memcpy(_outgoingDatagram.data(), datagramHeader.constData(), _datagramHeaderSize);
}

void DatagramSequencer::sendHighPriorityMessage(const QVariant& data) {
    HighPriorityMessage message = { data, _outgoingPacketNumber + 1 };
    _highPriorityMessages.append(message);
}

ReliableChannel* DatagramSequencer::getReliableOutputChannel(int index) {
    ReliableChannel*& channel = _reliableOutputChannels[index];
    if (!channel) {
        channel = new ReliableChannel(this, index, true);
    }
    return channel;
}
    
ReliableChannel* DatagramSequencer::getReliableInputChannel(int index) {
    ReliableChannel*& channel = _reliableInputChannels[index];
    if (!channel) {
        channel = new ReliableChannel(this, index, false);
    }
    return channel;
}

void DatagramSequencer::addReliableChannelStats(int& sendProgress, int& sendTotal,
        int& receiveProgress, int& receiveTotal) const {
    foreach (ReliableChannel* channel, _reliableOutputChannels) {
        int sent, total;
        if (channel->getMessageSendProgress(sent, total)) {
            sendProgress += sent;
            sendTotal += total;
        }
    }
    foreach (ReliableChannel* channel, _reliableInputChannels) {
        int received, total;
        if (channel->getMessageReceiveProgress(received, total)) {
            receiveProgress += received;
            receiveTotal += total;
        }
    }
}

int DatagramSequencer::notePacketGroup(int desiredPackets) {
    // figure out how much data we have enqueued and increase the number of packets desired
    int totalAvailable = 0;
    foreach (ReliableChannel* channel, _reliableOutputChannels) {
        totalAvailable += channel->getBytesAvailable();
    }
    desiredPackets += (totalAvailable / _maxPacketSize);

    // increment our packet counter and subtract/return the integer portion
    _packetsToWrite += _packetsPerGroup;
    int wholePackets = (int)_packetsToWrite;
    _packetsToWrite -= wholePackets;
    wholePackets = qMin(wholePackets, desiredPackets);
    
    // if we don't want to send any more, push out the rate increase number past the group
    if (desiredPackets <= _packetsPerGroup) {
        _packetRateIncreasePacketNumber = _outgoingPacketNumber + wholePackets + 1;
    }
    
    // likewise, if we're only sending one packet, don't let its loss cause rate decrease
    if (wholePackets == 1) {
        _packetRateDecreasePacketNumber = _outgoingPacketNumber + 2;
    }
    
    return wholePackets;
}

Bitstream& DatagramSequencer::startPacket() {
    // start with the list of acknowledgements
    _outgoingPacketStream << (quint32)_receiveRecords.size();
    foreach (const ReceiveRecord& record, _receiveRecords) {
        _outgoingPacketStream << (quint32)record.packetNumber;
    }
    
    // return the stream, allowing the caller to write the rest
    return _outputStream;
}

void DatagramSequencer::endPacket() {
    // write the high-priority messages
    _outputStream << _highPriorityMessages.size();
    foreach (const HighPriorityMessage& message, _highPriorityMessages) {
        _outputStream << message.data;
    }
    _outputStream.flush();
    
    // if we have space remaining, send some data from our reliable channels 
    int remaining = _maxPacketSize - _outgoingPacketStream.device()->pos();
    const int MINIMUM_RELIABLE_SIZE = sizeof(quint32) * 5; // count, channel number, segment count, offset, size
    QVector<ChannelSpan> spans;
    if (remaining > MINIMUM_RELIABLE_SIZE) {
        appendReliableData(remaining, spans);
    } else {
        _outgoingPacketStream << (quint32)0;
    }
    
    sendPacket(QByteArray::fromRawData(_outgoingPacketData.constData(), _outgoingPacketStream.device()->pos()), spans);
    _outgoingPacketStream.device()->seek(0);
}

void DatagramSequencer::cancelPacket() {
    _outputStream.reset();
    _outputStream.getAndResetWriteMappings();
    _outgoingPacketStream.device()->seek(0);
}

/// Simple RAII-style object to keep a device open when in scope.
class QIODeviceOpener {
public:
    
    QIODeviceOpener(QIODevice* device, QIODevice::OpenMode mode) : _device(device) { _device->open(mode); }
    ~QIODeviceOpener() { _device->close(); }

private:
    
    QIODevice* _device;
};

void DatagramSequencer::receivedDatagram(const QByteArray& datagram) {
    _incomingDatagramBuffer.setData(datagram.constData() + _datagramHeaderSize, datagram.size() - _datagramHeaderSize);
    QIODeviceOpener opener(&_incomingDatagramBuffer, QIODevice::ReadOnly);
    
    // read the sequence number
    int sequenceNumber;
    _incomingDatagramStream >> sequenceNumber;
    
    // if it's less than the last, ignore
    if (sequenceNumber < _incomingPacketNumber) {
        return;
    }
    
    // read the size and offset
    quint32 packetSize, offset;
    _incomingDatagramStream >> packetSize >> offset;
    
    // if it's greater, reset
    if (sequenceNumber > _incomingPacketNumber) {
        _incomingPacketNumber = sequenceNumber;
        _incomingPacketData.resize(packetSize);
        _offsetsReceived.clear();
        _offsetsReceived.insert(offset);
        _remainingBytes = packetSize;
         
    } else {
        // make sure it's not a duplicate
        if (_offsetsReceived.contains(offset)) {
            return;
        }
        _offsetsReceived.insert(offset);
    }
    
    // copy in the data
    memcpy(_incomingPacketData.data() + offset, _incomingDatagramBuffer.data().constData() + _incomingDatagramBuffer.pos(),
        _incomingDatagramBuffer.bytesAvailable());
    
    // see if we're done
    if ((_remainingBytes -= _incomingDatagramBuffer.bytesAvailable()) > 0) {
        return;
    }
    
    // read the list of acknowledged packets
    quint32 acknowledgementCount;
    _incomingPacketStream >> acknowledgementCount;
    for (quint32 i = 0; i < acknowledgementCount; i++) {
        quint32 packetNumber;
        _incomingPacketStream >> packetNumber;
        if (_sendRecords.isEmpty()) {
            continue;
        }
        int index = packetNumber - _sendRecords.first().packetNumber;
        if (index < 0 || index >= _sendRecords.size()) {
            continue;
        }
        QList<SendRecord>::iterator it = _sendRecords.begin();
        for (int i = 0; i < index; i++) {
            sendRecordLost(*it++);
        }
        sendRecordAcknowledged(*it);
        emit sendAcknowledged(index);
        _sendRecords.erase(_sendRecords.begin(), it + 1);
    }
    
    try {
        // alert external parties so that they can read the middle
        emit readyToRead(_inputStream);
        
        // read and dispatch the high-priority messages
        int highPriorityMessageCount;
        _inputStream >> highPriorityMessageCount;
        int newHighPriorityMessages = highPriorityMessageCount - _receivedHighPriorityMessages;
        for (int i = 0; i < highPriorityMessageCount; i++) {
            QVariant data;
            _inputStream >> data;
            if (i >= _receivedHighPriorityMessages) {
                emit receivedHighPriorityMessage(data);
            }
        }
        _receivedHighPriorityMessages = highPriorityMessageCount;
        
        // read the reliable data, if any
        quint32 reliableChannels;
        _incomingPacketStream >> reliableChannels;
        for (quint32 i = 0; i < reliableChannels; i++) {
            quint32 channelIndex;
            _incomingPacketStream >> channelIndex;
            getReliableInputChannel(channelIndex)->readData(_incomingPacketStream);
        }
        
        // record the receipt
        ReceiveRecord record = { _incomingPacketNumber, _inputStream.getAndResetReadMappings(), newHighPriorityMessages };
        _receiveRecords.append(record);
        
        emit receiveRecorded();
    
    } catch (const BitstreamException& e) {
        qWarning() << "Error reading datagram:" << e.getDescription();
    }
    
    _incomingPacketStream.device()->seek(0);
    _inputStream.reset();    
}

void DatagramSequencer::sendClearSharedObjectMessage(int id) {
    // send it low-priority unless the channel has messages disabled
    ReliableChannel* channel = getReliableOutputChannel();
    if (channel->getMessagesEnabled()) {
        ClearMainChannelSharedObjectMessage message = { id };
        channel->sendMessage(QVariant::fromValue(message));
        
    } else {
        ClearSharedObjectMessage message = { id };
        sendHighPriorityMessage(QVariant::fromValue(message));
    }
}

void DatagramSequencer::handleHighPriorityMessage(const QVariant& data) {
    if (data.userType() == ClearSharedObjectMessage::Type) {
        _inputStream.clearSharedObject(data.value<ClearSharedObjectMessage>().id);
    }
}

void DatagramSequencer::clearReliableChannel(QObject* object) {
    ReliableChannel* channel = static_cast<ReliableChannel*>(object);
    (channel->isOutput() ? _reliableOutputChannels : _reliableInputChannels).remove(channel->getIndex());
}

void DatagramSequencer::sendRecordAcknowledged(const SendRecord& record) {
    // stop acknowledging the recorded packets
    while (!_receiveRecords.isEmpty() && _receiveRecords.first().packetNumber <= record.lastReceivedPacketNumber) {
        emit receiveAcknowledged(0);
        const ReceiveRecord& received = _receiveRecords.first();
        _inputStream.persistReadMappings(received.mappings);
        _receivedHighPriorityMessages -= received.newHighPriorityMessages;
        _receiveRecords.removeFirst();
    }
    _outputStream.persistWriteMappings(record.mappings);
    
    // remove the received high priority messages
    for (int i = _highPriorityMessages.size() - 1; i >= 0; i--) {
        if (_highPriorityMessages.at(i).firstPacketNumber <= record.packetNumber) {
            _highPriorityMessages.erase(_highPriorityMessages.begin(), _highPriorityMessages.begin() + i + 1);
            break;
        }
    }
    
    // acknowledge the received spans
    foreach (const ChannelSpan& span, record.spans) {
        ReliableChannel* channel = _reliableOutputChannels.value(span.channel);
        if (channel) {
            channel->spanAcknowledged(span);
        }
    }
    
    // increase the packet rate with every ack until we pass the slow start threshold; then, every round trip
    if (record.packetNumber >= _packetRateIncreasePacketNumber) {
        if (_packetsPerGroup >= _slowStartThreshold) {
            _packetRateIncreasePacketNumber = _outgoingPacketNumber + 1;
        }
        _packetsPerGroup += 1.0f;
    }
}

void DatagramSequencer::sendRecordLost(const SendRecord& record) {
    // notify the channels of their lost spans
    foreach (const ChannelSpan& span, record.spans) {
        ReliableChannel* channel = _reliableOutputChannels.value(span.channel);
        if (channel) {
            channel->spanLost(record.packetNumber, _outgoingPacketNumber + 1);
        }
    }
    
    // halve the rate and remember as threshold
    if (record.packetNumber >= _packetRateDecreasePacketNumber) {
        _packetsPerGroup = qMax(_packetsPerGroup * 0.5f, 1.0f);
        _slowStartThreshold = _packetsPerGroup;
        _packetRateDecreasePacketNumber = _outgoingPacketNumber + 1;
    }
}

void DatagramSequencer::appendReliableData(int bytes, QVector<ChannelSpan>& spans) {
    // gather total number of bytes to write, priority
    int totalBytes = 0;
    float totalPriority = 0.0f;
    int totalChannels = 0;
    foreach (ReliableChannel* channel, _reliableOutputChannels) {
        int channelBytes = channel->getBytesAvailable();
        if (channelBytes > 0) {
            totalBytes += channelBytes;
            totalPriority += channel->getPriority();
            totalChannels++;
        }
    }
    _outgoingPacketStream << (quint32)totalChannels;
    if (totalChannels == 0) {
        return;    
    }
    totalBytes = qMin(bytes, totalBytes);
    
    foreach (ReliableChannel* channel, _reliableOutputChannels) {
        int channelBytes = channel->getBytesAvailable();
        if (channelBytes == 0) {
            continue;
        }
        _outgoingPacketStream << (quint32)channel->getIndex();
        channelBytes = qMin(channelBytes, (int)(totalBytes * channel->getPriority() / totalPriority));
        channel->writeData(_outgoingPacketStream, channelBytes, spans);   
        totalBytes -= channelBytes;
        totalPriority -= channel->getPriority();
    }
}

void DatagramSequencer::sendPacket(const QByteArray& packet, const QVector<ChannelSpan>& spans) {
    QIODeviceOpener opener(&_outgoingDatagramBuffer, QIODevice::WriteOnly);
    
    // increment the packet number
    _outgoingPacketNumber++;
    
    // record the send
    SendRecord record = { _outgoingPacketNumber, _receiveRecords.isEmpty() ? 0 : _receiveRecords.last().packetNumber,
        _outputStream.getAndResetWriteMappings(), spans };
    _sendRecords.append(record);
    
    emit sendRecorded();
    
    // write the sequence number and size, which are the same between all fragments
    _outgoingDatagramBuffer.seek(_datagramHeaderSize);
    _outgoingDatagramStream << (quint32)_outgoingPacketNumber;
    _outgoingDatagramStream << (quint32)packet.size();
    int initialPosition = _outgoingDatagramBuffer.pos();
    
    // break the packet into MTU-sized datagrams
    int offset = 0;
    do {
        _outgoingDatagramBuffer.seek(initialPosition);
        _outgoingDatagramStream << (quint32)offset;
        
        int payloadSize = qMin((int)(_outgoingDatagram.size() - _outgoingDatagramBuffer.pos()), packet.size() - offset);
        memcpy(_outgoingDatagram.data() + _outgoingDatagramBuffer.pos(), packet.constData() + offset, payloadSize);
        
        emit readyToWrite(QByteArray::fromRawData(_outgoingDatagram.constData(), _outgoingDatagramBuffer.pos() + payloadSize));
        
        offset += payloadSize;
        
    } while(offset < packet.size());
}

const int INITIAL_CIRCULAR_BUFFER_CAPACITY = 16;

CircularBuffer::CircularBuffer(QObject* parent) :
    QIODevice(parent),
    _data(INITIAL_CIRCULAR_BUFFER_CAPACITY, 0),
    _position(0),
    _size(0),
    _offset(0) {
}

void CircularBuffer::append(const char* data, int length) {
    // resize to fit
    int oldSize = _size;
    resize(_size + length);
    
    // write our data in up to two segments: one from the position to the end, one from the beginning
    int end = (_position + oldSize) % _data.size();
    int firstSegment = qMin(length, _data.size() - end);
    memcpy(_data.data() + end, data, firstSegment);
    int secondSegment = length - firstSegment;
    if (secondSegment > 0) {
        memcpy(_data.data(), data + firstSegment, secondSegment);
    }
}

void CircularBuffer::remove(int length) {
    _position = (_position + length) % _data.size();
    _size -= length;
}

QByteArray CircularBuffer::readBytes(int offset, int length) const {
    QByteArray bytes(length, 0);
    readBytes(offset, length, bytes.data());
    return bytes;
}

void CircularBuffer::readBytes(int offset, int length, char* data) const {
    // read in up to two segments
    QByteArray array;
    int start = (_position + offset) % _data.size();
    int firstSegment = qMin(length, _data.size() - start);
    memcpy(data, _data.constData() + start, firstSegment);
    int secondSegment = length - firstSegment;
    if (secondSegment > 0) {
        memcpy(data + firstSegment, _data.constData(), secondSegment);
    }
}

void CircularBuffer::writeBytes(int offset, int length, const char* data) {
    // write in up to two segments
    int start = (_position + offset) % _data.size();
    int firstSegment = qMin(length, _data.size() - start);
    memcpy(_data.data() + start, data, firstSegment);
    int secondSegment = length - firstSegment;
    if (secondSegment > 0) {
        memcpy(_data.data(), data + firstSegment, secondSegment);
    }
}

void CircularBuffer::writeToStream(int offset, int length, QDataStream& out) const {
    // write in up to two segments
    int start = (_position + offset) % _data.size();
    int firstSegment = qMin(length, _data.size() - start);
    out.writeRawData(_data.constData() + start, firstSegment);
    int secondSegment = length - firstSegment;
    if (secondSegment > 0) {
        out.writeRawData(_data.constData(), secondSegment);
    }
}

void CircularBuffer::readFromStream(int offset, int length, QDataStream& in) {
    // resize to fit
    int requiredSize = offset + length;
    if (requiredSize > _size) {
        resize(requiredSize);
    }
    
    // read in up to two segments
    int start = (_position + offset) % _data.size();
    int firstSegment = qMin(length, _data.size() - start);
    in.readRawData(_data.data() + start, firstSegment);
    int secondSegment = length - firstSegment;
    if (secondSegment > 0) {
        in.readRawData(_data.data(), secondSegment);
    }
}

void CircularBuffer::appendToBuffer(int offset, int length, CircularBuffer& buffer) const {
    // append in up to two segments
    int start = (_position + offset) % _data.size();
    int firstSegment = qMin(length, _data.size() - start);
    buffer.append(_data.constData() + start, firstSegment);
    int secondSegment = length - firstSegment;
    if (secondSegment > 0) {
        buffer.append(_data.constData(), secondSegment);
    }
}

bool CircularBuffer::atEnd() const {
    return _offset >= _size;
}

qint64 CircularBuffer::bytesAvailable() const {
    return _size - _offset;
}

bool CircularBuffer::canReadLine() const {
    for (int offset = _offset; offset < _size; offset++) {
        if (_data.at((_position + offset) % _data.size()) == '\n') {
            return true;
        }
    }
    return false;
}

bool CircularBuffer::open(OpenMode flags) {
    return QIODevice::open(flags | QIODevice::Unbuffered);
}

qint64 CircularBuffer::pos() const {
    return _offset;
}

bool CircularBuffer::seek(qint64 pos) {
    if (pos < 0 || pos > _size) {
        return false;
    }
    _offset = pos;
    return true;
}

qint64 CircularBuffer::size() const {
    return _size;
}

qint64 CircularBuffer::readData(char* data, qint64 length) {
    int readable = qMin((int)length, _size - _offset);
    
    // read in up to two segments
    int start = (_position + _offset) % _data.size();
    int firstSegment = qMin((int)length, _data.size() - start);
    memcpy(data, _data.constData() + start, firstSegment);
    int secondSegment = length - firstSegment;
    if (secondSegment > 0) {
        memcpy(data + firstSegment, _data.constData(), secondSegment);
    }
    _offset += readable;
    return readable;
}

qint64 CircularBuffer::writeData(const char* data, qint64 length) {
    // resize to fit
    int requiredSize = _offset + length;
    if (requiredSize > _size) {
        resize(requiredSize);
    }
    
    // write in up to two segments
    int start = (_position + _offset) % _data.size();
    int firstSegment = qMin((int)length, _data.size() - start);
    memcpy(_data.data() + start, data, firstSegment);
    int secondSegment = length - firstSegment;
    if (secondSegment > 0) {
        memcpy(_data.data(), data + firstSegment, secondSegment);
    }
    _offset += length;
    return length;
}

void CircularBuffer::resize(int size) {
    if (size > _data.size()) {
        // double our capacity until we can fit the desired length
        int newCapacity = _data.size();
        do {
            newCapacity *= 2;
        } while (size > newCapacity);
        
        int oldCapacity = _data.size();
        _data.resize(newCapacity);
        
        int trailing = _position + _size - oldCapacity;
        if (trailing > 0) {
            memcpy(_data.data() + oldCapacity, _data.constData(), trailing);
        }
    }
    _size = size;
}

SpanList::SpanList() : _totalSet(0) {
}

int SpanList::set(int offset, int length) {
    // if we intersect the front of the list, consume beginning spans and return advancement
    if (offset <= 0) {
        int intersection = offset + length;
        return (intersection > 0) ? setSpans(_spans.begin(), intersection) : 0;
    }

    // look for an intersection within the list
    int position = 0;
    for (int i = 0; i < _spans.size(); i++) {
        QList<Span>::iterator it = _spans.begin() + i;
    
        // if we intersect the unset portion, contract it
        position += it->unset;
        if (offset <= position) {
            int remove = position - offset;
            it->unset -= remove;
            
            // if we continue into the set portion, expand it and consume following spans
            int extra = offset + length - position;
            if (extra >= 0) {
                extra -= it->set;
                it->set += remove;
                _totalSet += remove;
                if (extra > 0) {
                    int amount = setSpans(it + 1, extra);
                    _spans[i].set += amount;
                    _totalSet += amount;
                }
            // otherwise, insert a new span
            } else {        
                Span span = { it->unset, length };
                it->unset = -extra;
                _spans.insert(it, span);
                _totalSet += length;
            }
            return 0;
        }
        
        // if we intersect the set portion, expand it and consume following spans
        position += it->set;
        if (offset <= position) {
            int extra = offset + length - position;
            if (extra > 0) {
                int amount = setSpans(it + 1, extra);
                _spans[i].set += amount;
                _totalSet += amount;
            }
            return 0;
        }
    }

    // add to end of list
    Span span = { offset - position, length };
    _spans.append(span);
    _totalSet += length;

    return 0;
}

int SpanList::setSpans(QList<Span>::iterator it, int length) {
    int remainingLength = length;
    int totalRemoved = 0;
    for (; it != _spans.end(); it = _spans.erase(it)) {
        if (remainingLength < it->unset) {
            it->unset -= remainingLength;
            totalRemoved += remainingLength;
            break;
        }
        int combined = it->unset + it->set;
        remainingLength = qMax(remainingLength - combined, 0);
        totalRemoved += combined;
        _totalSet -= it->set;
    }
    return qMax(length, totalRemoved);
}

int ReliableChannel::getBytesAvailable() const {
    return _buffer.size() - _acknowledged.getTotalSet();
}

void ReliableChannel::startMessage() {
    // write a placeholder for the length; we'll fill it in when we know what it is
    _messageLengthPlaceholder = _buffer.pos();
    _dataStream << (quint32)0;
}

void ReliableChannel::endMessage() {
    _bitstream.flush();
    _bitstream.persistAndResetWriteMappings();
    
    quint32 length = _buffer.pos() - _messageLengthPlaceholder;
    _buffer.writeBytes(_messageLengthPlaceholder, sizeof(quint32), (const char*)&length);

    pruneOutgoingMessageStats();
    _outgoingMessageStats.append(OffsetSizePair(getBytesWritten(), length));
}

void ReliableChannel::sendMessage(const QVariant& message) {
    startMessage();
    _bitstream << message;
    endMessage();
}

bool ReliableChannel::getMessageSendProgress(int& sent, int& total) {
    pruneOutgoingMessageStats();
    if (!_messagesEnabled || _outgoingMessageStats.isEmpty()) {
        return false;
    }
    const OffsetSizePair& stat = _outgoingMessageStats.first();
    sent = qMax(0, stat.second - (stat.first - _offset));
    total = stat.second;
    return true;
}

bool ReliableChannel::getMessageReceiveProgress(int& received, int& total) const {
    if (!_messagesEnabled || _buffer.bytesAvailable() < (int)sizeof(quint32)) {
        return false;
    }
    quint32 length;
    _buffer.readBytes(_buffer.pos(), sizeof(quint32), (char*)&length);
    total = length;
    received = _buffer.bytesAvailable();
    return true;
}

void ReliableChannel::sendClearSharedObjectMessage(int id) {
    ClearSharedObjectMessage message = { id };
    sendMessage(QVariant::fromValue(message));
}

void ReliableChannel::handleMessage(const QVariant& message, Bitstream& in) {
    if (message.userType() == ClearSharedObjectMessage::Type) {
        _bitstream.clearSharedObject(message.value<ClearSharedObjectMessage>().id);
    
    } else if (message.userType() == ClearMainChannelSharedObjectMessage::Type) {
        static_cast<DatagramSequencer*>(parent())->_inputStream.clearSharedObject(
            message.value<ClearMainChannelSharedObjectMessage>().id);
    }
}

ReliableChannel::ReliableChannel(DatagramSequencer* sequencer, int index, bool output) :
    QObject(sequencer),
    _index(index),
    _output(output),
    _dataStream(&_buffer),
    _bitstream(_dataStream, Bitstream::NO_METADATA, Bitstream::NO_GENERICS, this),
    _priority(1.0f),
    _offset(0),
    _writePosition(0),
    _writePositionResetPacketNumber(0),
    _messagesEnabled(true) {
    
    _buffer.open(output ? QIODevice::WriteOnly : QIODevice::ReadOnly);
    _dataStream.setByteOrder(QDataStream::LittleEndian);
    
    connect(&_bitstream, SIGNAL(sharedObjectCleared(int)), SLOT(sendClearSharedObjectMessage(int)));
    connect(this, SIGNAL(receivedMessage(const QVariant&, Bitstream&)), SLOT(handleMessage(const QVariant&, Bitstream&)));
    
    sequencer->connect(this, SIGNAL(destroyed(QObject*)), SLOT(clearReliableChannel(QObject*)));
}

void ReliableChannel::writeData(QDataStream& out, int bytes, QVector<DatagramSequencer::ChannelSpan>& spans) {
    if (bytes == 0) {
        out << (quint32)0;
        return;
    }
    _writePosition %= _buffer.pos();
    while (bytes > 0) {
        int position = 0;
        for (int i = 0; i < _acknowledged.getSpans().size(); i++) {
            const SpanList::Span& span = _acknowledged.getSpans().at(i);
            position += span.unset;
            if (_writePosition < position) {
                int start = qMax(position - span.unset, _writePosition);
                int length = qMin(bytes, position - start);
                writeSpan(out, start, length, spans);
                writeFullSpans(out, bytes - length, i + 1, position + span.set, spans);
                out << (quint32)0;
                return;
            }
            position += span.set;
        }
        int leftover = _buffer.pos() - position;
        position = _buffer.pos();
        
        if (_writePosition < position && leftover > 0) {
            int start = qMax(position - leftover, _writePosition);
            int length = qMin(bytes, position - start);
            writeSpan(out, start, length, spans);
            writeFullSpans(out, bytes - length, 0, 0, spans);
            out << (quint32)0;
            return;
        }
        _writePosition = 0;
    }
}

void ReliableChannel::writeFullSpans(QDataStream& out, int bytes, int startingIndex, int position,
        QVector<DatagramSequencer::ChannelSpan>& spans) {
    int expandedSize = _acknowledged.getSpans().size() + 1;
    for (int i = 0; i < expandedSize; i++) {
        if (bytes == 0) {
            return;
        }
        int index = (startingIndex + i) % expandedSize;
        if (index == _acknowledged.getSpans().size()) {
            int leftover = _buffer.pos() - position;
            if (leftover > 0) {
                int length = qMin(leftover, bytes);
                writeSpan(out, position, length, spans);
                bytes -= length;
            }
            position = 0;
            
        } else {
            const SpanList::Span& span = _acknowledged.getSpans().at(index);
            int length = qMin(span.unset, bytes);
            writeSpan(out, position, length, spans);
            bytes -= length;
            position += (span.unset + span.set);
        }
    }
}

int ReliableChannel::writeSpan(QDataStream& out, int position, int length, QVector<DatagramSequencer::ChannelSpan>& spans) {
    DatagramSequencer::ChannelSpan span = { _index, _offset + position, length };
    spans.append(span);
    out << (quint32)length;
    out << (quint32)span.offset;
    _buffer.writeToStream(position, length, out);
    _writePosition = position + length;
    
    return length;
}

void ReliableChannel::spanAcknowledged(const DatagramSequencer::ChannelSpan& span) {
    int advancement = _acknowledged.set(span.offset - _offset, span.length);
    if (advancement > 0) {
        _buffer.remove(advancement);
        _buffer.seek(_buffer.size());
        
        _offset += advancement;
        _writePosition = qMax(_writePosition - advancement, 0);   
    }
}

void ReliableChannel::spanLost(int packetNumber, int nextOutgoingPacketNumber) {
    // reset the write position up to once each round trip time
    if (packetNumber >= _writePositionResetPacketNumber) {
        _writePosition = 0;
        _writePositionResetPacketNumber = nextOutgoingPacketNumber;
    }
}

void ReliableChannel::readData(QDataStream& in) {
    bool readSome = false;
    forever {
        quint32 size;
        in >> size;
        if (size == 0) {
            break;
        }
        quint32 offset;
        in >> offset;
        
        int position = offset - _offset;
        int end = position + size;
        if (end <= 0) {
            in.skipRawData(size);
            
        } else if (position < 0) {
            in.skipRawData(-position);
            _assemblyBuffer.readFromStream(0, end, in);
            
        } else {
            _assemblyBuffer.readFromStream(position, size, in);
        }
        int advancement = _acknowledged.set(position, size);
        if (advancement > 0) {
            _assemblyBuffer.appendToBuffer(0, advancement, _buffer);
            _assemblyBuffer.remove(advancement);
            _offset += advancement;
            readSome = true;
        }
    }
    if (!readSome) {
        return;
    }
    
    forever {
        // if we're expecting a message, peek into the buffer to see if we have the whole thing.
        // if so, read it in, handle it, and loop back around in case there are more
        if (_messagesEnabled) {
            quint32 available = _buffer.bytesAvailable();
            if (available >= sizeof(quint32)) {
                quint32 length;
                _buffer.readBytes(_buffer.pos(), sizeof(quint32), (char*)&length);
                if (available >= length) {
                    _dataStream.skipRawData(sizeof(quint32));
                    QVariant message;
                    _bitstream >> message;
                    emit receivedMessage(message, _bitstream);
                    _bitstream.reset();
                    _bitstream.persistAndResetReadMappings();
                    continue;
                }
            }
        // otherwise, just let whoever's listening know that data is available
        } else {
            emit _buffer.readyRead();
        }
        break;
    }
    
    // prune any read data from the buffer
    if (_buffer.pos() > 0) {
        _buffer.remove((int)_buffer.pos());
        _buffer.seek(0);
    }
}

void ReliableChannel::pruneOutgoingMessageStats() {
    while (!_outgoingMessageStats.isEmpty() && _offset >= _outgoingMessageStats.first().first) {
        _outgoingMessageStats.removeFirst();
    }
}

