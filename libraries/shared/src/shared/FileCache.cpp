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

#include "../PathUtils.h"
#include "../NumericalConstants.h"

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
    Lock lock(_mutex);
    if (_initialized) {
        qCWarning(file_cache) << "File cache already initialized";
        return;
    }

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

std::unique_ptr<File> FileCache::createFile(Metadata&& metadata, const std::string& filepath) {
    return std::unique_ptr<File>(new cache::File(std::move(metadata), filepath));
}

FilePointer FileCache::addFile(Metadata&& metadata, const std::string& filepath) {
    File* rawFile = createFile(std::move(metadata), filepath).release();
    FilePointer file(rawFile, std::bind(&File::deleter, rawFile));
    if (file) {
        _numTotalFiles += 1;
        _totalFilesSize += file->getLength();
        file->_parent = shared_from_this();
        file->_locked = true;
        emit dirty();

        _files[file->getKey()] = file;
    }
    return file;
}

FilePointer FileCache::writeFile(const char* data, File::Metadata&& metadata, bool overwrite) {
    FilePointer file;

    if (0 == metadata.length) {
        qCWarning(file_cache) << "Cannot store empty files in the cache";
        return file;
    }


    Lock lock(_mutex);

    if (!_initialized) {
        qCWarning(file_cache) << "File cache used before initialization";
        return file;
    }

    std::string filepath = getFilepath(metadata.key);

    // if file already exists, return it
    file = getFile(metadata.key);
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
    assert(!file || (file->_locked && file->_parent.lock()));
    return file;
}


FilePointer FileCache::getFile(const Key& key) {
    Lock lock(_mutex);

    FilePointer file;
    if (!_initialized) {
        qCWarning(file_cache) << "File cache used before initialization";
        return file;
    }

    // check if file exists
    const auto it = _files.find(key);
    if (it != _files.cend()) {
        file = it->second.lock();
        if (file) {
            file->touch();
            // if it exists, it is active - remove it from the cache
            if (_unusedFiles.erase(file)) {
                assert(!file->_locked);
                file->_locked = true;
                _numUnusedFiles -= 1;
                _unusedFilesSize -= file->getLength();
            } else {
                assert(file->_locked);
            }
            qCDebug(file_cache, "[%s] Found %s", _dirname.c_str(), key.c_str());
            emit dirty();
        } else {
            // if not, remove the weak_ptr
            _files.erase(it);
        }
    }

    assert(!file || (file->_locked && file->_parent.lock()));
    return file;
}

std::string FileCache::getFilepath(const Key& key) {
    return _dirpath + DIR_SEP + key + EXT_SEP + _ext;
}

// This is a non-public function that uses the mutex because it's 
// essentially a public function specifically to a File object
void FileCache::addUnusedFile(const FilePointer& file) {
    assert(file->_locked);
    file->_locked = false;
    _files[file->getKey()] = file;
    _unusedFiles.insert(file);
    _numUnusedFiles += 1;
    _unusedFilesSize += file->getLength();
    clean();

    emit dirty();
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

// Take file pointer by value to insure it doesn't get destructed during the "erase()" calls
void FileCache::eject(FilePointer file) {
    file->_locked = false;
    const auto& length = file->getLength();
    const auto& key = file->getKey();

    if (0 != _files.erase(key)) {
        _numTotalFiles -= 1;
        _totalFilesSize -= length;
    }
    if (0 != _unusedFiles.erase(file)) {
        _numUnusedFiles -= 1;
        _unusedFilesSize -= length;
    }
}

void FileCache::clean() {
    size_t overbudgetAmount = getOverbudgetAmount();

    // Avoid sorting the unused files by LRU if we're not over budget / under free space
    if (0 == overbudgetAmount) {
        return;
    }

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
    Lock lock(_mutex);
    while (!_unusedFiles.empty()) {
        eject(*_unusedFiles.begin());
    }
}

void FileCache::clear() {
    Lock lock(_mutex);

    // Eliminate any overbudget files
    clean();

    // Mark everything remaining as persisted while effectively ejecting from the cache
    for (auto& file : _unusedFiles) {
        file->_shouldPersist = true;
        file->_parent.reset();
        qCDebug(file_cache, "[%s] Persisting %s", _dirname.c_str(), file->getKey().c_str());
    }
    _unusedFiles.clear();
}

void FileCache::releaseFile(File* file) {
    Lock lock(_mutex);
    if (file->_locked) {
        addUnusedFile(FilePointer(file, std::bind(&File::deleter, file)));
    } else {
        delete file;
    }
}

void File::deleter(File* file) {
    // If the cache shut down before the file was destroyed, then we should leave the file alone (prevents crash on shutdown)
    FileCachePointer cache = file->_parent.lock();
    if (!cache) {
        file->_shouldPersist = true;
        delete file;
        return;
    }

    // Any other operations we might do should be done inside a locked section, so we need to delegate to a FileCache member function
    cache->releaseFile(file);
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

