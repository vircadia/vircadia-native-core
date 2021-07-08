//
//  UDTTest.cpp
//  tools/udt-test/src
//
//  Created by Stephen Birarda on 2015-07-30.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UDTTest.h"

#include <QtCore/QDebug>

#include <udt/Constants.h>
#include <udt/Packet.h>
#include <udt/PacketList.h>

#include <LogHandler.h>

const QCommandLineOption PORT_OPTION { "p", "listening port for socket (defaults to random)", "port", 0 };
const QCommandLineOption TARGET_OPTION {
    "target", "target for sent packets (default is listen only)",
    "IP:PORT or HOSTNAME:PORT"
};
const QCommandLineOption PACKET_SIZE {
    "packet-size", "size for sent packets in bytes (defaults to " + QString::number(udt::MAX_PACKET_SIZE) + ")", "bytes",
    QString(udt::MAX_PACKET_SIZE)
};
const QCommandLineOption MIN_PACKET_SIZE {
    "min-packet-size", "min size for sent packets in bytes", "min bytes"
};
const QCommandLineOption MAX_PACKET_SIZE {
    "max-packet-size", "max size for sent packets in bytes", "max bytes"
};
const QCommandLineOption MAX_SEND_BYTES {
    "max-send-bytes", "number of bytes to send before stopping (default is infinite)", "max bytes"
};
const QCommandLineOption MAX_SEND_PACKETS {
    "max-send-packets", "number of packets to send before stopping (default is infinite)", "max packets"
};
const QCommandLineOption UNRELIABLE_PACKETS {
    "unreliable", "send unreliable packets (default is reliable)"
};
const QCommandLineOption ORDERED_PACKETS {
    "ordered", "send ordered packets (default is unordered)"
};
const QCommandLineOption MESSAGE_SIZE {
    "message-size", "megabytes per message payload for ordered sending (default is 20)", "megabytes"
};
const QCommandLineOption MESSAGE_SEED {
    "message-seed", "seed used for random number generation to match ordered messages (default is 742272)", "integer"
};
const QCommandLineOption STATS_INTERVAL {
    "stats-interval", "stats output interval (default is 100ms)", "milliseconds"
};

const QStringList CLIENT_STATS_TABLE_HEADERS {
    "Send (Mb/s)", "Est. Max (Mb/s)", "RTT (ms)", "CW (P)", "Period (us)",
    "Recv ACK", "Procd ACK", "Sent Packets", "Re-sent Packets"
};

const QStringList SERVER_STATS_TABLE_HEADERS {
    "  Mb/s  ", "Recv Mb/s", "Est. Max (Mb/s)", "RTT (ms)", "CW (P)",
    "Sent ACK", "Duplicates (P)"
};

