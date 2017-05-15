//
//  FileCache.cpp
//  libraries/model-networking/src
//
//  Created by Zach Pomerantz on 2/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FileCache.h"

#include <cstdio>
#include <cassert>
#include <fstream>
#include <unordered_set>

#include <QDir>
#include <QSaveFile>

#include <PathUtils.h>

Q_LOGGING_CATEGORY(file_cache, "hifi.file_cache", QtWarningMsg)

using namespace cache;

static const std::string MANIFEST_NAME = "manifest";

static const size_t BYTES_PER_MEGABYTES = 1024 * 1024;
static const size_t BYTES_PER_GIGABYTES = 1024 * BYTES_PER_MEGABYTES;
const size_t FileCache::DEFAULT_UNUSED_MAX_SIZE = 5 * BYTES_PER_GIGABYTES; // 5GB
const size_t FileCache::MAX_UNUSED_MAX_SIZE = 100 * BYTES_PER_GIGABYTES; // 100GB
const size_t FileCache::DEFAULT_OFFLINE_MAX_SIZE = 2 * BYTES_PER_GIGABYTES; // 2GB

void FileCache::setUnusedFileCacheSize(size_t unusedFilesMaxSize) {
    _unusedFilesMaxSize = std::min(unusedFilesMaxSize, MAX_UNUSED_MAX_SIZE);
    reserve(0);
    emit dirty();
}

void FileCache::setOfflineFileCacheSize(size_t offlineFilesMaxSize) {
    _offlineFilesMaxSize = std::min(offlineFilesMaxSize, MAX_UNUSED_MAX_SIZE);
}

FileCache::FileCache(const std::string& dirname, const std::string& ext, QObject* parent) :
    QObject(parent),
    _ext(ext),
    _dirname(dirname),
    _dirpath(PathUtils::getAppLocalDataFilePath(dirname.c_str()).toStdString()) {}

FileCache::~FileCache() {
    clear();
}

void fileDeleter(File* file) {
    file->deleter();
}

void FileCache::initialize() {
    QDir dir(_dirpath.c_str());

    if (dir.exists()) {
        auto nameFilters = QStringList(("*." + _ext).c_str());
        auto filters = QDir::Filters(QDir::NoDotAndDotDot | QDir::Files);
        auto sort = QDir::SortFlags(QDir::Time);
        auto files = dir.entryList(nameFilters, filters, sort);

        // load persisted files
        foreach(QString filename, files) {
            const Key key = filename.section('.', 0, 0).toStdString();
            const std::string filepath = dir.filePath(filename).toStdString();
            const size_t length = QFileInfo(filepath.c_str()).size();
            addFile(Metadata(key, length), filepath);
        }

        qCDebug(file_cache, "[%s] Initialized %s", _dirname.c_str(), _dirpath.c_str());
    } else {
        dir.mkpath(_dirpath.c_str());
        qCDebug(file_cache, "[%s] Created %s", _dirname.c_str(), _dirpath.c_str());
    }

    _initialized = true;
}

FilePointer FileCache::addFile(Metadata&& metadata, const std::string& filepath) {
    FilePointer file(createFile(std::move(metadata), filepath).release(), &fileDeleter);
    if (file) {
        _numTotalFiles += 1;
        _totalFilesSize += file->getLength();
        file->_cache = this;
        emit dirty();

        Lock lock(_filesMutex);
        _files[file->getKey()] = file;
    }
    return file;
}

