//
//  Reader.cpp
//  ktx/src/ktx
//
//  Created by Zach Pomerantz on 2/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "KTX.h"

#include <list>
#include <QtGlobal>

namespace ktx {
    class Exception: public std::exception {
    public:
        Exception(std::string explanation) : _explanation(explanation) {}
        const char* what() const override {
            return ("KTX deserialization error: " + _explanation).c_str();
        }
    private:
        std::string _explanation;
    };

    bool checkEndianness(uint32_t endianness, bool& matching) {
        switch (endianness) {
        case Header::ENDIAN_TEST: {
            matching = true;
            return true;
            }
            break;
        case Header::REVERSE_ENDIAN_TEST:
            {
                matching = false;
                return true;
            }
            break;
        default:
            return false;
        }
    }

    bool checkIdentifier(const Byte* identifier) {
        return memcmp(identifier, Header::IDENTIFIER.data(), Header::IDENTIFIER_LENGTH);
    }

    KeyValues getKeyValues(size_t length, const Byte* src) {
        KeyValues keyValues;
        size_t offset = 0;

        while (offset < length) {
            // determine byte size
            uint32_t keyValueByteSize;
            memcpy(&keyValueByteSize, src, sizeof(uint32_t));
            if (keyValueByteSize > length - offset) {
                throw Exception("invalid key-value size");
            }

            // find the first null character \0
            int keyLength = 0;
            while (reinterpret_cast<const char*>(src[++keyLength]) != '\0') {
                if (keyLength == keyValueByteSize) {
                    // key must be null-terminated, and there must be space for the value
                    throw Exception("invalid key-value " + std::string(reinterpret_cast<const char*>(src), keyLength));
                }
            }

            // populate the key-value
            keyValues.emplace_back(
                std::move(std::string(reinterpret_cast<const char*>(src), keyLength)),
                std::move(std::string(reinterpret_cast<const char*>(src + keyLength), keyValueByteSize - keyLength)));

            // advance offset/src
            uint32_t keyValuePadding = 3 - ((keyValueByteSize + 3) % 4);
            offset += keyValueByteSize + keyValuePadding;
            src += keyValueByteSize + keyValuePadding;
        }

        return keyValues;
    }

    bool KTX::read(Storage* src) {
        resetStorage(src);

        return true;
    }

    bool KTX::checkStorageHeader(const Storage& src) {
        try {
            size_t srcSize = src.size();
            const uint8_t* srcBytes = src.data();

            // validation
            if (srcSize < sizeof(Header)) {
                throw Exception("length is too short for header");
            }
            const Header* header = reinterpret_cast<const Header*>(srcBytes);

            if (!checkIdentifier(header->identifier)) {
                throw Exception("identifier field invalid");
            }

            bool endianMatch { true };
            if (!checkEndianness(header->endianness, endianMatch)) {
                throw Exception("endianness field has invalid value");
            }

            // TODO: endian conversion if !endianMatch - for now, this is for local use and is unnecessary


            // TODO: calculated bytesOfTexData
            if (srcSize < (sizeof(Header) + header->bytesOfKeyValueData)) {
                throw Exception("length is too short for metadata");
            }

             size_t bytesOfTexData = 0;
            if (srcSize < (sizeof(Header) + header->bytesOfKeyValueData + bytesOfTexData)) {

                throw Exception("length is too short for data");
            }


            // read metadata
            KeyValues keyValues = getKeyValues(header->bytesOfKeyValueData, srcBytes + sizeof(Header));

            // prepare gpu::Texture using header & key-values
            // TODO

            // read data
            // TODO

            return true;
        } catch (Exception& e) {
            qWarning(e.what());
            return false;
        }
    }

    KTX* KTX::create(const Storage* data) {
        try {
            if (!checkStorageHeader(*data)) {
                
            }

            auto result = new KTX();
            result->resetStorage(const_cast<Storage*>(data));

            // read metadata
            KeyValues keyValues = getKeyValues(result->getHeader()->bytesOfKeyValueData, result->getKeyValueData());

            return nullptr;
        } catch (Exception& e) {
            qWarning(e.what());
            return nullptr;
        }
    }
}