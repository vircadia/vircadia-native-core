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
    using StoragePointer = std::shared_ptr<const Storage>;

    // Abstract class to represent memory that stored _somewhere_ (in system memory or in a file, for example)
    class Storage : public std::enable_shared_from_this<Storage> {
    public:
        virtual ~Storage() {}
        virtual const uint8_t* data() const = 0;
        virtual uint8_t* mutableData() = 0;
        virtual size_t size() const = 0;
        virtual operator bool() const { return true; }

        StoragePointer createView(size_t size = 0, size_t offset = 0) const;
        StoragePointer toFileStorage(const QString& filename) const;
        StoragePointer toMemoryStorage() const;

        // Aliases to prevent having to re-write a ton of code
        inline size_t getSize() const { return size(); }
        inline const uint8_t* readData() const { return data(); }
    };

    class MemoryStorage : public Storage {
    public:
        MemoryStorage(size_t size, const uint8_t* data = nullptr);
        const uint8_t* data() const override { return _data.data(); }
        uint8_t* data() { return _data.data(); }
        uint8_t* mutableData() override { return _data.data(); }
        size_t size() const override { return _data.size(); }
        operator bool() const override { return true; }
    private:
        std::vector<uint8_t> _data;
    };

    class FileStorage : public Storage {
    public:
        static StoragePointer create(const QString& filename, size_t size, const uint8_t* data);
        FileStorage(const QString& filename);
        ~FileStorage();
        // Prevent copying
        FileStorage(const FileStorage& other) = delete;
        FileStorage& operator=(const FileStorage& other) = delete;

        const uint8_t* data() const override { return _mapped; }
        uint8_t* mutableData() override { return _hasWriteAccess ? _mapped : nullptr; }
        size_t size() const override { return _file.size(); }
        operator bool() const override { return _valid; }
    private:

        bool _valid { false };
        bool _hasWriteAccess { false };
        QFile _file;
        uint8_t* _mapped { nullptr };
    };

    class ViewStorage : public Storage {
    public:
        ViewStorage(const storage::StoragePointer& owner, size_t size, const uint8_t* data);
        const uint8_t* data() const override { return _data; }
        uint8_t* mutableData() override { throw std::runtime_error("Cannot modify ViewStorage");  }
        size_t size() const override { return _size; }
        operator bool() const override { return *_owner; }
    private:
        const storage::StoragePointer _owner;
        const size_t _size;
        const uint8_t* _data;
    };

}

#endif // hifi_Storage_h
