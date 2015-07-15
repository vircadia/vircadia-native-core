//
//  NetworkPacket.h
//  libraries/networking/src
//
//  Created by Brad Hefta-Gaub on 8/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  A really simple class that stores a network packet between being received and being processed
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NetworkPacket_h
#define hifi_NetworkPacket_h

#include "NodeList.h"

/// Storage of not-yet processed inbound, or not yet sent outbound generic UDP network packet
class NetworkPacket {
public:
    NetworkPacket() { }
    NetworkPacket(const NetworkPacket& packet); // copy constructor
    NetworkPacket& operator= (const NetworkPacket& other);    // copy assignment
    NetworkPacket(const SharedNodePointer& node, const QByteArray& byteArray);

    const SharedNodePointer& getNode() const { return _node; }
    const QByteArray& getByteArray() const { return _byteArray; }

private:
    void copyContents(const SharedNodePointer& node, const QByteArray& byteArray);

    SharedNodePointer _node;
    QByteArray _byteArray;
};

#endif // hifi_NetworkPacket_h
