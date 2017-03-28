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

#include <FileCache.h>

namespace ktx {
    class KTX;
}

class KTXFile;
using KTXFilePointer = std::shared_ptr<KTXFile>;

class KTXCache : public cache::FileCache {
    Q_OBJECT

public:
    KTXCache(const std::string& dir, const std::string& ext);

    KTXFilePointer writeFile(const char* data, Metadata&& metadata);
    KTXFilePointer getFile(const Key& key);

protected:
    std::unique_ptr<cache::File> createFile(Metadata&& metadata, const std::string& filepath) override final;
};

class KTXFile : public cache::File {
    Q_OBJECT

public:
    std::unique_ptr<ktx::KTX> getKTX() const;

protected:
    friend class KTXCache;

    KTXFile(Metadata&& metadata, const std::string& filepath);
};

#endif // hifi_KTXCache_h
