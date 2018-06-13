//
//  OwningBuffer.h
//  shared/src
//
//  Created by Ryan Huffman on 5/31/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OwningBuffer_h
#define hifi_OwningBuffer_h

#include <QBuffer>
class OwningBuffer : public QBuffer {
public:
    OwningBuffer(const QByteArray& content) : _content(content) {
        setData(_content);
    }
    OwningBuffer(QByteArray&& content) : _content(std::move(content)) {
        setData(_content);
    }

private:
    QByteArray _content;
};

#endif // hifi_OwningBuffer_h
