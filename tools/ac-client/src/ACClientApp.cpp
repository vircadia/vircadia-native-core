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
#include <NetworkingConstants.h>
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

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption verboseOutput("v", "verbose output");
    parser.addOption(verboseOutput);

    const QCommandLineOption authOption("u", "set usename and pass", "username:password");
    parser.addOption(authOption);

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

    if (parser.isSet(authOption)) {
        QStringList pieces = parser.value(authOption).split(":");
        if (pieces.size() != 2) {
            qDebug() << "-u should be followed by username:password";
            parser.showHelp();
            Q_UNREACHABLE();
        }

        _username = pieces[0];
        _password = pieces[1];
    }

    Setting::init();
    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();

    DependencyManager::set<AccountManager>([&]{ return QString("Mozilla/5.0 (HighFidelityACClient)"); });
    DependencyManager::set<AddressManager>();
    DependencyManager::set<NodeList>(NodeType::Agent, listenPort);

    auto accountManager = DependencyManager::get<AccountManager>();
    accountManager->setIsAgent(true);
    accountManager->setAuthURL(NetworkingConstants::METAVERSE_SERVER_URL());

    auto nodeList = DependencyManager::get<NodeList>();

    // setup a timer for domain-server check ins
    QTimer* domainCheckInTimer = new QTimer(nodeList.data());
    connect(domainCheckInTimer, &QTimer::timeout, nodeList.data(), &NodeList::sendDomainServerCheckIn);
    domainCheckInTimer->start(DOMAIN_SERVER_CHECK_IN_MSECS);

    // start the nodeThread so its event loop is running
    // (must happen after the checkin timer is created with the nodelist as it's parent)
    nodeList->startThread();

    const DomainHandler& domainHandler = nodeList->getDomainHandler();
    connect(&domainHandler, SIGNAL(hostnameChanged(const QString&)), SLOT(domainChanged(const QString&)));
    connect(&domainHandler, &DomainHandler::domainConnectionRefused, this, &ACClientApp::domainConnectionRefused);

    connect(nodeList.data(), &NodeList::nodeAdded, this, &ACClientApp::nodeAdded);
    connect(nodeList.data(), &NodeList::nodeKilled, this, &ACClientApp::nodeKilled);
    connect(nodeList.data(), &NodeList::nodeActivated, this, &ACClientApp::nodeActivated);
    connect(nodeList.data(), &NodeList::packetVersionMismatch, this, &ACClientApp::notifyPacketVersionMismatch);
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
                                                 << NodeType::EntityServer << NodeType::AssetServer << NodeType::MessagesMixer);

    if (_verbose) {
        QString username = accountManager->getAccountInfo().getUsername();
        qDebug() << "cached username is" << username << ", isLoggedIn =" << accountManager->isLoggedIn();
    }

    if (!_username.isEmpty()) {

        connect(accountManager.data(), &AccountManager::newKeypair, this, [&](){
            if (_verbose) {
                qDebug() << "new keypair has been created.";
            }
        });

        connect(accountManager.data(), &AccountManager::loginComplete, this, [&](){
            if (_verbose) {
                qDebug() << "login successful";
            }
        });
        connect(accountManager.data(), &AccountManager::loginFailed, this, [&](){
            qDebug() << "login failed.";
        });
        accountManager->requestAccessToken(_username, _password);
    }

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

void ACClientApp::notifyPacketVersionMismatch() {
    if (_verbose) {
        qDebug() << "packet version mismatch";
    }
    finish(1);
}

void ACClientApp::printFailedServers() {
    if (!_sawEntityServer) {
        qDebug() << "EntityServer";
    }
    if (!_sawAudioMixer) {
        qDebug() << "AudioMixer";
    }
    if (!_sawAvatarMixer) {
        qDebug() << "AvatarMixer";
    }
    if (!_sawAssetServer) {
        qDebug() << "AssetServer";
    }
    if (!_sawMessagesMixer) {
        qDebug() << "MessagesMixer";
    }
}

void ACClientApp::finish(int exitCode) {
    auto nodeList = DependencyManager::get<NodeList>();

    // send the domain a disconnect packet, force stoppage of domain-server check-ins
    nodeList->getDomainHandler().disconnect();
    nodeList->setIsShuttingDown(true);

    // tell the packet receiver we're shutting down, so it can drop packets
    nodeList->getPacketReceiver().setShouldDropPackets(true);

    // remove the NodeList from the DependencyManager
    DependencyManager::destroy<NodeList>();

    printFailedServers();
    QCoreApplication::exit(exitCode);
}
