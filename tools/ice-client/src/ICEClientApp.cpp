//
//  ICEClient.h
//  tools/ice-client/src
//
//  Created by Seth Alves on 3/5/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDataStream>
#include <QCommandLineParser>
#include <PathUtils.h>
#include <LimitedNodeList.h>

#include "ICEClientApp.h"

ICEClientApp::ICEClientApp(int argc, char* argv[]) : QCoreApplication(argc, argv) {
    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity ICE client");
    parser.addHelpOption();

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption verboseOutput("v", "verbose output");
    parser.addOption(verboseOutput);

    const QCommandLineOption iceServerAddressOption("i", "ice-server address", "IP:PORT or HOSTNAME:PORT");
    parser.addOption(iceServerAddressOption);

    const QCommandLineOption howManyTimesOption("n", "how many times to cycle", "0");
    parser.addOption(howManyTimesOption);

    const QCommandLineOption domainIDOption("d", "domain-server uuid");
    parser.addOption(domainIDOption);


    if (!parser.parse(QCoreApplication::arguments())) {
        qCritical() << parser.errorText() << endl;
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (parser.isSet(helpOption)) {
        parser.showHelp();
        Q_UNREACHABLE();
    }

    _verbose = parser.isSet(verboseOutput);

    if (parser.isSet(howManyTimesOption)) {
        _actionMax = parser.value(howManyTimesOption).toInt();
    }

    if (parser.isSet(domainIDOption)) {
        _domainID = QUuid(parser.value(howManyTimesOption));
    }


    _iceServerAddr = HifiSockAddr("127.0.0.1", ICE_SERVER_DEFAULT_PORT);
    if (parser.isSet(iceServerAddressOption)) {
        // parse the IP and port combination for this target
        QString hostnamePortString = parser.value(iceServerAddressOption);

        QHostAddress address { hostnamePortString.left(hostnamePortString.indexOf(':')) };
        quint16 port { (quint16) hostnamePortString.mid(hostnamePortString.indexOf(':') + 1).toUInt() };

        if (address.isNull() || port == 0) {
            qCritical() << "Could not parse an IP address and port combination from" << hostnamePortString << "-" <<
                "The parsed IP was" << address.toString() << "and the parsed port was" << port;

            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
        } else {
            _iceServerAddr = HifiSockAddr(address, port);
        }
    }

    qDebug() << "ICE-server address is" << _iceServerAddr;

    _state = 0;

    QTimer* doTimer = new QTimer(this);
    connect(doTimer, &QTimer::timeout, this, &ICEClientApp::doSomething);
    doTimer->start(200);
}

ICEClientApp::~ICEClientApp() {
    delete _socket;
}

void ICEClientApp::doSomething() {
    if (_actionMax > 0 && _actionCount >= _actionMax) {
        QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
    }

    if (_state == 0) {
        _domainServerPeerSet = false;
        unsigned int localPort = 0;
        _socket = new udt::Socket();
        _socket->bind(QHostAddress::AnyIPv4, localPort);
        _socket->setPacketHandler([this](std::unique_ptr<udt::Packet> packet) { processPacket(std::move(packet));  });

        qDebug() << "local port is" << _socket->localPort();
        _localSockAddr = HifiSockAddr("127.0.0.1", _socket->localPort());
        _publicSockAddr = HifiSockAddr("127.0.0.1", _socket->localPort());

        // QUuid peerID = QUuid("75cd162a-53dc-4292-aaa5-1304ab1bb0f2");
        QUuid peerID;
        if (_domainID == QUuid()) {
            // pick a random domain-id
            peerID = QUuid::createUuid();
            _state = 2;
        } else {
            // use the domain UUID given on the command-line
            peerID = _domainID;
            _state = 1;
        }
        _sessionUUID = QUuid::createUuid();

        sendPacketToIceServer(PacketType::ICEServerQuery, _iceServerAddr, _sessionUUID, peerID);

        _actionCount++;
    } else if (_state == 2) {
        _state = 3;
    } else if (_state == 3) {
        qDebug() << "";
        _state = 0;
        delete _socket;
        _socket = nullptr;
    }
}

void ICEClientApp::sendPacketToIceServer(PacketType packetType, const HifiSockAddr& iceServerSockAddr,
                                         const QUuid& clientID, const QUuid& peerID) {
    std::unique_ptr<NLPacket> icePacket = NLPacket::create(packetType);

    QDataStream iceDataStream(icePacket.get());
    iceDataStream << clientID << _publicSockAddr << _localSockAddr;

    if (packetType == PacketType::ICEServerQuery) {
        assert(!peerID.isNull());

        iceDataStream << peerID;

        qDebug() << "Sending packet to ICE server to request connection info for peer with ID"
                 << uuidStringWithoutCurlyBraces(peerID);
    }

    // fillPacketHeader(packet, connectionSecret);
    _socket->writePacket(*icePacket, _iceServerAddr);
}

void ICEClientApp::icePingDomainServer() {
    if (!_domainServerPeerSet) {
        return;
    }

    qDebug() << "ice-pinging domain-server";

    auto localPingPacket = LimitedNodeList::constructICEPingPacket(PingType::Local, _sessionUUID);
    _socket->writePacket(*localPingPacket, _domainServerPeer.getLocalSocket());

    auto publicPingPacket = LimitedNodeList::constructICEPingPacket(PingType::Public, _sessionUUID);
    _socket->writePacket(*publicPingPacket, _domainServerPeer.getPublicSocket());
}


void ICEClientApp::processPacket(std::unique_ptr<udt::Packet> packet) {
    auto nlPacket = NLPacket::fromBase(std::move(packet));

    if (nlPacket->getPayloadSize() < NLPacket::localHeaderSize(PacketType::ICEServerHeartbeat)) {
        qDebug() << "got a short packet.";
        return;
    }

    QSharedPointer<ReceivedMessage> message = QSharedPointer<ReceivedMessage>::create(*nlPacket);
    const HifiSockAddr& senderAddr = message->getSenderSockAddr();

    if (nlPacket->getType() == PacketType::ICEServerPeerInformation) {
        QDataStream iceResponseStream(message->getMessage());
        iceResponseStream >> _domainServerPeer;
        _domainServerPeerSet = true;

        icePingDomainServer();
        _pingDomainTimer = new QTimer(this);
        connect(_pingDomainTimer, &QTimer::timeout, this, &ICEClientApp::icePingDomainServer);
        _pingDomainTimer->start(1000);

        qDebug() << "got ICEServerPeerInformation from" << _domainServerPeer.getUUID();

    } else if (nlPacket->getType() == PacketType::ICEPing) {
        qDebug() << "got packet: " << nlPacket->getType();
        auto replyPacket = LimitedNodeList::constructICEPingReplyPacket(*message, _sessionUUID);
        _socket->writePacket(*replyPacket, senderAddr);

    } else if (nlPacket->getType() == PacketType::ICEPingReply) {
        qDebug() << "got packet: " << nlPacket->getType();
        if (_domainServerPeerSet && _state == 1 &&
            (senderAddr == _domainServerPeer.getLocalSocket() ||
             senderAddr == _domainServerPeer.getPublicSocket())) {

            delete _pingDomainTimer;
            _pingDomainTimer = nullptr;

            _state = 2;
        }

    } else {
        qDebug() << "got unexpected packet: " << nlPacket->getType();
    }
}
