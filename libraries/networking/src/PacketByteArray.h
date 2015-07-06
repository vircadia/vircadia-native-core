//
//  PacketByteArray.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/06/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketByteArray_h
#define hifi_PacketByteArray_h

#pragma once

class PacketByteArray {
public:
    PacketByteArray(char* data, int maxBytes);

    int write(const char* src, int srcBytes, int index = 0);
    int append(const char* src, int srcBytes)
private:
    char* _data;
    int _index = 0;
    int _maxBytes = 0;
};

#endif // hifi_PacketByteArray_h
