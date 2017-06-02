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

#include <SettingHandle.h>
#include <ktx/KTX.h>

using File = cache::File;
using FilePointer = cache::FilePointer;

// Whenever a change is made to the serialized format for the KTX cache that isn't backward compatible,
// this value should be incremented.  This will force the KTX cache to be wiped
const int KTXCache::CURRENT_VERSION = 0x01;
const int KTXCache::INVALID_VERSION = 0x00;
const char* KTXCache::SETTING_VERSION_NAME = "hifi.ktx.cache_version";

KTXCache::KTXCache(const std::string& dir, const std::string& ext) :
    FileCache(dir, ext) {
    initialize();

    Setting::Handle<int> cacheVersionHandle(SETTING_VERSION_NAME, INVALID_VERSION);
    auto cacheVersion = cacheVersionHandle.get();
    if (cacheVersion != CURRENT_VERSION) {
        wipe();
        cacheVersionHandle.set(CURRENT_VERSION);
    }
}

KTXFilePointer KTXCache::writeFile(const char* data, Metadata&& metadata) {
    FilePointer file = FileCache::writeFile(data, std::move(metadata), true);
    return std::static_pointer_cast<KTXFile>(file);
}

KTXFilePointer KTXCache::getFile(const Key& key) {
    return std::static_pointer_cast<KTXFile>(FileCache::getFile(key));
}

std::unique_ptr<File> KTXCache::createFile(Metadata&& metadata, const std::string& filepath) {
    qCInfo(file_cache) << "Wrote KTX" << metadata.key.c_str();
    return std::unique_ptr<File>(new KTXFile(std::move(metadata), filepath));
}

KTXFile::KTXFile(Metadata&& metadata, const std::string& filepath) :
    cache::File(std::move(metadata), filepath) {}

