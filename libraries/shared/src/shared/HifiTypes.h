//
//  HifiTypes.h
//  libraries/shared/src/shared
//
//  Created by Sabrina Shanman on 2018/11/12.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HifiTypes_h
#define hifi_HifiTypes_h

#include <QVarLengthArray>
#include <QVariant>
#include <QUrl>
#include <QMultiHash>
#include <QMultiMap>

namespace hifi {
    using ByteArray = QByteArray;
    using VariantHash = QVariantHash;
    using VariantMultiHash = QMultiHash<QString, QVariant>;
    using VariantMultiMap = QMultiMap<QString, QVariant>;
    using URL = QUrl;
};

#endif // hifi_HifiTypes_h
