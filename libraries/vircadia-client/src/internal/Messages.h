//
//  Messages.h
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 16 Apr 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_MESSAGES_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_MESSAGES_H

#include <vector>
#include <string>
#include <array>
#include <bitset>
#include <mutex>

#include <QString>
#include <QByteArray>
#include <QUuid>

#include "Common.h"

namespace vircadia::client {

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
    class Messages {

    public:

        void enable(std::bitset<8> type);
        void update(std::bitset<8> type);
        void clear(std::bitset<8> type);
        void subscribe(QString channel) const;
        void unsubscribe(QString channel) const;
        const std::vector<MessageData>& get(std::bitset<8> type) const;
        bool isEnabled(std::bitset<8> type) const;
        int send(std::bitset<8> type, QString channel, QByteArray payload, bool localOnly);

        void destroy();

    private:

        struct MessagesContext {
            bool enabled { false };
            std::vector<MessageParams> messages {};
            std::vector<MessageData> buffer {};
            mutable std::mutex mutex {};
        };

        std::array<MessagesContext, 2> messageContexts;
    };


} // namespace vircadia::client

#endif /* end of include guard */
