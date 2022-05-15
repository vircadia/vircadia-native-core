//
//  AvatarData.h
//  libraries/vircadia-client/src/avatars
//
//  Created by Nshan G. on 9 May 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_AVATARDATA_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_AVATARDATA_H

#include <bitset>

#include <QObject>

#include <AvatarDataStream.h>

namespace vircadia::client
{

    /// @private
    struct AvatarData {

        enum PropertyIndex : size_t {
            IdentityIndex
        };

        QUuid sessionUUID;
        using Properties = std::tuple<AvatarDataPacket::Identity>;
        Properties properties;
        std::bitset<std::tuple_size_v<Properties>> changes;

        template <size_t Index>
        bool setProperty(const std::tuple_element_t<Index, Properties>& value) {
            auto& current = std::get<Index>(properties);
            if (current != value) {
                current = value;
                changes.set(Index);
                return true;
            }
            return false;
        }

    };

    /// @private
    class Avatar : public QObject, public AvatarDataStream<Avatar> {
        Q_OBJECT
        public:
            Avatar();

            const QUuid& getSessionUUID() const;
            void setSessionUUID(const QUuid& uuid);

            AvatarDataPacket::Identity getIdentityDataOut() const;
            bool getIdentityDataChanged();

            AvatarData data;
    };

} // namespace vircadia::client

#endif /* end of include guard */
