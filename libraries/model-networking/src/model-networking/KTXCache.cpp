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

#include <ktx/KTX.h>

using File = cache::File;
using FilePointer = cache::FilePointer;

KTXCache::KTXCache(const std::string& dir, const std::string& ext) :
    FileCache(dir, ext) {
    initialize();
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

