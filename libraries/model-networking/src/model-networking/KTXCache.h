//
//  KTXCache.h
//  libraries/model-networking/src
//
//  Created by Zach Pomerantz 2/22/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_KTXCache_h
#define hifi_KTXCache_h

#include <QUrl>

#include <shared/FileCache.h>

namespace ktx {
    class KTX;
}

class KTXFile;
using KTXFilePointer = std::shared_ptr<KTXFile>;

class KTXCache : public cache::FileCache {
    Q_OBJECT

public:
    // Whenever a change is made to the serialized format for the KTX cache that isn't backward compatible,
    // this value should be incremented.  This will force the KTX cache to be wiped
    static const int CURRENT_VERSION;
    static const int INVALID_VERSION;
    static const char* SETTING_VERSION_NAME;

    KTXCache(const std::string& dir, const std::string& ext);

    KTXFilePointer writeFile(const char* data, Metadata&& metadata);
    KTXFilePointer getFile(const Key& key);

protected:
    std::unique_ptr<cache::File> createFile(Metadata&& metadata, const std::string& filepath) override final;
};

class KTXFile : public cache::File {
    Q_OBJECT

protected:
    friend class KTXCache;

    KTXFile(Metadata&& metadata, const std::string& filepath);
};

#endif // hifi_KTXCache_h
