//
//  Created by Bradley Austin Davis 2015/07/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_StreamHelpers_h
#define hifi_StreamHelpers_h

#include <QIODevice>
#include <QBuffer>

// Helper functions for reading binary data from an IO device
template<class T>
inline void readStream(QIODevice& in, T& t) {
    in.read((char*) &t, sizeof(t));
}

template<typename T, size_t N>
inline void readStream(QIODevice& in, T (&t)[N]) {
    in.read((char*) t, N);
}

template<class T, size_t N>
inline void fillBuffer(QBuffer& buffer, T (&t)[N]) {
    buffer.setData((const char*) t, N);
}

#endif
