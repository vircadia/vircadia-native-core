//
//  ACClientApp.cpp
//  tools/ac-client/src
//
//  Created by Seth Alves on 2016-10-5
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDataStream>
#include <QThread>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <NetworkLogging.h>
#include <SharedLogging.h>
#include <AddressManager.h>
#include <DependencyManager.h>
#include <SettingHandle.h>

#include "ACClientApp.h"

ACClientApp::ACClientApp(int argc, char* argv[]) :
    QCoreApplication(argc, argv)
{
    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity AC client");
    parser.addHelpOption();

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption verboseOutput("v", "verbose output");
    parser.addOption(verboseOutput);

    const QCommandLineOption domainAddressOption("d", "domain-server address", "127.0.0.1");
    parser.addOption(domainAddressOption);

    const QCommandLineOption cacheSTUNOption("s", "cache stun-server response");
    parser.addOption(cacheSTUNOption);

    const QCommandLineOption listenPortOption("listenPort", "listen port", QString::number(INVALID_PORT));
    parser.addOption(listenPortOption);


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
    if (!_verbose) {
        QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");

        const_cast<QLoggingCategory*>(&networking())->setEnabled(QtDebugMsg, false);
        const_cast<QLoggingCategory*>(&networking())->setEnabled(QtInfoMsg, false);
        const_cast<QLoggingCategory*>(&networking())->setEnabled(QtWarningMsg, false);

        const_cast<QLoggingCategory*>(&shared())->setEnabled(QtDebugMsg, false);
        const_cast<QLoggingCategory*>(&shared())->setEnabled(QtInfoMsg, false);
        const_cast<QLoggingCategory*>(&shared())->setEnabled(QtWarningMsg, false);
    }

    // QString domainServerAddress = "127.0.0.1";
    // if (parser.isSet(domainAddressOption)) {
    //     // parse the IP and port combination for this target
    //     QString hostnamePortString = parser.value(domainAddressOption);

    //     QHostAddress address { hostnamePortString.left(hostnamePortString.indexOf(':')) };
    //     quint16 port { (quint16) hostnamePortString.mid(hostnamePortString.indexOf(':') + 1).toUInt() };
    //     if (port == 0) {
    //         port = DEFAULT_DOMAIN_SERVER_PORT;
    //     }

    //     if (address.isNull()) {
    //         qCritical() << "Could not parse an IP address and port combination from" << hostnamePortString << "-" <<
    //             "The parsed IP was" << address.toString() << "and the parsed port was" << port;

    //         QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
    //     } else {
    //         _iceServerAddr = HifiSockAddr(address, port);
    //     }

    //     if (_verbose) {
    //         qDebug() << "domain-server Address is" << domainServerAddress << "port is" << port;
    //     }
    // }

    QString domainServerAddress = "127.0.0.1:40103";
    if (parser.isSet(domainAddressOption)) {
        domainServerAddress = parser.value(domainAddressOption);
    }

    if (_verbose) {
        qDebug() << "domain-server address is" << domainServerAddress;
    }

    int listenPort = INVALID_PORT;
    if (parser.isSet(listenPortOption)) {
        listenPort = parser.value(listenPortOption).toInt();
    }

    Setting::preInit();
    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    // DependencyManager::registerInheritance<AvatarHashMap, AvatarManager>();
    Setting::init();

    DependencyManager::set<AccountManager>([&]{ return QString("Mozilla/5.0 (HighFidelityACClient)"); });
    DependencyManager::set<AddressManager>();
    DependencyManager::set<NodeList>(NodeType::Agent, listenPort);


    auto nodeList = DependencyManager::get<NodeList>();

    // start the nodeThread so its event loop is running
    QThread* nodeThread = new QThread(this);
    nodeThread->setObjectName("NodeList Thread");
    nodeThread->start();

    // make sure the node thread is given highest priority
    nodeThread->setPriority(QThread::TimeCriticalPriority);

    // setup a timer for domain-server check ins
    QTimer* domainCheckInTimer = new QTimer(nodeList.data());
    connect(domainCheckInTimer, &QTimer::timeout, nodeList.data(), &NodeList::sendDomainServerCheckIn);
    domainCheckInTimer->start(DOMAIN_SERVER_CHECK_IN_MSECS);

    // put the NodeList and datagram processing on the node thread
    nodeList->moveToThread(nodeThread);


    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    connect(&domainHandler, SIGNAL(hostnameChanged(const QString&)), SLOT(domainChanged(const QString&)));
    // connect(&domainHandler, SIGNAL(resetting()), SLOT(resettingDomain()));
    // connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(updateWindowTitle()));
    // connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(updateWindowTitle()));
    // connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(clearDomainOctreeDetails()));
    connect(&domainHandler, &DomainHandler::domainConnectionRefused, this, &ACClientApp::domainConnectionRefused);

    connect(nodeList.data(), &NodeList::nodeAdded, this, &ACClientApp::nodeAdded);
    connect(nodeList.data(), &NodeList::nodeKilled, this, &ACClientApp::nodeKilled);
    connect(nodeList.data(), &NodeList::nodeActivated, this, &ACClientApp::nodeActivated);
    // connect(nodeList.data(), &NodeList::uuidChanged, getMyAvatar(), &MyAvatar::setSessionUUID);
    // connect(nodeList.data(), &NodeList::uuidChanged, this, &ACClientApp::setSessionUUID);
    // connect(nodeList.data(), &NodeList::packetVersionMismatch, this, &ACClientApp::notifyPacketVersionMismatch);

    // // you might think we could just do this in NodeList but we only want this connection for Interface
    // connect(nodeList.data(), &NodeList::limitOfSilentDomainCheckInsReached, nodeList.data(), &NodeList::reset);

    // AccountManager _accountManager = new AccountManager(std::bind(&ACClientApp::getUserAgent, qApp));

    // setState(lookUpStunServer);


    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
                                                 << NodeType::EntityServer << NodeType::AssetServer << NodeType::MessagesMixer);


    // // send the identity packet for our avatar each second to our avatar mixer
    // connect(&identityPacketTimer, &QTimer::timeout, getMyAvatar(), &MyAvatar::sendIdentityPacket);
    // identityPacketTimer.start(AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS);


    // DependencyManager::get<AddressManager>()->loadSettings(domainServerAddress);

    DependencyManager::get<AddressManager>()->handleLookupString(domainServerAddress, false);

    QTimer* doTimer = new QTimer(this);
    doTimer->setSingleShot(true);
    connect(doTimer, &QTimer::timeout, this, &ACClientApp::timedOut);
    doTimer->start(4000);
}

