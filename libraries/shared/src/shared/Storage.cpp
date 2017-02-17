//
//  Created by Bradley Austin Davis on 2016/02/17
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Storage.h"

#include <QFileInfo>

using namespace storage;

ViewStoragePointer Storage::createView(size_t viewSize, size_t offset) const {
    auto selfSize = size();
    if ((viewSize + offset) > selfSize) {
        throw std::runtime_error("Unable to map file");
    }
    return ViewStoragePointer(new ViewStorage(viewSize, data() + offset));
}

MemoryStoragePointer Storage::toMemoryStorage() const {
    return MemoryStoragePointer(new MemoryStorage(size(), data()));
}

FileStoragePointer Storage::toFileStorage(const QString& filename) const {
    return FileStorage::create(filename, size(), data());
}

MemoryStorage::MemoryStorage(size_t size, const uint8_t* data) {
    _data.resize(size);
    memcpy(_data.data(), data, size);
}

FileStoragePointer FileStorage::create(const QString& filename, size_t size, const uint8_t* data) {
    QFile file(filename);
    if (!file.open(QFile::ReadWrite | QIODevice::Truncate)) {
        throw std::runtime_error("Unable to open file for writing");
    }
    if (!file.resize(size)) {
        throw std::runtime_error("Unable to resize file");
    }
    {
        auto mapped = file.map(0, size);
        if (!mapped) {
            throw std::runtime_error("Unable to map file");
        }
        memcpy(mapped, data, size);
        if (!file.unmap(mapped)) {
            throw std::runtime_error("Unable to unmap file");
        }
    }
    file.close();
    return FileStoragePointer(new FileStorage(filename));
}

FileStorage::FileStorage(const QString& filename) : _file(filename) {
    if (!_file.open(QFile::ReadOnly)) {
        throw std::runtime_error("Unable to open file");
    }
    _mapped = _file.map(0, _file.size());
    if (!_mapped) {
        throw std::runtime_error("Unable to map file");
    }
}

FileStorage::~FileStorage() {
    if (_mapped) {
        if (!_file.unmap(_mapped)) {
            throw std::runtime_error("Unable to unmap file");
        }
    }
    if (_file.isOpen()) {
        _file.close();
    }
}
