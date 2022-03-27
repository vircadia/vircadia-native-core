//
//  Context.cpp
//  libraries/client/src/internal
//
//  Created by Nshan G. on 27 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Context.h"

#include <algorithm>
#include <iterator>

#include <QCoreApplication>
#include <QString>
#include <QThreadPool>

#include <SharedUtil.h>
#include <DependencyManager.h>
#include <AccountManager.h>
#include <DomainAccountManager.h>
#include <AddressManager.h>

namespace vircadia { namespace client {

    Context::Context(NodeList::Params nodeListParams, const char* userAgent, vircadia_client_info info) :
        argc(1),
        argvData("qt_is_such_a_joke"),
        argv(&argvData[0])
    {
        auto qtInitialization = qtInitialized.get_future();
        appThraed = std::thread{ [
            &app = this->app,
            &argc = this->argc,
            &argv = this->argv,
            &qtInitialized = this->qtInitialized,
            nodeListParams, userAgent, info
        ] () {
            setupHifiApplication(info.name, info.organization, info.domain, info.version);
            Setting::init();

            QCoreApplication qtApp{argc, &argv};

            DependencyManager::registerInheritance<LimitedNodeList, NodeList>();

            if (userAgent != nullptr) {
                DependencyManager::set<AccountManager>(false,
                    [userAgentString = QString(userAgent)]
                    () { return userAgentString; });
            } else {
                DependencyManager::set<AccountManager>(false);
            }
            DependencyManager::set<DomainAccountManager>();
            DependencyManager::set<AddressManager>();
            DependencyManager::set<NodeList>(nodeListParams);

            {
                auto accountManager = DependencyManager::get<AccountManager>();
                accountManager->setIsAgent(true);
                accountManager->setAuthURL(MetaverseAPI::getCurrentMetaverseServerURL());
            }

            {
                auto nodeList = DependencyManager::get<NodeList>();

                // setup a timer for domain-server check ins
                QTimer* domainCheckInTimer = new QTimer(nodeList.data());
                QObject::connect(domainCheckInTimer, &QTimer::timeout, nodeList.data(), &NodeList::sendDomainServerCheckIn);
                domainCheckInTimer->start(DOMAIN_SERVER_CHECK_IN_MSECS);

                // start the nodeThread so its event loop is running
                // (must happen after the checkin timer is created with the nodelist as it's parent)
                nodeList->startThread();

                // TODO: parameterize
                nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
                     << NodeType::EntityServer << NodeType::AssetServer << NodeType::MessagesMixer);
            }

            // TODO: account manager login

            QObject::connect(&qtApp, &QCoreApplication::aboutToQuit, [](){
                DependencyManager::prepareToExit();

                {
                    auto nodeList = DependencyManager::get<NodeList>();

                    // send the domain a disconnect packet, force stoppage of domain-server check-ins
                    nodeList->getDomainHandler().disconnect("Quit");
                    nodeList->setIsShuttingDown(true);

                    // tell the packet receiver we're shutting down, so it can drop packets
                    nodeList->getPacketReceiver().setShouldDropPackets(true);
                }

                QThreadPool::globalInstance()->clear();
                QThreadPool::globalInstance()->waitForDone();

                // TODO: these are still being used after this point, so need a better way to make sure all threads and pending operations have finished before destroying
                // DependencyManager::destroy<NodeList>();
                // DependencyManager::destroy<AddressManager>();
                // DependencyManager::destroy<DomainAccountManager>();
                // DependencyManager::destroy<AccountManager>();

            });

            app = &qtApp;

            qtInitialized.set_value();

            qtApp.exec();

            app = nullptr;
        } };

        qtInitialization.wait();
    }

    bool Context::ready() {
        return app != nullptr;
    }

    Context::~Context() {
        if (ready()) {
            QMetaObject::invokeMethod(app, "quit");
        }
        appThraed.join();
    }

    const std::vector<NodeData> Context::getNodes() {
        return nodes;
    }

    void Context::updateNodes() {
        nodes.clear();
        DependencyManager::get<NodeList>()->nestedEach([this](auto begin, auto end) {
            std::transform(begin, end, std::back_inserter(nodes), [](const auto& node) {
                NodeData data{};
                data.type = node->getType();

                auto activeSocket = node->getActiveSocket();
                data.active = activeSocket != nullptr;
                data.address = activeSocket != nullptr ? activeSocket->toString().toStdString() : "";

                auto rfc4122 = node->getUUID().toRfc4122();
                assert(rfc4122.size() == data.uuid.size());
                std::copy(rfc4122.begin(), rfc4122.end(), data.uuid.begin());

                return data;
            });
        });
    }

    bool Context::isConnected() {
        return DependencyManager::get<NodeList>()->getDomainHandler().isConnected();
    }

    void Context::connect(const QString& address) {
        DependencyManager::get<AddressManager>()->handleLookupString(address, false);
    }

}} // namespace vircadia::client
