//
//  PacketList.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/06/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketList_h
#define hifi_PacketList_h

#pragma once

template <class T> class PacketList : public QIODevice {
public:
    PacketList(PacketType::Value type, bool isOrdered = false);
protected:
    virtual qint64 writeData(const char* data, qint64 maxSize);
    virtual qint64 readData(const char* data, qint64 maxSize);
private:
    std::list<std::unique_ptr<T>> _packets;
}

#endif // hifi_PacketList_h
