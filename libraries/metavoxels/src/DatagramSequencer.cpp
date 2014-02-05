//
//  DatagramSequencer.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>

#include <QtDebug>

#include "DatagramSequencer.h"

const int MAX_DATAGRAM_SIZE = 1500;

const int DEFAULT_MAX_PACKET_SIZE = 3000;

DatagramSequencer::DatagramSequencer(const QByteArray& datagramHeader) :
    _outgoingPacketStream(&_outgoingPacketData, QIODevice::WriteOnly),
    _outputStream(_outgoingPacketStream),
    _incomingDatagramStream(&_incomingDatagramBuffer),
    _datagramHeaderSize(datagramHeader.size()),
    _outgoingPacketNumber(0),
    _outgoingDatagram(MAX_DATAGRAM_SIZE, 0),
    _outgoingDatagramBuffer(&_outgoingDatagram),
    _outgoingDatagramStream(&_outgoingDatagramBuffer),
    _incomingPacketNumber(0),
    _incomingPacketStream(&_incomingPacketData, QIODevice::ReadOnly),
    _inputStream(_incomingPacketStream),
    _receivedHighPriorityMessages(0),
    _maxPacketSize(DEFAULT_MAX_PACKET_SIZE) {

    _outgoingPacketStream.setByteOrder(QDataStream::LittleEndian);
    _incomingDatagramStream.setByteOrder(QDataStream::LittleEndian);
    _incomingPacketStream.setByteOrder(QDataStream::LittleEndian);
    _outgoingDatagramStream.setByteOrder(QDataStream::LittleEndian);

    connect(&_outputStream, SIGNAL(sharedObjectCleared(int)), SLOT(sendClearSharedObjectMessage(int)));

    memcpy(_outgoingDatagram.data(), datagramHeader.constData(), _datagramHeaderSize);
}

void DatagramSequencer::sendReliableMessage(const QVariant& data, int channel) {
    
}

void DatagramSequencer::sendHighPriorityMessage(const QVariant& data) {
    HighPriorityMessage message = { data, _outgoingPacketNumber + 1 };
    _highPriorityMessages.append(message);
}

ReliableChannel* DatagramSequencer::getReliableOutputChannel(int index) {
    ReliableChannel*& channel = _reliableOutputChannels[index];
    if (!channel) {
        channel = new ReliableChannel(this);
    }
    return channel;
}
    
ReliableChannel* DatagramSequencer::getReliableInputChannel(int index) {
    ReliableChannel*& channel = _reliableInputChannels[index];
    if (!channel) {
        channel = new ReliableChannel(this);
    }
    return channel;
}

Bitstream& DatagramSequencer::startPacket() {
    // start with the list of acknowledgements
    _outgoingPacketStream << (quint32)_receiveRecords.size();
    foreach (const ReceiveRecord& record, _receiveRecords) {
        _outgoingPacketStream << (quint32)record.packetNumber;
    }
    
    // write the high-priority messages
    _outgoingPacketStream << (quint32)_highPriorityMessages.size();
    foreach (const HighPriorityMessage& message, _highPriorityMessages) {
        _outputStream << message.data;
    }
    
    // return the stream, allowing the caller to write the rest
    return _outputStream;
}

void DatagramSequencer::endPacket() {
    _outputStream.flush();
    
    // if we have space remaining, send some data from our reliable channels 
    int remaining = _maxPacketSize - _outgoingPacketStream.device()->pos();
    const int MINIMUM_RELIABLE_SIZE = sizeof(quint32) * 4; // count, channel number, offset, size
    if (remaining > MINIMUM_RELIABLE_SIZE) {
        appendReliableData(remaining);
    } else {
        _outgoingPacketStream << (quint32)0;
    }
    
    sendPacket(QByteArray::fromRawData(_outgoingPacketData.constData(), _outgoingPacketStream.device()->pos()));
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
        QList<SendRecord>::iterator it = _sendRecords.begin() + index;
        sendRecordAcknowledged(*it);
        emit sendAcknowledged(index);
        _sendRecords.erase(_sendRecords.begin(), it + 1);
    }
    
    // read and dispatch the high-priority messages
    quint32 highPriorityMessageCount;
    _incomingPacketStream >> highPriorityMessageCount;
    int newHighPriorityMessages = highPriorityMessageCount - _receivedHighPriorityMessages;
    for (int i = 0; i < highPriorityMessageCount; i++) {
        QVariant data;
        _inputStream >> data;
        if (i >= _receivedHighPriorityMessages) {
            emit receivedHighPriorityMessage(data);
        }
    }
    _receivedHighPriorityMessages = highPriorityMessageCount;
    
    // alert external parties so that they can read the middle
    emit readyToRead(_inputStream);
    
    // read the reliable data, if any
    quint32 reliableChannels;
    _incomingPacketStream >> reliableChannels;
    for (int i = 0; i < reliableChannels; i++) {
        quint32 channelIndex;
        _incomingPacketStream >> channelIndex;
        getReliableOutputChannel(channelIndex)->readData(_incomingPacketStream);
    }
    
    _incomingPacketStream.device()->seek(0);
    _inputStream.reset();
    
    // record the receipt
    ReceiveRecord record = { _incomingPacketNumber, _inputStream.getAndResetReadMappings(), newHighPriorityMessages };
    _receiveRecords.append(record);
}

void DatagramSequencer::sendClearSharedObjectMessage(int id) {
    qDebug() << "cleared " << id;
}

void DatagramSequencer::sendRecordAcknowledged(const SendRecord& record) {
    // stop acknowledging the recorded packets
    while (!_receiveRecords.isEmpty() && _receiveRecords.first().packetNumber <= record.lastReceivedPacketNumber) {
        const ReceiveRecord& received = _receiveRecords.first();
        _inputStream.persistReadMappings(received.mappings);
        _receivedHighPriorityMessages -= received.newHighPriorityMessages;
        emit receiveAcknowledged(0);
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
}

void DatagramSequencer::appendReliableData(int bytes) {
    _outgoingPacketStream << (quint32)0;

    for (QHash<int, ReliableChannel*>::const_iterator it = _reliableOutputChannels.constBegin();
            it != _reliableOutputChannels.constEnd(); it++) {
        
    }
}

void DatagramSequencer::sendPacket(const QByteArray& packet) {
    QIODeviceOpener opener(&_outgoingDatagramBuffer, QIODevice::WriteOnly);
    
    // increment the packet number
    _outgoingPacketNumber++;
    
    // record the send
    SendRecord record = { _outgoingPacketNumber, _receiveRecords.isEmpty() ? 0 : _receiveRecords.last().packetNumber,
        _outputStream.getAndResetWriteMappings() };
    _sendRecords.append(record);
    
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

void ReliableChannel::sendMessage(const QVariant& message) {
    _bitstream << message;
}

void ReliableChannel::sendClearSharedObjectMessage(int id) {
}

ReliableChannel::ReliableChannel(DatagramSequencer* sequencer) :
    QObject(sequencer),
    _dataStream(&_buffer),
    _bitstream(_dataStream),
    _priority(1.0f) {
    
    _buffer.open(QIODevice::WriteOnly);
    _dataStream.setByteOrder(QDataStream::LittleEndian);
    
    connect(&_bitstream, SIGNAL(sharedObjectCleared(int)), SLOT(sendClearSharedObjectMessage(int)));
}

void ReliableChannel::readData(QDataStream& in) {
    quint32 offset, size;
    in >> offset >> size;
    
    
    in.skipRawData(size);
}
