//
//  Context.h
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 27 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_CONTEXT_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_CONTEXT_H

#include <vector>
#include <string>
#include <array>
#include <bitset>
#include <thread>
#include <atomic>
#include <future>
#include <mutex>

#include <NodeList.h>

#include "../context.h"

class QCoreApplication;
class QString;

namespace vircadia::client {

    using UUID = std::array<uint8_t, 16>;

    /// @private
    struct NodeData {
        uint8_t type;
        bool active;
        std::string address;
        UUID uuid;
    };

    // @private
    enum MessageTypeIndex : uint8_t {
        MESSAGE_TYPE_TEXT_INDEX,
        MESSAGE_TYPE_DATA_INDEX,
    };

    // @private
    enum MessageType : uint8_t {
        MESSAGE_TYPE_TEXT = 1ul << MESSAGE_TYPE_TEXT_INDEX,
        MESSAGE_TYPE_DATA = 1ul << MESSAGE_TYPE_DATA_INDEX,
        MESSAGE_TYPE_ANY = MESSAGE_TYPE_TEXT | MESSAGE_TYPE_DATA
    };

    /// @private
    struct MessageData {
        std::string channel;
        std::string payload;
        UUID sender;
        bool localOnly;
    };

    /// @private
    struct MessageParams {
        QString channel;
        QByteArray message;
        QUuid senderUuid;
        bool localOnly;
    };

    /// @private
    class Context {

    public:
        Context(NodeList::Params nodeListParams, const char* userAgent, vircadia_client_info info);

        bool ready() const;

        ~Context();

        const std::vector<NodeData>& getNodes() const;
        void updateNodes();

        bool isConnected() const;

        void connect(const QString& address) const;

        void enableMessages(std::bitset<8> type);
        void updateMessages(std::bitset<8> type);
        void clearMessages(std::bitset<8> type);
        void subscribeMessages(QString channel) const;
        void unsubscribeMessages(QString channel) const;
        const std::vector<MessageData>& getMessages(std::bitset<8> type) const;
        bool getMessagesEnabled(std::bitset<8> type) const;
        void sendMessage(std::bitset<8> type, QString channel, QByteArray payload, bool localOnly);

    private:
        std::thread appThread {};
        std::atomic<QCoreApplication*> app {};
        std::vector<NodeData> nodes {};
        std::promise<void> qtInitialized {};


        struct MessagesContext {
            bool enabled {false};
            std::vector<MessageParams> messages {};
            std::vector<MessageData> buffer {};
            mutable std::mutex mutex {};
        };

        std::array<MessagesContext, 2> messageContexts;

        int argc;
        char argvData[18];
        char* argv;
    };

    extern std::list<Context> contexts;

    int vircadiaContextValid(int id);
    int vircadiaContextReady(int id);

} // namespace vircadia::client

#endif /* end of include guard */