UDTTest::UDTTest(int& argc, char** argv) :
    QCoreApplication(argc, argv)
{
    parseArguments();
    
    // randomize the seed for packet size randomization
    srand(time(NULL));

    _socket.bind(QHostAddress::AnyIPv4, _argumentParser.value(PORT_OPTION).toUInt());
    qDebug() << "Test socket is listening on" << _socket.localPort();
    
    if (_argumentParser.isSet(TARGET_OPTION)) {
        // parse the IP and port combination for this target
        QString hostnamePortString = _argumentParser.value(TARGET_OPTION);
        
        QHostAddress address { hostnamePortString.left(hostnamePortString.indexOf(':')) };
        quint16 port { (quint16) hostnamePortString.mid(hostnamePortString.indexOf(':') + 1).toUInt() };
        
        if (address.isNull() || port == 0) {
            qCritical() << "Could not parse an IP address and port combination from" << hostnamePortString << "-" <<
                "The parsed IP was" << address.toString() << "and the parsed port was" << port;
            
            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
        } else {
            _target = SockAddr(address, port);
            qDebug() << "Packets will be sent to" << _target;
        }
    }
    
    if (_argumentParser.isSet(PACKET_SIZE)) {
        // parse the desired packet size
        _minPacketSize = _maxPacketSize = _argumentParser.value(PACKET_SIZE).toInt();
        
        if (_argumentParser.isSet(MIN_PACKET_SIZE) || _argumentParser.isSet(MAX_PACKET_SIZE)) {
            qCritical() << "Cannot set a min packet size or max packet size AND a specific packet size.";
            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
        }
    } else {
        
        bool customMinSize = false;
        
        if (_argumentParser.isSet(MIN_PACKET_SIZE)) {
            _minPacketSize = _argumentParser.value(MIN_PACKET_SIZE).toInt();
            customMinSize = true;
        }
        
        if (_argumentParser.isSet(MAX_PACKET_SIZE)) {
            _maxPacketSize = _argumentParser.value(MAX_PACKET_SIZE).toInt();
            
            // if we don't have a min packet size we should make it 1, because we have a max
            if (customMinSize) {
                _minPacketSize = 1;
            }
        }
        
        if (_maxPacketSize < _minPacketSize) {
            qCritical() << "Cannot set a max packet size that is smaller than the min packet size.";
            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
        }
    }
    
    if (_argumentParser.isSet(MAX_SEND_BYTES)) {
        _maxSendBytes = _argumentParser.value(MAX_SEND_BYTES).toInt();
    }
    
    if (_argumentParser.isSet(MAX_SEND_PACKETS)) {
        _maxSendPackets = _argumentParser.value(MAX_SEND_PACKETS).toInt();
    }
    
    if (_argumentParser.isSet(UNRELIABLE_PACKETS)) {
        _sendReliable = false;
    }

    if (_argumentParser.isSet(ORDERED_PACKETS)) {
        _sendOrdered = true;
    }
    
    if (_argumentParser.isSet(MESSAGE_SIZE)) {
        if (_argumentParser.isSet(ORDERED_PACKETS)) {
            static const double BYTES_PER_MEGABYTE = 1000000;
            _messageSize = (int) _argumentParser.value(MESSAGE_SIZE).toInt() * BYTES_PER_MEGABYTE;
            
            qDebug() << "Message size for ordered packet sending is" << QString("%1MB").arg(_messageSize / BYTES_PER_MEGABYTE);
        } else {
            qWarning() << "message-size has no effect if not sending ordered - it will be ignored";
        }
    }
    
    
    // in case we're an ordered sender or receiver setup our random number generator now
    static const int FIRST_MESSAGE_SEED = 742272;
    
    int messageSeed = FIRST_MESSAGE_SEED;
    
    if (_argumentParser.isSet(MESSAGE_SEED)) {
        messageSeed = _argumentParser.value(MESSAGE_SEED).toInt();
    }
    
    // seed the generator with a value that the receiver will also use when verifying the ordered message
    _generator.seed(messageSeed);
    
    if (!_target.isNull()) {
        sendInitialPackets();
    } else {
        // this is a receiver - in case there are ordered packets (messages) being sent to us make sure that we handle them
        // so that they can be verified
        _socket.setMessageHandler(
            [this](std::unique_ptr<udt::Packet> packet) {
                auto messageNumber = packet->getMessageNumber();
                auto it = _pendingMessages.find(messageNumber);

                if (it == _pendingMessages.end()) {
                    auto message = std::unique_ptr<Message>(new Message { messageNumber, packet->readAll() });
                    message->data.reserve(_messageSize);
                    if (packet->getPacketPosition() == udt::Packet::ONLY) {
                        handleMessage(std::move(message));
                    } else {
                        _pendingMessages[messageNumber] = std::move(message);
                    }
                } else {
                    auto& message = it->second;
                    message->data.append(packet->readAll());

                    if (packet->getPacketPosition() == udt::Packet::LAST) {
                        handleMessage(std::move(message));
                        _pendingMessages.erase(it);
                    }
                }

        });
    }
    _socket.setMessageFailureHandler(
        [this](SockAddr from, udt::Packet::MessageNumber messageNumber) {
            _pendingMessages.erase(messageNumber);
        }
    );
    
    // the sender reports stats every 100 milliseconds, unless passed a custom value
    
    if (_argumentParser.isSet(STATS_INTERVAL)) {
        _statsInterval = _argumentParser.value(STATS_INTERVAL).toInt();
    }
    
    QTimer* statsTimer = new QTimer(this);
    connect(statsTimer, &QTimer::timeout, this, &UDTTest::sampleStats);
    statsTimer->start(_statsInterval);
}

