//
//  ExtendedIODevice.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 8/7/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ExtendedIODevice_h
#define hifi_ExtendedIODevice_h

#include <QtCore/QIODevice>

class ExtendedIODevice : public QIODevice {
public:
    ExtendedIODevice(QObject* parent = nullptr) : QIODevice(parent) {};

    template<typename T> qint64 peekPrimitive(T* data);
    template<typename T> qint64 readPrimitive(T* data);
    template<typename T> qint64 writePrimitive(const T& data);
};

template<typename T> qint64 ExtendedIODevice::peekPrimitive(T* data) {
    return peek(reinterpret_cast<char*>(data), sizeof(T));
}

template<typename T> qint64 ExtendedIODevice::readPrimitive(T* data) {
    return read(reinterpret_cast<char*>(data), sizeof(T));
}

template<typename T> qint64 ExtendedIODevice::writePrimitive(const T& data) {
    static_assert(!std::is_pointer<T>::value, "T must not be a pointer");
    return write(reinterpret_cast<const char*>(&data), sizeof(T));
}

#endif // hifi_ExtendedIODevice_h
