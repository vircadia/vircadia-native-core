//
//  ATPGetApp.cpp
//  tools/atp-get/src
//
//  Created by Seth Alves on 2017-3-15
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDataStream>
#include <QTextStream>
#include <QThread>
#include <QFile>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <NetworkLogging.h>
#include <SharedLogging.h>
#include <AddressManager.h>
#include <DependencyManager.h>
#include <SettingHandle.h>

#include "ATPGetApp.h"

ATPGetApp::ATPGetApp(int argc, char* argv[]) :
    QCoreApplication(argc, argv)
{
    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity ATP-Get");

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


    QStringList filenames = parser.positionalArguments();
    if (filenames.empty() || filenames.size() > 2) {
        qDebug() << "give remote url and optional local filename as arguments";
        parser.showHelp();
        Q_UNREACHABLE();
    }

    _url = QUrl(filenames[0]);
    if (_url.scheme() != "atp") {
        qDebug() << "url should start with atp:";
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (filenames.size() == 2) {
        _localOutputFile = filenames[1];
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

    Setting::init();
    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();

    DependencyManager::set<AccountManager>([&]{ return QString("Mozilla/5.0 (HighFidelityATPGet)"); });
    DependencyManager::set<AddressManager>();
    DependencyManager::set<NodeList>(NodeType::Agent, listenPort);


    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->startThread();

    // setup a timer for domain-server check ins
    QTimer* domainCheckInTimer = new QTimer(nodeList.data());
    connect(domainCheckInTimer, &QTimer::timeout, nodeList.data(), &NodeList::sendDomainServerCheckIn);
    domainCheckInTimer->start(DOMAIN_SERVER_CHECK_IN_MSECS);

    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    connect(&domainHandler, SIGNAL(hostnameChanged(const QString&)), SLOT(domainChanged(const QString&)));
    // connect(&domainHandler, SIGNAL(resetting()), SLOT(resettingDomain()));
    // connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(clearDomainOctreeDetails()));
    connect(&domainHandler, &DomainHandler::domainConnectionRefused, this, &ATPGetApp::domainConnectionRefused);

    connect(nodeList.data(), &NodeList::nodeAdded, this, &ATPGetApp::nodeAdded);
    connect(nodeList.data(), &NodeList::nodeKilled, this, &ATPGetApp::nodeKilled);
    connect(nodeList.data(), &NodeList::nodeActivated, this, &ATPGetApp::nodeActivated);
    // connect(nodeList.data(), &NodeList::uuidChanged, getMyAvatar(), &MyAvatar::setSessionUUID);
    // connect(nodeList.data(), &NodeList::uuidChanged, this, &ATPGetApp::setSessionUUID);
    connect(nodeList.data(), &NodeList::packetVersionMismatch, this, &ATPGetApp::notifyPacketVersionMismatch);

    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
                                                 << NodeType::EntityServer << NodeType::AssetServer << NodeType::MessagesMixer);

    DependencyManager::get<AddressManager>()->handleLookupString(domainServerAddress, false);

    auto assetClient = DependencyManager::set<AssetClient>();
    assetClient->init();

    QTimer* doTimer = new QTimer(this);
    doTimer->setSingleShot(true);
    connect(doTimer, &QTimer::timeout, this, &ATPGetApp::timedOut);
    doTimer->start(4000);
}

ATPGetApp::~ATPGetApp() {
}


void ATPGetApp::domainConnectionRefused(const QString& reasonMessage, int reasonCodeInt, const QString& extraInfo) {
    qDebug() << "domainConnectionRefused";
}

void ATPGetApp::domainChanged(const QString& domainHostname) {
    if (_verbose) {
        qDebug() << "domainChanged";
    }
}

void ATPGetApp::nodeAdded(SharedNodePointer node) {
    if (_verbose) {
        qDebug() << "node added: " << node->getType();
    }
}

void ATPGetApp::nodeActivated(SharedNodePointer node) {
    if (node->getType() == NodeType::AssetServer) {
        lookup();
    }
}

void ATPGetApp::nodeKilled(SharedNodePointer node) {
    qDebug() << "nodeKilled";
}

void ATPGetApp::timedOut() {
    finish(1);
}

void ATPGetApp::notifyPacketVersionMismatch() {
    if (_verbose) {
        qDebug() << "packet version mismatch";
    }
    finish(1);
}

void ATPGetApp::lookup() {

    auto path = _url.path();
    qDebug() << "path is " << path;

    auto request = DependencyManager::get<AssetClient>()->createGetMappingRequest(path);
    QObject::connect(request, &GetMappingRequest::finished, this, [=](GetMappingRequest* request) mutable {
        auto result = request->getError();
        if (result == GetMappingRequest::NotFound) {
            qDebug() << "not found";
        } else if (result == GetMappingRequest::NoError) {
            qDebug() << "found, hash is " << request->getHash();
            download(request->getHash());
        } else {
            qDebug() << "error -- " << request->getError() << " -- " << request->getErrorString();
        }
        request->deleteLater();
    });
    request->start();
}

void ATPGetApp::download(AssetHash hash) {
    auto assetClient = DependencyManager::get<AssetClient>();
    auto assetRequest = new AssetRequest(hash);

    connect(assetRequest, &AssetRequest::finished, this, [this](AssetRequest* request) mutable {
        Q_ASSERT(request->getState() == AssetRequest::Finished);

        if (request->getError() == AssetRequest::Error::NoError) {
            QString data = QString::fromUtf8(request->getData());
            if (_localOutputFile == "") {
                QTextStream cout(stdout);
                cout << data;
            } else {
                QFile outputHandle(_localOutputFile);
                if (outputHandle.open(QIODevice::ReadWrite)) {
                    QTextStream stream( &outputHandle );
                    stream << data;
                } else {
                    qDebug() << "couldn't open output file:" << _localOutputFile;
                }
            }
        }

        request->deleteLater();
        finish(0);
    });

    assetRequest->start();
}

void ATPGetApp::finish(int exitCode) {
    auto nodeList = DependencyManager::get<NodeList>();

    // send the domain a disconnect packet, force stoppage of domain-server check-ins
    nodeList->getDomainHandler().disconnect();
    nodeList->setIsShuttingDown(true);

    // tell the packet receiver we're shutting down, so it can drop packets
    nodeList->getPacketReceiver().setShouldDropPackets(true);

    // remove the NodeList from the DependencyManager
    DependencyManager::destroy<NodeList>();

    QCoreApplication::exit(exitCode);
}
