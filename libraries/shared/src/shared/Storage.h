//
//  Created by Bradley Austin Davis on 2016/02/17
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Storage_h
#define hifi_Storage_h

#include <stdint.h>
#include <vector>
#include <memory>
#include <QFile>
#include <QString>

namespace storage {
    class Storage;
    using StoragePointer = std::unique_ptr<Storage>;
    class MemoryStorage;
    using MemoryStoragePointer = std::unique_ptr<MemoryStorage>;
    class FileStorage;
    using FileStoragePointer = std::unique_ptr<FileStorage>;

    class Storage {
    public:
        virtual ~Storage() {}
        virtual const uint8_t* data() const = 0;
        virtual size_t size() const = 0;

        FileStoragePointer toFileStorage(const QString& filename) const;
        MemoryStoragePointer toMemoryStorage() const;
    };

    class MemoryStorage : public Storage {
    public:
        MemoryStorage(size_t size, const uint8_t* data);
        const uint8_t* data() const override;
        size_t size() const override;
    private:
        std::vector<uint8_t> _data;
    };

    class FileStorage : public Storage {
    public:
        static FileStoragePointer create(const QString& filename, size_t size, const uint8_t* data);
        FileStorage(const QString& filename);
        ~FileStorage();
        // Prevent copying
        FileStorage(const FileStorage& other) = delete;
        FileStorage& operator=(const FileStorage& other) = delete;

        const uint8_t* data() const override;
        size_t size() const override;
    private:
        QFile _file;
        uint8_t* _mapped { nullptr };
    };


}

#endif // hifi_Storage_h