void UDTTest::parseArguments() {
    // use a QCommandLineParser to setup command line arguments and give helpful output
    _argumentParser.setApplicationDescription("High Fidelity UDT Protocol Test Client");
    _argumentParser.addHelpOption();
    
    const QCommandLineOption helpOption = _argumentParser.addHelpOption();
    
    _argumentParser.addOptions({
        PORT_OPTION, TARGET_OPTION, PACKET_SIZE, MIN_PACKET_SIZE, MAX_PACKET_SIZE,
        MAX_SEND_BYTES, MAX_SEND_PACKETS, UNRELIABLE_PACKETS, ORDERED_PACKETS,
        MESSAGE_SIZE, MESSAGE_SEED, STATS_INTERVAL
    });
    
    if (!_argumentParser.parse(arguments())) {
        qCritical() << _argumentParser.errorText();
        _argumentParser.showHelp();
        Q_UNREACHABLE();
    }
    
    if (_argumentParser.isSet(helpOption)) {
        _argumentParser.showHelp();
        Q_UNREACHABLE();
    }
}

void UDTTest::sendInitialPackets() {
    static const int NUM_INITIAL_PACKETS = 500;
    
    int numPackets = std::max(NUM_INITIAL_PACKETS, _maxSendPackets);
    
    for (int i = 0; i < numPackets; ++i) {
        sendPacket();
    }
    
    if (numPackets == NUM_INITIAL_PACKETS) {
        // we've put 500 initial packets in the queue, everytime we hear one has gone out we should add a new one
        _socket.connectToSendSignal(_target, this, SLOT(refillPacket()));
    }
}

void UDTTest::sendPacket() {
    
    if (_maxSendPackets != -1 && _totalQueuedPackets > _maxSendPackets) {
        // don't send more packets, we've hit max
        return;
    }
    
    if (_maxSendBytes != -1 && _totalQueuedBytes > _maxSendBytes) {
        // don't send more packets, we've hit max
        return;
    }
    
    // we're good to send a new packet, construct it now
    
    // figure out what size the packet will be
    int packetPayloadSize = 0;
    
    if (_minPacketSize == _maxPacketSize) {
        // we know what size we want - figure out the payload size
        packetPayloadSize = _maxPacketSize - udt::Packet::localHeaderSize(false);
    } else {
        // pick a random size in our range
        int randomPacketSize = rand() % _maxPacketSize + _minPacketSize;
        packetPayloadSize = randomPacketSize - udt::Packet::localHeaderSize(false);
    }

    if (_sendOrdered) {
        // check if it is time to add another message - we do this every time 95% of the message size has been sent
        static int call = 0;
        static int packetSize = udt::Packet::maxPayloadSize(true);
        static int messageSizePackets = (int) ceil(_messageSize / udt::Packet::maxPayloadSize(true));
        
        static int refillCount = (int) (messageSizePackets * 0.95);
        
        if (call++ % refillCount == 0) {
            // construct a reliable and ordered packet list
            auto packetList = udt::PacketList::create(PacketType::BulkAvatarData, QByteArray(), true, true);
            
            // fill the packet list with random data according to the constant seed (so receiver can verify)
            for (int i = 0; i < messageSizePackets; ++i) {
                // setup a QByteArray full of zeros for our random padded data
                QByteArray randomPaddedData { packetSize, 0 };
                
                // generate a random integer for the first 8 bytes of the random data
                uint64_t randomInt = _distribution(_generator);
                randomPaddedData.replace(0, sizeof(randomInt), reinterpret_cast<char*>(&randomInt), sizeof(randomInt));
                
                // write this data to the PacketList
                packetList->write(randomPaddedData);
            }
            
            packetList->closeCurrentPacket();
            
            _totalQueuedBytes += (int)packetList->getDataSize();
            _totalQueuedPackets += (int)packetList->getNumPackets();
            
            _socket.writePacketList(std::move(packetList), _target);
        }
        
    } else {
        auto newPacket = udt::Packet::create(packetPayloadSize, _sendReliable);
        newPacket->setPayloadSize(packetPayloadSize);
        
        _totalQueuedBytes += newPacket->getDataSize();
        
        // queue or send this packet by calling write packet on the socket for our target
        if (_sendReliable) {
            _socket.writePacket(std::move(newPacket), _target);
        } else {
            _socket.writePacket(*newPacket, _target);
        }
        
        ++_totalQueuedPackets;
    }
    
}

