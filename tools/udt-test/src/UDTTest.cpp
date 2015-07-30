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

UDTTest::UDTTest(int& argc, char** argv) :
    QCoreApplication(argc, argv)
{
    parseArguments();
    
    _socket.bind(QHostAddress::LocalHost, _argumentParser.value(PORT_OPTION).toUInt());
    qDebug() << "Test socket is listening on" << _socket.localPort();
    
    if (_argumentParser.isSet(TARGET_OPTION)) {
        // parse the IP and port combination for this target
        QString hostnamePortString = _argumentParser.value(TARGET_OPTION);
        
        QHostAddress address { hostnamePortString.left(hostnamePortString.indexOf(':')) };
        quint16 port { (quint16) hostnamePortString.right(hostnamePortString.indexOf(':') + 1).toUInt() };
        
        if (address.isNull() || port == 0) {
            qCritical() << "Could not parse an IP address and port combination from" << hostnamePortString << "-" <<
                "The parsed IP was" << address.toString() << "and the parsed port was" << port;
            
            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
        } else {
            _target = HifiSockAddr(address, port);
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
            
            // if we don't have a min packet size we should make it zero, because we have a max
            if (customMinSize) {
                _minPacketSize = 0;
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
