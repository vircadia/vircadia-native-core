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
#include <QtCore/QDebug>

#ifndef _MSC_VER
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

namespace ktx {
    class ReaderException: public std::exception {
    public:
        ReaderException(const std::string& explanation) : _explanation("KTX deserialization error: " + explanation) {}
        const char* what() const NOEXCEPT override { return _explanation.c_str(); }
    private:
        const std::string _explanation;
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
            throw ReaderException("endianness field has invalid value");
            return false;
        }
    }

    bool checkIdentifier(const Byte* identifier) {
        if (!(0 == memcmp(identifier, Header::IDENTIFIER.data(), Header::IDENTIFIER_LENGTH))) {
            throw ReaderException("identifier field invalid");
            return false;
        }
        return true;
    }


    KeyValue KeyValue::parseKeyAndValue(uint32_t keyAndValueByteSize, const Byte* bytes) {
        // find the first null character \0
        uint32_t keyLength = 0;
        while (reinterpret_cast<const char*>(bytes[++keyLength]) != '\0') {
            if (keyLength == keyAndValueByteSize) {
                // key must be null-terminated, and there must be space for the value
                throw ReaderException("invalid key-value " + std::string(reinterpret_cast<const char*>(bytes), keyLength));
            }
        }

        return KeyValue(std::string(reinterpret_cast<const char*>(bytes), keyLength), keyAndValueByteSize - keyLength, bytes + keyLength);
    }

    static KeyValue parseSerializedKeyAndValue(uint32_t byteSizeAhead, const Byte* bytes) {
        uint32_t keyValueByteSize;
        memcpy(&keyValueByteSize, bytes, sizeof(uint32_t));
        if (keyValueByteSize > byteSizeAhead) {
            throw ReaderException("invalid key-value size");
        }

        auto keyValueBytes = bytes + sizeof(uint32_t);

        // parse the key-value
        return KeyValue::parseKeyAndValue(keyValueByteSize, keyValueBytes);
    }

    static KeyValues parseKeyValues(size_t srcSize, const Byte* srcBytes);

    KeyValues KTX::parseKeyValues(size_t srcSize, const Byte* src) {
        KeyValues keyValues;
        try {
            uint32_t length = (uint32_t) srcSize;
            uint32_t offset = 0;
            while (offset < length) {
                auto keyValue = parseSerializedKeyAndValue(length - offset, src);
                keyValues.emplace_back(keyValue);

                // advance offset/src
                offset += keyValue.serializedByteSize();
                src += keyValue.serializedByteSize();
            }
        }
        catch (const ReaderException& e) {
            qWarning() << e.what();
        }
        return keyValues;
    }

    bool KTX::checkHeaderFromStorage(size_t srcSize, const Byte* srcBytes) {
        try {
            // validation
            if (srcSize < sizeof(Header)) {
                throw ReaderException("length is too short for header");
            }
            const Header* header = reinterpret_cast<const Header*>(srcBytes);

            checkIdentifier(header->identifier);

            bool endianMatch { true };
            checkEndianness(header->endianness, endianMatch);

            // TODO: endian conversion if !endianMatch - for now, this is for local use and is unnecessary


            // TODO: calculated bytesOfTexData
            if (srcSize < (sizeof(Header) + header->bytesOfKeyValueData)) {
                throw ReaderException("length is too short for metadata");
            }

             size_t bytesOfTexData = 0;
            if (srcSize < (sizeof(Header) + header->bytesOfKeyValueData + bytesOfTexData)) {

                throw ReaderException("length is too short for data");
            }

            return true;
        }
        catch (const ReaderException& e) {
            qWarning() << e.what();
            return false;
        }
    }

    Images KTX::parseImages(const Header& header, size_t srcSize, const Byte* srcBytes) {
        Images images;
        auto currentPtr = srcBytes;
        auto numFaces = header.numberOfFaces;

        // Keep identifying new mip as long as we can at list query the next imageSize
        while ((currentPtr - srcBytes) + sizeof(uint32_t) <= (srcSize)) {

            // Grab the imageSize coming up
            size_t imageSize = *reinterpret_cast<const uint32_t*>(currentPtr);
            currentPtr += sizeof(uint32_t);

            // If enough data ahead then capture the pointer
            if ((currentPtr - srcBytes) + imageSize <= (srcSize)) {
                auto padding = Header::evalPadding(imageSize);

                if (numFaces == 6) {
                    size_t faceSize = imageSize / 6;
                    Image::FaceBytes faces(6);
                    for (uint32_t face = 0; face < 6; face++) {
                        faces[face] = currentPtr;
                        currentPtr += faceSize;
                    }
                    images.emplace_back(Image((uint32_t) faceSize, padding, faces));
                    currentPtr += padding;
                } else {
                    images.emplace_back(Image((uint32_t) imageSize, padding, currentPtr));
                    currentPtr += imageSize + padding;
                }
            } else {
                break;
            }
        }

        return images;
    }

    std::unique_ptr<KTX> KTX::create(const StoragePointer& src) {
        if (!src) {
            return nullptr;
        }

        if (!checkHeaderFromStorage(src->size(), src->data())) {
            return nullptr;
        }

        std::unique_ptr<KTX> result(new KTX());
        result->resetStorage(src);

        // read metadata
        result->_keyValues = parseKeyValues(result->getHeader()->bytesOfKeyValueData, result->getKeyValueData());

        // populate image table
        result->_images = parseImages(*result->getHeader(), result->getTexelsDataSize(), result->getTexelsData());

        return result;
    }
}
