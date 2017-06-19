//
//  Created by Bradley Austin Davis on 2017/06/06
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <mutex>

#include <QtCore/QCoreApplication>

#include <NodeList.h>
#include <AccountManager.h>
#include <AddressManager.h>
#include <MessagesClient.h>

#include <BuildInfo.h>


class Qt59TestApp : public QCoreApplication {
    Q_OBJECT
public:
    Qt59TestApp(int argc, char* argv[]);
    ~Qt59TestApp();

private:
    void finish(int exitCode);
};



Qt59TestApp::Qt59TestApp(int argc, char* argv[]) :
    QCoreApplication(argc, argv)
{

    Setting::init();
    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    DependencyManager::set<AccountManager>([&] { return QString("Mozilla/5.0 (HighFidelityACClient)"); });
    DependencyManager::set<AddressManager>();
    DependencyManager::set<NodeList>(NodeType::Agent, 0);
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->startThread();
    auto messagesClient = DependencyManager::set<MessagesClient>();
    messagesClient->startThread();
    QTimer::singleShot(1000, [this] { finish(0); });
}

Qt59TestApp::~Qt59TestApp() {
}


void Qt59TestApp::finish(int exitCode) {
    auto nodeList = DependencyManager::get<NodeList>();

    // send the domain a disconnect packet, force stoppage of domain-server check-ins
    nodeList->getDomainHandler().disconnect();
    nodeList->setIsShuttingDown(true);
    nodeList->getPacketReceiver().setShouldDropPackets(true);

    // remove the NodeList from the DependencyManager
    DependencyManager::destroy<NodeList>();
    QCoreApplication::exit(exitCode);
}


int main(int argc, char * argv[]) {
    QCoreApplication::setApplicationName("Qt59Test");
    QCoreApplication::setOrganizationName(BuildInfo::MODIFIED_ORGANIZATION);
    QCoreApplication::setOrganizationDomain(BuildInfo::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationVersion(BuildInfo::VERSION);

    Qt59TestApp app(argc, argv);

    return app.exec();
}

#include "main.moc"