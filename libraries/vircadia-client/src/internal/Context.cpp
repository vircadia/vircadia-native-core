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
#include <MessagesClient.h>

#include "Error.h"

namespace vircadia::client {

    UUID toUUIDArray(QUuid from) {
        UUID to{};
        auto rfc4122 = from.toRfc4122();
        assert(rfc4122.size() == to.size());
        std::copy(rfc4122.begin(), rfc4122.end(), to.begin());
        return to;
    }

    Context::Context(NodeList::Params nodeListParams, const char* userAgent, vircadia_client_info info) :
        argc(1),
        argvData("qt_is_such_a_joke"),
        argv(&argvData[0])
    {
        auto qtInitialization = qtInitialized.get_future();
        appThread = std::thread{ [
            &app = this->app,
            &argc = this->argc,
            &argv = this->argv,
            &qtInitialized = this->qtInitialized,
            nodeListParams, userAgent, info
        ] () {
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

                DependencyManager::destroy<MessagesClient>();
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


    template <std::size_t N>
    std::size_t find(std::size_t begin, const std::bitset<N>& flags, bool value) {
        assert(begin < N);
        while (begin != N && flags[begin] != value) {
            ++begin;
        }
        return begin;
    };

    template <typename C, std::size_t N, typename F>
    void forAllSet(std::bitset<N> flags, C& container, F&& func) {
        auto set = find(0, flags, true);
        while (set != N) {
            if (set >= container.size()) {
                break;
            }

            auto& element = *std::next(container.begin(), set);
            std::invoke(std::forward<F>(func), element);

            set = find(set+1, flags, true);
        }
    }

    void Context::enableMessages(std::bitset<8> type) {

        forAllSet(type, messageContexts, [&](auto& messagesContext) {
            if (messagesContext.enabled) {
                return;
            }

            if (!DependencyManager::isSet<MessagesClient>()) {
                DependencyManager::set<MessagesClient>()->startThread();
            }

            auto messagesClient = DependencyManager::get<MessagesClient>();

            auto receiver = [&](QString channel, auto payload, QUuid sender, bool localOnly) {
                QByteArray message;
                if constexpr (std::is_same_v<decltype(payload), QString>) {
                    message = payload.toUtf8();
                } else {
                    message = payload;
                }

                std::scoped_lock lock(messagesContext.mutex);
                messagesContext.messages.push_back({
                    channel,
                    message,
                    sender,
                    localOnly
                });
            };

            if (type[MESSAGE_TYPE_TEXT_INDEX]) { // text message
                QObject::connect(messagesClient.data(), &MessagesClient::messageReceived, receiver);
            } else { // data message
                QObject::connect(messagesClient.data(), &MessagesClient::dataReceived, receiver);
            }

            messagesContext.enabled = true;
        });
    }


    void Context::updateMessages(std::bitset<8> type) {
        forAllSet(type, messageContexts, [&](auto& messagesContext) {
            std::unique_lock<std::mutex> lock(messagesContext.mutex, std::try_to_lock);
            if (lock) {
                auto& messages = messagesContext.messages;
                auto& buffer = messagesContext.buffer;
                std::transform(messages.begin(), messages.end(), std::back_inserter(buffer), [](const auto& message) {
                    return MessageData {
                        message.channel.toStdString(),
                        message.message.toStdString(),
                        toUUIDArray(message.senderUuid),
                        message.localOnly
                    };
                });
                messages.clear();
            }
        });
    }

    void Context::clearMessages(std::bitset<8> type) {
        forAllSet(type, messageContexts, [&](auto& messagesContext) {
            messagesContext.buffer.clear();
        });
    }

    void Context::subscribeMessages(QString channel) const {
        DependencyManager::get<MessagesClient>()->subscribe(channel);
    }

    void Context::unsubscribeMessages(QString channel) const {
        DependencyManager::get<MessagesClient>()->unsubscribe(channel);
    }

    const std::vector<MessageData>& Context::getMessages(std::bitset<8> type) const {
        assert(type.count() == 1);
        const std::vector<MessageData>* ret = nullptr;
        forAllSet(type, messageContexts, [&](auto& messagesContext) {
            ret = &messagesContext.buffer;
        });
        return *ret;
    }

    bool Context::isMessagesEnabled(std::bitset<8> type) const {
        bool ret = true;
        forAllSet(type, messageContexts, [&](auto& messagesContext) {
            ret &= messagesContext.enabled;
        });
        return ret;
    }

    void Context::sendMessage(std::bitset<8> type, QString channel, QByteArray payload, bool localOnly) {
        auto client = DependencyManager::get<MessagesClient>();

        if (type[MESSAGE_TYPE_TEXT_INDEX]) {
            client->sendMessage(channel, QString::fromUtf8(payload), localOnly);
        }

        if (type[MESSAGE_TYPE_DATA_INDEX]) {
            client->sendData(channel, payload, localOnly);
        }
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
