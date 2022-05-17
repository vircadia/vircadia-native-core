//
//  Messages.cpp
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 16 Apr 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Messages.h"

#include <QString>
#include <QByteArray>
#include <QUuid>
#include <MessagesClient.h>

namespace vircadia::client
{

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

    void Messages::enable(std::bitset<8> type) {

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

    void Messages::update(std::bitset<8> type) {
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

    void Messages::clear(std::bitset<8> type) {
        forAllSet(type, messageContexts, [&](auto& messagesContext) {
            messagesContext.buffer.clear();
        });
    }

    void Messages::subscribe(QString channel) const {
        DependencyManager::get<MessagesClient>()->subscribe(channel);
    }

    void Messages::unsubscribe(QString channel) const {
        DependencyManager::get<MessagesClient>()->unsubscribe(channel);
    }

    const std::vector<MessageData>& Messages::get(std::bitset<8> type) const {
        assert(type.count() == 1);
        const std::vector<MessageData>* ret = nullptr;
        forAllSet(type, messageContexts, [&](auto& messagesContext) {
            ret = &messagesContext.buffer;
        });
        return *ret;
    }

    bool Messages::isEnabled(std::bitset<8> type) const {
        bool ret = true;
        forAllSet(type, messageContexts, [&](auto& messagesContext) {
            ret &= messagesContext.enabled;
        });
        return ret;
    }

    int Messages::send(std::bitset<8> type, QString channel, QByteArray payload, bool localOnly) {
        auto client = DependencyManager::get<MessagesClient>();

        if (type[MESSAGE_TYPE_TEXT_INDEX]) {
            return client->sendMessage(channel, QString::fromUtf8(payload), localOnly);
        }

        if (type[MESSAGE_TYPE_DATA_INDEX]) {
            return client->sendData(channel, payload, localOnly);
        }

        return -1;
    }

    void Messages::destroy() {
        if (DependencyManager::isSet<MessagesClient>()) {
            DependencyManager::destroy<MessagesClient>();
        }
    }

} // namespace vircadia::client
