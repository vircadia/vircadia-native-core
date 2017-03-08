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

#include <ServerPathUtils.h>

Q_LOGGING_CATEGORY(file_cache, "hifi.file_cache", QtWarningMsg)

using namespace cache;

static const std::string MANIFEST_NAME = "manifest";

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
    _dirpath(ServerPathUtils::getDataFilePath(dirname.c_str()).toStdString()) {}

FileCache::~FileCache() {
    clear();
}

void fileDeleter(File* file) {
    file->deleter();
}

void FileCache::initialize() {
    QDir dir(_dirpath.c_str());

    if (dir.exists()) {
        std::unordered_map<std::string, std::pair<Key, std::string>> persistedEntries;
        if (dir.exists(MANIFEST_NAME.c_str())) {
            std::ifstream manifest;
            manifest.open(dir.absoluteFilePath(MANIFEST_NAME.c_str()).toStdString());
            while (manifest.good()) {
                std::string key, metadata;
                std::getline(manifest, key, '\t');
                std::getline(manifest, metadata, '\n');
                if (!key.empty()) {
                    qCDebug(file_cache, "[%s] Manifest contains %s (%s)", _dirname.c_str(), key.c_str(), metadata.c_str());
                    auto filename = key + '.' + _ext;
                    persistedEntries[filename] = { key, metadata };
                }
            }
        } else {
            qCWarning(file_cache, "[%s] Missing manifest", _dirname.c_str());
        }

        std::unordered_map<Key, std::string> entries;

        foreach(QString filename, dir.entryList(QDir::Filters(QDir::NoDotAndDotDot | QDir::Files))) {
            const auto& it = persistedEntries.find(filename.toStdString());
            if (it == persistedEntries.cend()) {
                // unlink extra files
                dir.remove(filename);
                qCDebug(file_cache, "[%s] Cleaned %s", _dirname.c_str(), filename.toStdString().c_str());
            } else {
                // load existing files
                const Key& key = it->second.first;
                const std::string& metadata = it->second.second;
                const std::string filepath = dir.filePath(filename).toStdString();
                const size_t length = std::ifstream(filepath, std::ios::binary | std::ios::ate).tellg();

                FilePointer file(loadFile(key, filepath, length, metadata).release(), &fileDeleter);
                file->_cache = this;
                _files[key] = file;
                _numTotalFiles += 1;
                _totalFilesSize += length;
            }
        }

        qCDebug(file_cache, "[%s] Initialized %s", _dirname.c_str(), _dirpath.c_str());
    } else {
        dir.mkpath(_dirpath.c_str());
        qCDebug(file_cache, "[%s] Created %s", _dirname.c_str(), _dirpath.c_str());
    }

    _initialized = true;
}

FilePointer FileCache::writeFile(const Key& key, const char* data, size_t length, void* extra) {
    assert(_initialized);

    std::string filepath = getFilepath(key);

    Lock lock(_filesMutex);

    // if file already exists, return it
    FilePointer file = getFile(key);
    if (file) {
        qCWarning(file_cache, "[%s] Attempted to overwrite %s", _dirname.c_str(), key.c_str());
        return file;
    }

    // write the new file
    FILE* saveFile = fopen(filepath.c_str(), "wb");
    if (saveFile != nullptr && fwrite(data, length, 1, saveFile) && fclose(saveFile) == 0) {
        file.reset(createFile(key, filepath, length, extra).release(), &fileDeleter);
        file->_cache = this;
        _files[key] = file;
        _numTotalFiles += 1;
        _totalFilesSize += length;

        emit dirty();
    } else {
        qCWarning(file_cache, "[%s] Failed to write %s (%s)", _dirname.c_str(), key.c_str(), strerror(errno));
        errno = 0;
    }

    return file;
}

FilePointer FileCache::getFile(const Key& key) {
    assert(_initialized);

    FilePointer file;

    Lock lock(_filesMutex);

    // check if file already exists
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

        unusedLock.unlock();
        evictedFile(file);
        unusedLock.lock();
    }
}

void FileCache::clear() {
    auto forAllFiles = [&](std::function<void(const FilePointer& file)> functor) {
        Lock unusedFilesLock(_unusedFilesMutex);
        for (const auto& pair : _unusedFiles) {
            functor(pair.second);
        }
        // clear files so they are not reiterated from _files
        _unusedFiles.clear();
        unusedFilesLock.unlock();

        Lock filesLock(_filesMutex);
        for (const auto& pair : _files) {
            FilePointer file;
            if ((file = pair.second.lock())) {
                functor(file);
            }
        }
    };

    try {
        std::string manifestPath= _dirpath + '/' + MANIFEST_NAME;
        std::ofstream manifest(manifestPath);

        forAllFiles([&](const FilePointer& file) {
            file->_cache = nullptr;

            if (_totalFilesSize > _offlineFilesMaxSize) {
                _totalFilesSize -= file->getLength();
            } else {
                manifest << file->getKey() << '\t' << file->getMetadata() << '\n';
                file->_shouldPersist = true;
                qCDebug(file_cache, "[%s] Persisting %s (%s)",
                        _dirname.c_str(), file->getKey().c_str(), file->getMetadata().c_str());
            }
        });
    } catch (std::exception& e) {
        qCWarning(file_cache, "[%s] Failed to write manifest (%s)", _dirname.c_str(), e.what());

        forAllFiles([](const FilePointer& file) {
            file->_cache = nullptr;
            file->_shouldPersist = false;
        });
    }

    Lock lock(_unusedFilesMutex);
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

File::~File() {
    QFile file(getFilepath().c_str());
    if (file.exists() && !_shouldPersist) {
        qCInfo(file_cache, "Unlinked %s", getFilepath().c_str());
        file.remove();
    }
}
