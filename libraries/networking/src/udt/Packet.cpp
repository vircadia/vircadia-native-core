//
//  Packet.cpp
//  libraries/networking/src
//
//  Created by Clement on 7/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Packet.h"

#include <array>

#include <LogHandler.h>

#include "Socket.h"

using namespace udt;

int packetMetaTypeId = qRegisterMetaType<Packet*>("Packet*");

using Key = uint64_t;
static const std::array<Key, 4> KEYS {{
    0x0,
    0x6362726973736574,
    0x7362697261726461,
    0x72687566666d616e,
}};

void xorHelper(char* start, int size, Key key) {
    auto current = start;
    auto xorValue = reinterpret_cast<const char*>(&key);
    for (int i = 0; i < size; ++i) {
        *(current++) ^= *(xorValue + (i % sizeof(Key)));
    }
}

int Packet::localHeaderSize(bool isPartOfMessage) {
    return sizeof(Packet::SequenceNumberAndBitField) +
            (isPartOfMessage ? sizeof(Packet::MessageNumberAndBitField) + sizeof(MessagePartNumber) : 0);
}

int Packet::totalHeaderSize(bool isPartOfMessage) {
    return BasePacket::totalHeaderSize() + Packet::localHeaderSize(isPartOfMessage);
}

int Packet::maxPayloadSize(bool isPartOfMessage) {
    return BasePacket::maxPayloadSize() - Packet::localHeaderSize(isPartOfMessage);
}


std::unique_ptr<Packet> Packet::create(qint64 size, bool isReliable, bool isPartOfMessage) {
    auto packet = std::unique_ptr<Packet>(new Packet(size, isReliable, isPartOfMessage));
    
    packet->open(QIODevice::ReadWrite);
   
    return packet;
}

std::unique_ptr<Packet> Packet::fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr) {
    // Fail with invalid size
    Q_ASSERT(size >= 0);

    // allocate memory
    auto packet = std::unique_ptr<Packet>(new Packet(std::move(data), size, senderSockAddr));

    packet->open(QIODevice::ReadOnly);

    return packet;
}

std::unique_ptr<Packet> Packet::createCopy(const Packet& other) {
    return std::unique_ptr<Packet>(new Packet(other));
}

Packet::Packet(qint64 size, bool isReliable, bool isPartOfMessage) :
    BasePacket((size == -1) ? -1 : (Packet::localHeaderSize() + size)),
    _isReliable(isReliable),
    _isPartOfMessage(isPartOfMessage)
{
    adjustPayloadStartAndCapacity(Packet::localHeaderSize(_isPartOfMessage));
    // set the UDT header to default values
    writeHeader();
}