ACClientApp::~ACClientApp() {
}


void ACClientApp::domainConnectionRefused(const QString& reasonMessage, int reasonCodeInt, const QString& extraInfo) {
    qDebug() << "domainConnectionRefused";
}

void ACClientApp::domainChanged(const QString& domainHostname) {
    if (_verbose) {
        qDebug() << "domainChanged";
    }
}

void ACClientApp::nodeAdded(SharedNodePointer node) {
    if (_verbose) {
        qDebug() << "node added: " << node->getType();
    }
}

void ACClientApp::nodeActivated(SharedNodePointer node) {
    if (node->getType() == NodeType::EntityServer) {
        if (_verbose) {
            qDebug() << "saw EntityServer";
        }
        _sawEntityServer = true;
    }
    else if (node->getType() == NodeType::AudioMixer) {
        if (_verbose) {
            qDebug() << "saw AudioMixer";
        }
        _sawAudioMixer = true;
    }
    else if (node->getType() == NodeType::AvatarMixer) {
        if (_verbose) {
            qDebug() << "saw AvatarMixer";
        }
        _sawAvatarMixer = true;
    }
    else if (node->getType() == NodeType::AssetServer) {
        if (_verbose) {
            qDebug() << "saw AssetServer";
        }
        _sawAssetServer = true;
    }
    else if (node->getType() == NodeType::MessagesMixer) {
        if (_verbose) {
            qDebug() << "saw MessagesMixer";
        }
        _sawMessagesMixer = true;
    }

    if (_sawEntityServer && _sawAudioMixer && _sawAvatarMixer && _sawAssetServer && _sawMessagesMixer) {
        if (_verbose) {
            qDebug() << "success";
        }
        finish(0);
    }
}

void ACClientApp::nodeKilled(SharedNodePointer node) {
    qDebug() << "nodeKilled";
}

void ACClientApp::timedOut() {
    if (_verbose) {
        qDebug() << "timed out: " << _sawEntityServer << _sawAudioMixer <<
            _sawAvatarMixer << _sawAssetServer << _sawMessagesMixer;
    }
    finish(1);
}

void ACClientApp::finish(int exitCode) {
    auto nodeList = DependencyManager::get<NodeList>();

    // send the domain a disconnect packet, force stoppage of domain-server check-ins
    nodeList->getDomainHandler().disconnect();
    nodeList->setIsShuttingDown(true);

    // tell the packet receiver we're shutting down, so it can drop packets
    nodeList->getPacketReceiver().setShouldDropPackets(true);

    QThread* nodeThread = DependencyManager::get<NodeList>()->thread();
    // remove the NodeList from the DependencyManager
    DependencyManager::destroy<NodeList>();
    // ask the node thread to quit and wait until it is done
    nodeThread->quit();
    nodeThread->wait();

    QCoreApplication::exit(exitCode);
}
