//
//  FileCache.h
//  libraries/networking/src
//
//  Created by Zach Pomerantz on 2/21/2017.
//  Copyright 2017 High Fidelity, Inc.  // //  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FileCache_h
#define hifi_FileCache_h

#include <atomic>
#include <memory>
#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

#include <QObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(file_cache)

namespace cache {

class File;
using FilePointer = std::shared_ptr<File>;

class FileCache : public QObject {
    Q_OBJECT
    Q_PROPERTY(size_t numTotal READ getNumTotalFiles NOTIFY dirty)
    Q_PROPERTY(size_t numCached READ getNumCachedFiles NOTIFY dirty)
    Q_PROPERTY(size_t sizeTotal READ getSizeTotalFiles NOTIFY dirty)
    Q_PROPERTY(size_t sizeCached READ getSizeCachedFiles NOTIFY dirty)

    static const size_t DEFAULT_UNUSED_MAX_SIZE;
    static const size_t MAX_UNUSED_MAX_SIZE;
    static const size_t DEFAULT_OFFLINE_MAX_SIZE;

public:
    size_t getNumTotalFiles() const { return _numTotalFiles; }
    size_t getNumCachedFiles() const { return _numUnusedFiles; }
    size_t getSizeTotalFiles() const { return _totalFilesSize; }
    size_t getSizeCachedFiles() const { return _unusedFilesSize; }

    void setUnusedFileCacheSize(size_t unusedFilesMaxSize);
    size_t getUnusedFileCacheSize() const { return _unusedFilesSize; }

    void setOfflineFileCacheSize(size_t offlineFilesMaxSize);

    // initialize FileCache with a directory name (not a path, ex.: "temp_jpgs") and an ext (ex.: "jpg")
    FileCache(const std::string& dirname, const std::string& ext, QObject* parent = nullptr);
    // precondition: there should be no references to Files when FileCache is destroyed
    virtual ~FileCache();

    // derived classes are left to implement hashing of the files on their own
    using Key = std::string;

    // derived classes should implement a setter/getter, for example, for a FileCache backing a network cache:
    //
    // DerivedFilePointer writeFile(const DerivedData& data) {
    //  return writeFile(data->key, data->data, data->length, &data);
    // }
    //
    // DerivedFilePointer getFile(const QUrl& url) {
    //  // assuming storage/removal of url->hash in createFile/evictedFile overrides
    //  auto key = lookup_hash_for(url);
    //  return getFile(key);
    // }

signals:
    void dirty();

protected:
    /// must be called after construction to create the cache on the fs and restore persisted files
    void initialize();

    FilePointer writeFile(const Key& key, const char* data, size_t length, void* extra);
    FilePointer getFile(const Key& key);

    /// create a file (ex.: create a class derived from File and store it in a secondary map with extra->url)
    virtual std::unique_ptr<File> createFile(const Key& key, const std::string& filepath, size_t length, void* extra) = 0;
    /// load a file
    virtual std::unique_ptr<File> loadFile(const Key& key, const std::string& filepath, size_t length, const std::string& metadata) = 0;
    /// take action when a file is evicted from the cache (ex.: evict it from a secondary map)
    virtual void evictedFile(const FilePointer& file) = 0;

private:
    using Mutex = std::recursive_mutex;
    using Lock = std::unique_lock<Mutex>;

    friend class File;

    std::string getFilepath(const Key& key);

    void addUnusedFile(const FilePointer file);
    void removeUnusedFile(const FilePointer file);
    void reserve(size_t length);
    void clear();

    std::atomic<size_t> _numTotalFiles { 0 };
    std::atomic<size_t> _numUnusedFiles { 0 };
    std::atomic<size_t> _totalFilesSize { 0 };
    std::atomic<size_t> _unusedFilesSize { 0 };

    std::string _ext;
    std::string _dirname;
    std::string _dirpath;
    bool _initialized { false };

    std::unordered_map<Key, std::weak_ptr<File>> _files;
    Mutex _filesMutex;

    std::map<int, FilePointer> _unusedFiles;
    Mutex _unusedFilesMutex;
    size_t _unusedFilesMaxSize { DEFAULT_UNUSED_MAX_SIZE };
    int _lastLRUKey { 0 };

    size_t _offlineFilesMaxSize { DEFAULT_OFFLINE_MAX_SIZE };
};

class File : public QObject {
    Q_OBJECT

public:
    using Key = FileCache::Key;

    std::string getFilepath() const { return _filepath; }
    Key getKey() const { return _key; }
    size_t getLength() const { return _length; }

    // the destructor should handle unlinking of the actual filepath
    virtual ~File();
    // overrides should call File::deleter to maintain caching behavior
    virtual void deleter();

protected:
    // when constructed, the file has already been created/written
    File(const Key& key, const std::string& filepath, size_t length) :
        _filepath(filepath), _key(key), _length(length) {}

    /// get metadata to store with a file between instances (ex.: return the url of a hash)
    virtual std::string getMetadata() const = 0;

    const std::string _filepath;

private:
    friend class FileCache;

    const Key _key;
    const size_t _length;

    FileCache* _cache;
    int _LRUKey { 0 };

    bool _shouldPersist { false };
};

}

#endif // hifi_FileCache_h
