//
//  SocketType.h
//  libraries/networking/src
//
//  Created by David Rowe on 17 May 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Handles UDP and WebRTC sockets in parallel.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef vircadia_SocketType_h
#define vircadia_SocketType_h

/// @addtogroup Networking
/// @{


/// @brief The types of network socket.
enum class SocketType : uint8_t {
    Unknown,    ///< Socket type unknown or not set.
    UDP,        ///< UDP socket.
    WebRTC      ///< WebRTC socket. A WebRTC data channel presented as a UDP-style socket.
};

class SocketTypeToString {
public:
    /// @brief Returns the name of a SocketType value, e.g., <code>"WebRTC"</code>.
    /// @param socketType The SocketType value.
    /// @return The name of the SocketType value.
    static QString socketTypeToString (SocketType socketType) {
        static QStringList SOCKET_TYPE_STRINGS { "Unknown", "UDP", "WebRTC" };
        return SOCKET_TYPE_STRINGS[(int)socketType];
    }
};

/// @}

#endif // vircadia_SocketType_h
