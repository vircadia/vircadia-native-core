//
//  DatagramSequencer.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>

#include "DatagramSequencer.h"

const int MAX_DATAGRAM_SIZE = 1500;

DatagramSequencer::DatagramSequencer() :
    _outgoingPacketStream(&_outgoingPacketData, QIODevice::WriteOnly),
    _outputStream(_outgoingPacketStream),
    _datagramStream(&_datagramBuffer),
    _outgoingPacketNumber(0),
    _outgoingDatagram(MAX_DATAGRAM_SIZE, 0),
    _incomingPacketNumber(0),
    _incomingPacketStream(&_incomingPacketData, QIODevice::ReadOnly),
    _inputStream(_incomingPacketStream) {

    _datagramStream.setByteOrder(QDataStream::LittleEndian);
}

/// Simple RAII-style object to keep a device open when in scope.
class QIODeviceOpener {
public:
    
    QIODeviceOpener(QIODevice* device, QIODevice::OpenMode mode) : _device(device) { _device->open(mode); }
    ~QIODeviceOpener() { _device->close(); }

private:
    
    QIODevice* _device;
};

Bitstream& DatagramSequencer::startPacket() {
    // start with the list of acknowledgements
    _outgoingPacketStream << (quint32)_receivedPacketNumbers.size();
    foreach (int packetNumber, _receivedPacketNumbers) {
        _outgoingPacketStream << (quint32)packetNumber;
    }
    return _outputStream;
}

void DatagramSequencer::endPacket() {
    _outputStream.flush();
    sendPacket(QByteArray::fromRawData(_outgoingPacketData.constData(), _outgoingPacketStream.device()->pos()));
    _outgoingPacketStream.device()->seek(0);
}

void DatagramSequencer::receivedDatagram(const QByteArray& datagram) {
    _datagramBuffer.setBuffer(const_cast<QByteArray*>(&datagram));
    QIODeviceOpener opener(&_datagramBuffer, QIODevice::ReadOnly);
     
    // read the sequence number
    quint32 sequenceNumber;
    _datagramStream >> sequenceNumber;
    
    // if it's less than the last, ignore
    if (sequenceNumber < _incomingPacketNumber) {
        return;
    }
    
    // read the size and offset
    quint32 packetSize, offset;
    _datagramStream >> packetSize >> offset;
    
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
    memcpy(_incomingPacketData.data() + offset, datagram.constData() + _datagramBuffer.pos(), _datagramBuffer.bytesAvailable());
    
    // see if we're done
    if ((_remainingBytes -= _datagramBuffer.bytesAvailable()) > 0) {
        return;
    }
    _receivedPacketNumbers.append(_incomingPacketNumber);    
    
    // read the list of acknowledged packets
    quint32 acknowledgementCount;
    _incomingPacketStream >> acknowledgementCount;
    for (int i = 0; i < acknowledgementCount; i++) {
        quint32 packetNumber;
        _incomingPacketStream >> packetNumber;
        if (_sendRecords.isEmpty()) {
            continue;
        }
        int index = packetNumber - _sendRecords.first().packetNumber;
        if (index >= _sendRecords.size()) {
            continue;
        }
        QList<SendRecord>::iterator it = _sendRecords.begin() + index;
        sendRecordAcknowledged(*it);
        _sendRecords.erase(_sendRecords.begin(), it + 1);
    }
    
    emit readyToRead(_inputStream);
    _incomingPacketStream.device()->seek(0);
    _inputStream.reset();
}

void DatagramSequencer::sendRecordAcknowledged(const SendRecord& record) {
    // stop acknowledging the recorded packets (TODO: replace with interpolation search?)
    QList<int>::iterator it = qBinaryFind(_receivedPacketNumbers.begin(), _receivedPacketNumbers.end(),
        record.lastReceivedPacketNumber);
    if (it != _receivedPacketNumbers.end()) {
        _receivedPacketNumbers.erase(it + 1);
    }
    
    
}

void DatagramSequencer::sendPacket(const QByteArray& packet) {
    _datagramBuffer.setBuffer(&_outgoingDatagram);
    QIODeviceOpener opener(&_datagramBuffer, QIODevice::WriteOnly);
    
    // increment the packet number
    _outgoingPacketNumber++;
    
    // record the send
    SendRecord record = { _outgoingPacketNumber, _receivedPacketNumbers.isEmpty() ? 0 : _receivedPacketNumbers.last(),
        _outputStream.getAndResetWriteMappings() };
    _sendRecords.append(record);
    
    // write the sequence number and size, which are the same between all fragments
    _datagramStream << (quint32)_outgoingPacketNumber;
    _datagramStream << (quint32)packet.size();
    int initialPosition = _datagramBuffer.pos();
    
    // break the packet into MTU-sized datagrams
    int offset = 0;
    do {
        _datagramBuffer.seek(initialPosition);
        _datagramStream << (quint32)offset;
        
        int payloadSize = qMin((int)(_outgoingDatagram.size() - _datagramBuffer.pos()), packet.size() - offset);
        memcpy(_outgoingDatagram.data() + _datagramBuffer.pos(), packet.constData() + offset, payloadSize);
        
        emit readyToWrite(QByteArray::fromRawData(_outgoingDatagram.constData(), _datagramBuffer.pos() + payloadSize));
        
        offset += payloadSize;
        
    } while(offset < packet.size());
}

void DatagramSequencer::packetAcknowledged(int packetNumber) {
    
}
