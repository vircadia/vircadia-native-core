//
//  Context.cpp
//  libraries/vircadia-client/src/internal
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
#include <array>

#include <QCoreApplication>
#include <QString>
#include <QThreadPool>

#include <SharedUtil.h>
#include <DependencyManager.h>
#include <AccountManager.h>
#include <DomainAccountManager.h>
#include <AddressManager.h>

#include "Error.h"

namespace vircadia::client {

    Context::Context(NodeList::Params nodeListParams, const char* userAgent, vircadia_client_info info) :
        argc(1),
        argvData("qt_is_such_a_joke"),
        argv(&argvData[0]),
        messages_(),
        audio_()
    {
        auto qtInitialization = qtInitialized.get_future();
        appThread = std::thread{ [ this, nodeListParams, userAgent, info ] () {
            setupHifiApplication(info.name, info.organization, info.domain, info.version);

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

            QObject::connect(&qtApp, &QCoreApplication::aboutToQuit, [this](){
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

                messages_.destroy();
                audio_.destroy();
                DependencyManager::destroy<NodeList>();
                DependencyManager::destroy<AddressManager>();
                DependencyManager::destroy<DomainAccountManager>();
                DependencyManager::destroy<AccountManager>();


            });

            app = &qtApp;

            qtInitialized.set_value();

            qtApp.exec();

            app = nullptr;
        } };

        qtInitialization.wait();
    }

    bool Context::ready() const {
        return app != nullptr;
    }

    Context::~Context() {
        if (ready()) {
            QMetaObject::invokeMethod(app, "quit");
        }
        appThread.join();
    }

    const std::vector<NodeData>& Context::getNodes() const {
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

                data.uuid = toUUIDArray(node->getUUID());

                return data;
            });
        });
    }

    bool Context::isConnected() const {
        return DependencyManager::get<NodeList>()->getDomainHandler().isConnected();
    }

    void Context::connect(const QString& address) const {
        DependencyManager::get<AddressManager>()->handleLookupString(address, false);
    }

    Messages& Context::messages() {
        return messages_;
    }

    const Messages& Context::messages() const {
        return messages_;
    }

    Audio& Context::audio() {
        return audio_;
    }

    const Audio& Context::audio() const {
        return audio_;
    }

    std::list<Context> contexts;

    int checkContextValid(int id) {
        return checkIndexValid(contexts, id, ErrorCode::CONTEXT_INVALID);
    }

    int checkContextReady(int id) {
        return chain(checkContextValid(id), [&](auto) {
            return std::next(std::begin(contexts), id)->ready()
                ? 0
                : toInt(ErrorCode::CONTEXT_LOSS);
        });
    }

} // namespace vircadia::client
