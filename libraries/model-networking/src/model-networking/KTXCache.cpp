//
//  KTXCache.cpp
//  libraries/model-networking/src
//
//  Created by Zach Pomerantz on 2/22/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "KTXCache.h"

#include <memory>

KTXFilePointer KTXCache::writeFile(Data data) {
    return std::static_pointer_cast<KTXFile>(FileCache::writeFile(data.key, data.data, data.length, (void*)&data));
}

KTXFilePointer KTXCache::getFile(const QUrl& url) {
    Key key;
    {
        Lock lock(_urlMutex);
        const auto it = _urlMap.find(url);
        if (it != _urlMap.cend()) {
            key = it->second;
        }
    }

    KTXFilePointer file;
    if (!key.empty()) {
        file = std::static_pointer_cast<KTXFile>(FileCache::getFile(key));
    }

    return file;
}

File* KTXCache::createFile(const Key& key, const std::string& filepath, size_t length, void* extra) {
    const QUrl& url = reinterpret_cast<Data*>(extra)->url;
    Lock lock(_urlMutex);
    _urlMap[url] = key;
    return new KTXFile(key, filepath, length, url);
}

void KTXCache::evictedFile(const FilePointer& file) {
    const QUrl url = std::static_pointer_cast<KTXFile>(file)->getUrl();
    Lock lock(_urlMutex);
    _urlMap.erase(url);
}