FilePointer FileCache::writeFile(const char* data, File::Metadata&& metadata, bool overwrite) {
    assert(_initialized);

    std::string filepath = getFilepath(metadata.key);

    Lock lock(_filesMutex);

    // if file already exists, return it
    FilePointer file = getFile(metadata.key);
    if (file) {
        if (!overwrite) {
            qCWarning(file_cache, "[%s] Attempted to overwrite %s", _dirname.c_str(), metadata.key.c_str());
            return file;
        } else {
            qCWarning(file_cache, "[%s] Overwriting %s", _dirname.c_str(), metadata.key.c_str());
            file.reset();
        }
    }

    QSaveFile saveFile(QString::fromStdString(filepath));
    if (saveFile.open(QIODevice::WriteOnly)
        && saveFile.write(data, metadata.length) == static_cast<qint64>(metadata.length)
        && saveFile.commit()) {

        file = addFile(std::move(metadata), filepath);
    } else {
        qCWarning(file_cache, "[%s] Failed to write %s", _dirname.c_str(), metadata.key.c_str());
    }

    return file;
}

FilePointer FileCache::getFile(const Key& key) {
    assert(_initialized);

    FilePointer file;

    Lock lock(_filesMutex);

    // check if file exists
    const auto it = _files.find(key);
    if (it != _files.cend()) {
        file = it->second.lock();
        if (file) {
            // if it exists, it is active - remove it from the cache
            removeUnusedFile(file);
            qCDebug(file_cache, "[%s] Found %s", _dirname.c_str(), key.c_str());
            emit dirty();
        } else {
            // if not, remove the weak_ptr
            _files.erase(it);
        }
    }

    return file;
}

std::string FileCache::getFilepath(const Key& key) {
    return _dirpath + '/' + key + '.' + _ext;
}

void FileCache::addUnusedFile(const FilePointer file) {
    {
        Lock lock(_filesMutex);
        _files[file->getKey()] = file;
    }

    reserve(file->getLength());
    file->_LRUKey = ++_lastLRUKey;

    {
        Lock lock(_unusedFilesMutex);
        _unusedFiles.insert({ file->_LRUKey, file });
        _numUnusedFiles += 1;
        _unusedFilesSize += file->getLength();
    }

    emit dirty();
}

void FileCache::removeUnusedFile(const FilePointer file) {
    Lock lock(_unusedFilesMutex);
    const auto it = _unusedFiles.find(file->_LRUKey);
    if (it != _unusedFiles.cend()) {
        _unusedFiles.erase(it);
        _numUnusedFiles -= 1;
        _unusedFilesSize -= file->getLength();
    }
}

void FileCache::reserve(size_t length) {
    Lock unusedLock(_unusedFilesMutex);
    while (!_unusedFiles.empty() &&
            _unusedFilesSize + length > _unusedFilesMaxSize) {
        auto it = _unusedFiles.begin();
        auto file = it->second;
        auto length = file->getLength();

        unusedLock.unlock();
        {
            file->_cache = nullptr;
            Lock lock(_filesMutex);
            _files.erase(file->getKey());
        }
        unusedLock.lock();

        _unusedFiles.erase(it);
        _numTotalFiles -= 1;
        _numUnusedFiles -= 1;
        _totalFilesSize -= length;
        _unusedFilesSize -= length;
    }
}

void FileCache::clear() {
    Lock unusedFilesLock(_unusedFilesMutex);
    for (const auto& pair : _unusedFiles) {
        auto& file = pair.second;
        file->_cache = nullptr;

        if (_totalFilesSize > _offlineFilesMaxSize) {
            _totalFilesSize -= file->getLength();
        } else {
            file->_shouldPersist = true;
            qCDebug(file_cache, "[%s] Persisting %s", _dirname.c_str(), file->getKey().c_str());
        }
    }
    _unusedFiles.clear();
}

void File::deleter() {
    if (_cache) {
        FilePointer self(this, &fileDeleter);
        _cache->addUnusedFile(self);
    } else {
        deleteLater();
    }
}

File::File(Metadata&& metadata, const std::string& filepath) :
    _key(std::move(metadata.key)),
    _length(metadata.length),
    _filepath(filepath) {}

File::~File() {
    QFile file(getFilepath().c_str());
    if (file.exists() && !_shouldPersist) {
        qCInfo(file_cache, "Unlinked %s", getFilepath().c_str());
        file.remove();
    }
}
