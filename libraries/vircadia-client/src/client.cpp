//
//  client.cpp
//  libraries/client/src
//
//  Created by Nshan G. on 15 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "client.h"

#include <memory>
#include <vector>
#include <list>
#include <iterator>
#include <string>
#include <array>
#include <atomic>
#include <thread>

#include <iostream>

#include <QCoreApplication>
#include <QThreadPool>

#include <nlohmann/json.hpp>

#include <SharedUtil.h>
#include <DependencyManager.h>
#include <AccountManager.h>
#include <DomainAccountManager.h>
#include <AddressManager.h>
#include <NodeList.h>

#include "version.h"

namespace vircadia { namespace client {

    /// @private
    struct NodeData {
        uint8_t type;
        bool active;
        std::string address;
        std::array<uint8_t, 16> uuid;
    };

    /// @private
    class Context {

    public:
        Context(NodeList::Params nodeListParams, const char* userAgent, vircadia_client_info info) :
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

        bool ready() {
            return app != nullptr;
        }

        ~Context() {
            if (ready()) {
                QMetaObject::invokeMethod(app, "quit");
            }
            appThraed.join();
        }

        const std::vector<NodeData> getNodes() { return nodes; }

        void updateNodes() {
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

        bool isConnected() {
            return DependencyManager::get<NodeList>()->getDomainHandler().isConnected();
        }

        void connect(const QString& address) {
            DependencyManager::get<AddressManager>()->handleLookupString(address, false);
        };

    private:
        std::thread appThraed {};
        std::atomic<QCoreApplication*> app {};
        std::vector<NodeData> nodes {};
        std::promise<void> qtInitialized {};

        int argc = 1;
        char argvData[18] = {"qt_is_such_a_joke"};
        char* argv;
    };

    std::list<Context> contexts;

}}

using namespace vircadia::client;

VIRCADIA_CLIENT_DYN_API
vircadia_context_params vircadia_context_defaults() {
    return {
        -1, -1,
        nullptr,
        nullptr,
        {
            "VircadiaClient",
            "Vircadia",
            "vircadia.com",
            vircadia_client_version()->full
        }
    };
}

VIRCADIA_CLIENT_DYN_API
int vircadia_create_context(vircadia_context_params params) {
    if (!contexts.empty()) {
        return -1;
    }

    nlohmann::json platform_info{};
    if (params.platform_info != nullptr) {
        auto parsed = nlohmann::json::parse(params.platform_info, nullptr, false);
        if (!parsed.is_discarded()) {
            platform_info = parsed;
        }
    }

    int index = contexts.size();
    vircadia::client::contexts.emplace_back(
        NodeList::Params{
            NodeType::Agent,
            {params.listenPort, params.dtlsListenPort},
            platform_info
        },
        params.user_agent,
        params.client_info);
    return index;
}

int vircadia_context_ready(int id) {
    auto begin = std::begin(contexts);
    if ( id < 0 || id >= static_cast<int>(contexts.size()) ) {
        return -1;
    } else {
        return std::next(begin, id)->ready() ? 0 : -2;
    }
}

VIRCADIA_CLIENT_DYN_API
int vircadia_destroy_context(int id) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);
    contexts.erase(context);
    return 0;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_connect(int id, const char* address) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);
    context->connect(address);
    return 0;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_connection_status(int id) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);

    return context->isConnected() ? 1 : 0;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_update_nodes(int id) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);
    context->updateNodes();
    return 0;
}

VIRCADIA_CLIENT_DYN_API
int vircadia_node_count(int id) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return ready;
    }

    auto context = std::next(std::begin(contexts), id);
    return context->getNodes().size();
}

VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_node_uuid(int id, int index) {
    int ready = vircadia_context_ready(id);
    if (ready != 0) {
        return nullptr;
    }

    auto context = std::next(std::begin(contexts), id);
    const auto& nodes = context->getNodes();

    if ( index < 0 || id >= static_cast<int>(nodes.size()) ) {
        return nullptr;
    }

    auto node = std::next(std::begin(nodes), index);
    return node->uuid.data();
}
