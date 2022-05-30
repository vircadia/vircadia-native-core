//
//  ClientTraitsHandler.h
//  libraries/vircadia-client/src/internal/avatars
//
//  Created by Nshan G. on 20 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_CLIENTTRAITSHANDLER_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_CLIENTTRAITSHANDLER_H

#include <QObject>

#include <ClientTraitsHandler.h>

namespace vircadia::client
{

    class Avatar;

    class ClientTraitsHandler;

    using ClientTraitsHandlerBase = ::ClientTraitsHandler<ClientTraitsHandler, Avatar*>;

    /// @private
    class ClientTraitsHandler : public QObject, public ClientTraitsHandlerBase  {
        Q_OBJECT
    public:
        using ClientTraitsHandlerBase::ClientTraitsHandler;
    private:
        friend ClientTraitsHandlerBase;
    };

} // namespace vircadia::client

#endif /* end of include guard */
