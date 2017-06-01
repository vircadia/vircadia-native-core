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


#include <unordered_set>
#include <queue>
#include <cassert>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QSaveFile>
#include <QtCore/QStorageInfo>

#include <PathUtils.h>
#include <NumericalConstants.h>

#ifdef Q_OS_WIN
#include <sys/utime.h>
#else
#include <utime.h>
#endif

#ifdef NDEBUG
Q_LOGGING_CATEGORY(file_cache, "hifi.file_cache", QtWarningMsg)
#else
Q_LOGGING_CATEGORY(file_cache, "hifi.file_cache")
#endif
using namespace cache;

static const char DIR_SEP = '/';
static const char EXT_SEP = '.';

const size_t FileCache::DEFAULT_MAX_SIZE { GB_TO_BYTES(5) };
const size_t FileCache::MAX_MAX_SIZE { GB_TO_BYTES(100) };
const size_t FileCache::DEFAULT_MIN_FREE_STORAGE_SPACE { GB_TO_BYTES(1) };


std::string getCacheName(const std::string& dirname_str) {
    QString dirname { dirname_str.c_str() };
    QDir dir(dirname);
    if (!dir.isAbsolute()) {
        return dirname_str;
    }
    return dir.dirName().toStdString();
}

std::string getCachePath(const std::string& dirname_str) {
    QString dirname { dirname_str.c_str() };
    QDir dir(dirname);
    if (dir.isAbsolute()) {
        return dirname_str;
    }
    return PathUtils::getAppLocalDataFilePath(dirname).toStdString();
}

void FileCache::setMinFreeSize(size_t size) {
    _minFreeSpaceSize = size;
    clean();
    emit dirty();
}

void FileCache::setMaxSize(size_t maxSize) {
    _maxSize = std::min(maxSize, MAX_MAX_SIZE);
    clean();
    emit dirty();
}

FileCache::FileCache(const std::string& dirname, const std::string& ext, QObject* parent) :
    QObject(parent),
    _ext(ext),
    _dirname(getCacheName(dirname)),
    _dirpath(getCachePath(dirname)) {
}

FileCache::~FileCache() {
    clear();
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
    FilePointer file(createFile(std::move(metadata), filepath).release(), std::mem_fn(&File::deleter));
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
            file->touch();
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
    return _dirpath + DIR_SEP + key + EXT_SEP + _ext;
}

void FileCache::addUnusedFile(const FilePointer& file) {
    {
        Lock lock(_filesMutex);
        _files[file->getKey()] = file;
    }

    {
        Lock lock(_unusedFilesMutex);
        _unusedFiles.insert(file);
        _numUnusedFiles += 1;
        _unusedFilesSize += file->getLength();
    }
    clean();

    emit dirty();
}

void FileCache::removeUnusedFile(const FilePointer& file) {
    Lock lock(_unusedFilesMutex);
    if (_unusedFiles.erase(file)) {
        _numUnusedFiles -= 1;
        _unusedFilesSize -= file->getLength();
    }
}

size_t FileCache::getOverbudgetAmount() const {
    size_t result = 0;

    size_t currentFreeSpace = QStorageInfo(_dirpath.c_str()).bytesFree();
    if (_minFreeSpaceSize > currentFreeSpace) {
        result = _minFreeSpaceSize - currentFreeSpace;
    }

    if (_totalFilesSize > _maxSize) {
        result = std::max(_totalFilesSize - _maxSize, result);
    }

    return result;
}

namespace cache {
    struct FilePointerComparator {
        bool operator()(const FilePointer& a, const FilePointer& b) {
            return a->_modified > b->_modified;
        }
    };
}

void FileCache::eject(const FilePointer& file) {
    file->_cache = nullptr;
    const auto& length = file->getLength();
    const auto& key = file->getKey();

    {
        Lock lock(_filesMutex);
        if (0 != _files.erase(key)) {
            _numTotalFiles -= 1;
            _totalFilesSize -= length;
        }
    }

    {
        Lock unusedLock(_unusedFilesMutex);
        if (0 != _unusedFiles.erase(file)) {
            _numUnusedFiles -= 1;
            _unusedFilesSize -= length;
        }
    }
}

void FileCache::clean() {
    size_t overbudgetAmount = getOverbudgetAmount();

    // Avoid sorting the unused files by LRU if we're not over budget / under free space
    if (0 == overbudgetAmount) {
        return;
    }

    Lock unusedLock(_unusedFilesMutex);
    using Queue = std::priority_queue<FilePointer, std::vector<FilePointer>, FilePointerComparator>;
    Queue queue;
    for (const auto& file : _unusedFiles) {
        queue.push(file);
    }

    while (!queue.empty() && overbudgetAmount > 0) {
        auto file = queue.top();
        queue.pop();
        eject(file);
        auto length = file->getLength();
        overbudgetAmount -= std::min(length, overbudgetAmount);
    }
}

void FileCache::wipe() {
    Lock unusedFilesLock(_unusedFilesMutex);
    while (!_unusedFiles.empty()) {
        eject(*_unusedFiles.begin());
    }
}

void FileCache::clear() {
    // Eliminate any overbudget files
    clean();

    // Mark everything remaining as persisted
    Lock unusedFilesLock(_unusedFilesMutex);
    for (auto& file : _unusedFiles) {
        file->_shouldPersist = true;
        file->_cache = nullptr;
        qCDebug(file_cache, "[%s] Persisting %s", _dirname.c_str(), file->getKey().c_str());
    }
    _unusedFiles.clear();
}

void File::deleter() {
    if (_cache) {
        _cache->addUnusedFile(FilePointer(this, std::mem_fn(&File::deleter)));
    } else {
        deleteLater();
    }
}

File::File(Metadata&& metadata, const std::string& filepath) :
    _key(std::move(metadata.key)),
    _length(metadata.length),
    _filepath(filepath),
    _modified(QFileInfo(_filepath.c_str()).lastRead().toMSecsSinceEpoch()) {
}

File::~File() {
    QFile file(getFilepath().c_str());
    if (file.exists() && !_shouldPersist) {
        qCInfo(file_cache, "Unlinked %s", getFilepath().c_str());
        file.remove();
    }
}

void File::touch() {
    utime(_filepath.c_str(), nullptr);
    _modified = std::max<int64_t>(QFileInfo(_filepath.c_str()).lastRead().toMSecsSinceEpoch(), _modified);
}