Packet::Packet(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    BasePacket(std::move(data), size, senderSockAddr)
{
    readHeader();

    adjustPayloadStartAndCapacity(Packet::localHeaderSize(_isPartOfMessage), _payloadSize > 0);

    if (getObfuscationLevel() != Packet::NoObfuscation) {
#ifdef UDT_CONNECTION_DEBUG
        QString debugString = "Unobfuscating packet %1 with level %2";
        debugString = debugString.arg(QString::number((uint32_t)getSequenceNumber()),
                                      QString::number(getObfuscationLevel()));

        if (isPartOfMessage()) {
            debugString += "\n";
            debugString += "    Message Number: %1, Part Number: %2.";
            debugString = debugString.arg(QString::number(getMessageNumber()),
                                          QString::number(getMessagePartNumber()));
        }

        static QString repeatedMessage = LogHandler::getInstance().addRepeatedMessageRegex("^Unobfuscating packet .*");
        qDebug() << qPrintable(debugString);
#endif

        obfuscate(NoObfuscation); // Undo obfuscation
    }
}

Packet::Packet(const Packet& other) : BasePacket(other) {
    copyMembers(other);
}

Packet& Packet::operator=(const Packet& other) {
    BasePacket::operator=(other);

    copyMembers(other);

    return *this;
}

Packet::Packet(Packet&& other) : BasePacket(std::move(other)) {
    copyMembers(other);
}

Packet& Packet::operator=(Packet&& other) {
    BasePacket::operator=(std::move(other));

    copyMembers(other);

    return *this;
}

void Packet::writeMessageNumber(MessageNumber messageNumber, PacketPosition position, MessagePartNumber messagePartNumber) {
    _isPartOfMessage = true;
    _messageNumber = messageNumber;
    _packetPosition = position;
    _messagePartNumber = messagePartNumber;
    writeHeader();
}

void Packet::writeSequenceNumber(SequenceNumber sequenceNumber) const {
    _sequenceNumber = sequenceNumber;
    writeHeader();
}

void Packet::obfuscate(ObfuscationLevel level) {
    auto obfuscationKey = KEYS[getObfuscationLevel()] ^ KEYS[level]; // Undo old and apply new one.
    if (obfuscationKey != 0) {
        xorHelper(getData() + localHeaderSize(isPartOfMessage()),
                  getDataSize() - localHeaderSize(isPartOfMessage()), obfuscationKey);

        // Update members and header
        _obfuscationLevel = level;
        writeHeader();
    }
}

void Packet::copyMembers(const Packet& other) {
    _isReliable = other._isReliable;
    _isPartOfMessage = other._isPartOfMessage;
    _obfuscationLevel = other._obfuscationLevel;
    _sequenceNumber = other._sequenceNumber;
    _packetPosition = other._packetPosition;
    _messageNumber = other._messageNumber;
    _messagePartNumber = other._messagePartNumber;
}

void Packet::readHeader() const {
    SequenceNumberAndBitField* seqNumBitField = reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    
    Q_ASSERT_X(!(*seqNumBitField & CONTROL_BIT_MASK), "Packet::readHeader()", "This should be a data packet");
    
    _isReliable = (bool) (*seqNumBitField & RELIABILITY_BIT_MASK); // Only keep reliability bit
    _isPartOfMessage = (bool) (*seqNumBitField & MESSAGE_BIT_MASK); // Only keep message bit
    _obfuscationLevel = (ObfuscationLevel)((*seqNumBitField & OBFUSCATION_LEVEL_MASK) >> OBFUSCATION_LEVEL_OFFSET);
    _sequenceNumber = SequenceNumber{ *seqNumBitField & SEQUENCE_NUMBER_MASK }; // Remove the bit field

    if (_isPartOfMessage) {
        MessageNumberAndBitField* messageNumberAndBitField = seqNumBitField + 1;

        _messageNumber = *messageNumberAndBitField & MESSAGE_NUMBER_MASK;
        _packetPosition = static_cast<PacketPosition>(*messageNumberAndBitField >> PACKET_POSITION_OFFSET);

        MessagePartNumber* messagePartNumber = messageNumberAndBitField + 1;
        _messagePartNumber = *messagePartNumber;
    }
}

void Packet::writeHeader() const {
    // grab pointer to current SequenceNumberAndBitField
    SequenceNumberAndBitField* seqNumBitField = reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    
    // Write sequence number and reset bit field
    Q_ASSERT_X(!((SequenceNumber::Type)_sequenceNumber & BIT_FIELD_MASK),
               "Packet::writeHeader()", "Sequence number is overflowing into bit field");
    *seqNumBitField = ((SequenceNumber::Type)_sequenceNumber);
    
    if (_isReliable) {
        *seqNumBitField |= RELIABILITY_BIT_MASK;
    }

    if (_obfuscationLevel != NoObfuscation) {
        *seqNumBitField |= (_obfuscationLevel << OBFUSCATION_LEVEL_OFFSET);
    }
    
    if (_isPartOfMessage) {
        *seqNumBitField |= MESSAGE_BIT_MASK;

        Q_ASSERT_X(!(_messageNumber & PACKET_POSITION_MASK),
                   "Packet::writeHeader()", "Message number is overflowing into bit field");

        MessageNumberAndBitField* messageNumberAndBitField = seqNumBitField + 1;
        *messageNumberAndBitField = _messageNumber;
        *messageNumberAndBitField |= _packetPosition << PACKET_POSITION_OFFSET;
        
        MessagePartNumber* messagePartNumber = messageNumberAndBitField + 1;
        *messagePartNumber = _messagePartNumber;
    }
}
