//
//  Created by Bradley Austin Davis on 2016/02/17
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Storage.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(storagelogging, "hifi.core.storage")

using namespace storage;

ViewStorage::ViewStorage(const storage::StoragePointer& owner, size_t size, const uint8_t* data)
    : _owner(owner), _size(size), _data(data) {}

StoragePointer Storage::createView(size_t viewSize, size_t offset) const {
    auto selfSize = size();
    if (0 == viewSize) {
        viewSize = selfSize;
    }
    if ((viewSize + offset) > selfSize) {
        return StoragePointer();
        //TODO: Disable te exception for now and return an empty storage instead.
        //throw std::runtime_error("Invalid mapping range");
    }
    return std::make_shared<ViewStorage>(shared_from_this(), viewSize, data() + offset);
}

StoragePointer Storage::toMemoryStorage() const {
    return std::make_shared<MemoryStorage>(size(), data());
}

StoragePointer Storage::toFileStorage(const QString& filename) const {
    return FileStorage::create(filename, size(), data());
}

MemoryStorage::MemoryStorage(size_t size, const uint8_t* data) {
    _data.resize(size);
    if (data) {
        memcpy(_data.data(), data, size);
    }
}

StoragePointer FileStorage::create(const QString& filename, size_t size, const uint8_t* data) {
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
    return std::make_shared<FileStorage>(filename);
}

FileStorage::FileStorage(const QString& filename) : _file(filename) {
    bool opened = _file.open(QFile::ReadWrite);
    if (opened) {
        _hasWriteAccess = true;
    } else {
        _hasWriteAccess = false;
        opened = _file.open(QFile::ReadOnly);
    }

    if (opened) {
        _mapped = _file.map(0, _file.size());
        if (_mapped) {
            _valid = true;
        } else {
            qCWarning(storagelogging) << "Failed to map file " << filename;
        }
    } else {
        qCWarning(storagelogging) << "Failed to open file " << filename;
    }
}

FileStorage::~FileStorage() {
    if (_mapped) {
        _file.unmap(_mapped);
        _mapped = nullptr;
    }
    if (_file.isOpen()) {
        _file.close();
    }
}