//
//  UDTTest.cpp
//  tools/udt-test/src
//
//  Created by Stephen Birarda on 2015-07-30.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UDTTest.h"

#include <QtCore/QDebug>

#include <udt/Constants.h>
#include <udt/Packet.h>

#include <LogHandler.h>

const QCommandLineOption PORT_OPTION { "p", "listening port for socket (defaults to random)", "port", 0 };
const QCommandLineOption TARGET_OPTION {
    "target", "target for sent packets (default is listen only)",
    "IP:PORT or HOSTNAME:PORT"
};
const QCommandLineOption PACKET_SIZE {
    "packet-size", "size for sent packets in bytes (defaults to 1500)", "bytes",
    QString(udt::MAX_PACKET_SIZE_WITH_UDP_HEADER)
};
const QCommandLineOption MIN_PACKET_SIZE {
    "min-packet-size", "min size for sent packets in bytes", "min-bytes"
};
const QCommandLineOption MAX_PACKET_SIZE {
    "max-packet-size", "max size for sent packets in bytes", "max-bytes"
};
const QCommandLineOption MAX_SEND_BYTES {
    "max-send-bytes", "number of bytes to send before stopping (default is infinite)", "max-bytes"
};
const QCommandLineOption MAX_SEND_PACKETS {
    "max-send-packets", "number of packets to send before stopping (default is infinite)", "max-packets"
};
const QCommandLineOption UNRELIABLE_PACKETS {
    "unreliable", "send unreliable packets (default is reliable)"
};

const QStringList CLIENT_STATS_TABLE_HEADERS {
    "Send Rate (P/s)", "Bandwidth (P/s)", "RTT(ms)", "CW (P)", "Send Period (us)",
    "Received ACK", "Processed ACK", "Received LACK", "Received NAK", "Received TNAK",
    "Sent ACK2", "Sent Packets", "Re-sent Packets"
};

const QStringList SERVER_STATS_TABLE_HEADERS {
    "Receive Rate (P/s)", "Bandwidth (P/s)", "RTT(ms)", "CW (P)",
    //"Total Bytes", "Util Bytes", "Ratio (%)",
    "Sent ACK", "Sent LACK", "Sent NAK", "Sent TNAK",
    "Recieved ACK2", "Duplicate Packets"
};

UDTTest::UDTTest(int& argc, char** argv) :
    QCoreApplication(argc, argv)
{
    qInstallMessageHandler(LogHandler::verboseMessageHandler);
    
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
            _target = HifiSockAddr(address, port);
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
    
    if (!_target.isNull()) {
        sendInitialPackets();
    }
    
    // the sender reports stats every 100 milliseconds
    static const int STATS_SAMPLE_INTERVAL = 100;
    
    QTimer* statsTimer = new QTimer(this);
    connect(statsTimer, &QTimer::timeout, this, &UDTTest::sampleStats);
    statsTimer->start(STATS_SAMPLE_INTERVAL);
}

void UDTTest::parseArguments() {
    // use a QCommandLineParser to setup command line arguments and give helpful output
    _argumentParser.setApplicationDescription("High Fidelity UDT Protocol Test Client");
    _argumentParser.addHelpOption();
    
    const QCommandLineOption helpOption = _argumentParser.addHelpOption();
    
    _argumentParser.addOptions({
        PORT_OPTION, TARGET_OPTION, PACKET_SIZE, MIN_PACKET_SIZE, MAX_PACKET_SIZE,
        MAX_SEND_BYTES, MAX_SEND_PACKETS, UNRELIABLE_PACKETS
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

void UDTTest::sampleStats() {
    static bool first = true;
    static const double USECS_PER_MSEC = 1000.0;
    
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
            QString::number(stats.sendRate).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.estimatedBandwith).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.rtt / USECS_PER_MSEC).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.congestionWindowSize).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.packetSendPeriod).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.events[udt::ConnectionStats::Stats::ReceivedACK]).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.events[udt::ConnectionStats::Stats::ProcessedACK]).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.events[udt::ConnectionStats::Stats::ReceivedLightACK]).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.events[udt::ConnectionStats::Stats::ReceivedNAK]).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.events[udt::ConnectionStats::Stats::ReceivedTimeoutNAK]).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.events[udt::ConnectionStats::Stats::SentACK2]).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.sentPackets).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
            QString::number(stats.events[udt::ConnectionStats::Stats::Retransmission]).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size())
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
            
            // setup a list of left justified values
            QStringList values {
                QString::number(stats.receiveRate).leftJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.estimatedBandwith).leftJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.rtt / USECS_PER_MSEC).leftJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.congestionWindowSize).leftJustified(CLIENT_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.events[udt::ConnectionStats::Stats::SentACK]).leftJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.events[udt::ConnectionStats::Stats::SentLightACK]).leftJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.events[udt::ConnectionStats::Stats::SentNAK]).leftJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.events[udt::ConnectionStats::Stats::SentTimeoutNAK]).leftJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.events[udt::ConnectionStats::Stats::ReceivedACK2]).leftJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size()),
                QString::number(stats.events[udt::ConnectionStats::Stats::Duplicate]).leftJustified(SERVER_STATS_TABLE_HEADERS[++headerIndex].size())
            };
            
            // output this line of values
            qDebug() << qPrintable(values.join(" | "));
        }
    }
}