void UDTTest::handleMessage(std::unique_ptr<Message> message) {
    // generate the byte array that should match this message - using the same seed the sender did
    
    int packetSize = udt::Packet::maxPayloadSize(true);
    int messageSize = message->data.size();
    
    QByteArray messageData(messageSize, 0);
   
    for (int i = 0; i < messageSize; i += packetSize) {
        // generate the random 64-bit unsigned integer that should lead this packet
        uint64_t randomInt = _distribution(_generator);
        
        messageData.replace(i, sizeof(randomInt), reinterpret_cast<char*>(&randomInt), sizeof(randomInt));
    }
    
    bool dataMatch = messageData == message->data;
    
    Q_ASSERT_X(dataMatch, "UDTTest::handleMessage",
               "received message did not match expected message (from seeded random number generation).");
    
    if (!dataMatch) {
        qCritical() << "UDTTest::handleMessage" << "received message did not match expected message"
            << "(from seeded random number generation).";
    }
}

void UDTTest::sampleStats() {
    static bool first = true;
    static const double USECS_PER_MSEC = 1000.0;
    static const double MEGABITS_PER_BYTE = 8.0 / 1000000.0;
    static const double MS_PER_SECOND = 1000.0;
    static const double PPS_TO_MBPS = udt::MAX_PACKET_SIZE * MEGABITS_PER_BYTE;


    if (!_target.isNull()) {
        if (first) {
            // output the headers for stats for our table
            qDebug() << qPrintable(CLIENT_STATS_TABLE_HEADERS.join(" | "));
            first = false;
        }
        
        udt::ConnectionStats::Stats stats = _socket.sampleStatsForConnection(_target);
        
        int headerIndex = -1;
        
        // setup a list of left justified values
        QStringList values {
            QString::number(stats.sendRate * PPS_TO_MBPS).rightJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.estimatedBandwith * PPS_TO_MBPS).rightJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.rtt / USECS_PER_MSEC, 'f', 2).rightJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.congestionWindowSize).rightJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.packetSendPeriod).rightJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.events[udt::ConnectionStats::Stats::ReceivedACK]).rightJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.events[udt::ConnectionStats::Stats::ProcessedACK]).rightJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.sentPackets).rightJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.retransmittedPackets).rightJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size())
        };
        
        // output this line of values
        qDebug() << qPrintable(values.join(" | "));
    } else {
        if (first) {
            // output the headers for stats for our table
            qDebug() << qPrintable(SERVER_STATS_TABLE_HEADERS.join(" | "));
            first = false;
        }
        
        auto sockets = _socket.getConnectionSockAddrs();
        if (sockets.size() > 0) {
            udt::ConnectionStats::Stats stats = _socket.sampleStatsForConnection(sockets.front());
            
            int headerIndex = -1;
            
            double megabitsPerSecond = (stats.receivedBytes * MEGABITS_PER_BYTE * MS_PER_SECOND) / _statsInterval;
            
            // setup a list of left justified values
            QStringList values {
                QString::number(megabitsPerSecond, 'f', 2).rightJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.receiveRate * PPS_TO_MBPS).rightJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.estimatedBandwith * PPS_TO_MBPS).rightJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.rtt / USECS_PER_MSEC, 'f', 2).rightJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.congestionWindowSize).rightJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.events[udt::ConnectionStats::Stats::SentACK]).rightJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.events[udt::ConnectionStats::Stats::Duplicate]).rightJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size())
            };
            
            // output this line of values
            qDebug() << qPrintable(values.join(" | "));
        }
    }
}
