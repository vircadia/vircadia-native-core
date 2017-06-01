//
//  Writer.cpp
//  ktx/src/ktx
//
//  Created by Zach Pomerantz on 2/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "KTX.h"


#include <QtGlobal>
#include <QtCore/QDebug>
#ifndef _MSC_VER
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

namespace ktx {

    class WriterException : public std::exception {
    public:
        WriterException(const std::string& explanation) : _explanation("KTX serialization error: " + explanation) {}
        const char* what() const NOEXCEPT override { return _explanation.c_str(); }
    private:
        const std::string _explanation;
    };

    std::unique_ptr<KTX> KTX::create(const Header& header, const Images& images, const KeyValues& keyValues) {
        StoragePointer storagePointer;
        {
            auto storageSize = ktx::KTX::evalStorageSize(header, images, keyValues);
            auto memoryStorage = new storage::MemoryStorage(storageSize);
            ktx::KTX::write(memoryStorage->data(), memoryStorage->size(), header, images, keyValues);
            storagePointer.reset(memoryStorage);
        }
        return create(storagePointer);
    }

    std::unique_ptr<KTX> KTX::createBare(const Header& header, const KeyValues& keyValues) {
        auto descriptors = header.generateImageDescriptors();

        Byte minMip = header.numberOfMipmapLevels;
        auto newKeyValues = keyValues;
        newKeyValues.emplace_back(KeyValue(HIFI_MIN_POPULATED_MIP_KEY, sizeof(Byte), &minMip));

        StoragePointer storagePointer;
        {
            auto storageSize = ktx::KTX::evalStorageSize(header, descriptors, newKeyValues);
            auto memoryStorage = new storage::MemoryStorage(storageSize);
            qDebug() << "Memory storage size is: " << storageSize;
            ktx::KTX::writeWithoutImages(memoryStorage->data(), memoryStorage->size(), header, descriptors, newKeyValues);
            storagePointer.reset(memoryStorage);
        }
        return create(storagePointer);
    }

    size_t KTX::evalStorageSize(const Header& header, const Images& images, const KeyValues& keyValues) {
        size_t storageSize = sizeof(Header);

        if (!keyValues.empty()) {
            size_t keyValuesSize = KeyValue::serializedKeyValuesByteSize(keyValues);
            storageSize += keyValuesSize;
        }

        auto numMips = header.getNumberOfLevels();
        for (uint32_t l = 0; l < numMips; l++) {
            if (images.size() > l) {
                storageSize += sizeof(uint32_t);
                storageSize += evalPaddedSize(images[l]._imageSize);
            }
        }
        return storageSize;
    }

    size_t KTX::evalStorageSize(const Header& header, const ImageDescriptors& imageDescriptors, const KeyValues& keyValues) {
        size_t storageSize = sizeof(Header);

        if (!keyValues.empty()) {
            size_t keyValuesSize = KeyValue::serializedKeyValuesByteSize(keyValues);
            storageSize += keyValuesSize;
        }

        auto numMips = header.getNumberOfLevels();
        for (uint32_t l = 0; l < numMips; l++) {
            if (imageDescriptors.size() > l) {
                storageSize += sizeof(uint32_t);
                storageSize += evalPaddedSize(imageDescriptors[l]._imageSize);
            }
        }
        return storageSize;
    }

    size_t KTX::write(Byte* destBytes, size_t destByteSize, const Header& header, const Images& srcImages, const KeyValues& keyValues) {
        // Check again that we have enough destination capacity
        if (!destBytes || (destByteSize < evalStorageSize(header, srcImages, keyValues))) {
            return 0;
        }

        auto currentDestPtr = destBytes;
        // Header
        auto destHeader = reinterpret_cast<Header*>(currentDestPtr);
        memcpy(currentDestPtr, &header, sizeof(Header));
        currentDestPtr += sizeof(Header);

        // KeyValues
        if (!keyValues.empty()) {
            destHeader->bytesOfKeyValueData = (uint32_t) writeKeyValues(currentDestPtr, destByteSize - sizeof(Header), keyValues);
        } else {
            // Make sure the header contains the right bytesOfKeyValueData size
            destHeader->bytesOfKeyValueData = 0;
        }
        currentDestPtr += destHeader->bytesOfKeyValueData;

        // Images
        auto destImages = writeImages(currentDestPtr, destByteSize - sizeof(Header) - destHeader->bytesOfKeyValueData, srcImages);
        // We chould check here that the amoutn of dest IMages generated is the same as the source

        return destByteSize;
    }

    size_t KTX::writeWithoutImages(Byte* destBytes, size_t destByteSize, const Header& header, const ImageDescriptors& descriptors, const KeyValues& keyValues) {
        // Check again that we have enough destination capacity
        if (!destBytes || (destByteSize < evalStorageSize(header, descriptors, keyValues))) {
            qWarning() << "Destination capacity is insufficient to write KTX without images";
            return 0;
        }

        auto currentDestPtr = destBytes;
        // Header
        auto destHeader = reinterpret_cast<Header*>(currentDestPtr);
        memcpy(currentDestPtr, &header, sizeof(Header));
        currentDestPtr += sizeof(Header);


        // KeyValues
        if (!keyValues.empty()) {
            destHeader->bytesOfKeyValueData = (uint32_t) writeKeyValues(currentDestPtr, destByteSize - sizeof(Header), keyValues);
        } else {
            // Make sure the header contains the right bytesOfKeyValueData size
            destHeader->bytesOfKeyValueData = 0;
        }
        currentDestPtr += destHeader->bytesOfKeyValueData;

        for (size_t i = 0; i < descriptors.size(); ++i) {
            auto ptr = reinterpret_cast<uint32_t*>(currentDestPtr);
            uint32_t imageFaceSize = descriptors[i]._faceSize;
            *ptr = imageFaceSize; // the imageSize written in the ktx is the FACE size

#ifdef DEBUG
            ptr++;
            for (size_t k = 0; k < descriptors[i]._imageSize/4; k++) {
                *(ptr + k) = 0xFFFFFFFF;
            }
#endif
            currentDestPtr += sizeof(uint32_t);
            currentDestPtr += descriptors[i]._imageSize;
        }

        return destByteSize;
    }

    uint32_t KeyValue::writeSerializedKeyAndValue(Byte* destBytes, uint32_t destByteSize, const KeyValue& keyval) {
        uint32_t keyvalSize = keyval.serializedByteSize();
        if (keyvalSize > destByteSize) {
            throw WriterException("invalid key-value size");
        }

        *((uint32_t*) destBytes) = keyval._byteSize;

        auto dest = destBytes + sizeof(uint32_t);

        auto keySize = keyval._key.size() + 1; // Add 1 for the '\0' character at the end of the string
        memcpy(dest, keyval._key.data(), keySize);
        dest += keySize;

        memcpy(dest, keyval._value.data(), keyval._value.size());

        return keyvalSize;
    }

    size_t KTX::writeKeyValues(Byte* destBytes, size_t destByteSize, const KeyValues& keyValues) {
        size_t writtenByteSize = 0;
        try {
            auto dest = destBytes;
            for (auto& keyval : keyValues) {
                size_t keyvalSize = KeyValue::writeSerializedKeyAndValue(dest, (uint32_t) (destByteSize - writtenByteSize), keyval);
                writtenByteSize += keyvalSize;
                dest += keyvalSize;
            }
        }
        catch (const WriterException& e) {
            qWarning() << e.what();
        }
        return writtenByteSize;
    }

    Images KTX::writeImages(Byte* destBytes, size_t destByteSize, const Images& srcImages) {
        Images destImages;
        auto imagesDataPtr = destBytes;
        if (!imagesDataPtr) {
            return destImages;
        }
        auto allocatedImagesDataSize = destByteSize;
        size_t currentDataSize = 0;
        auto currentPtr = imagesDataPtr;

        for (uint32_t l = 0; l < srcImages.size(); l++) {
            if (currentDataSize + sizeof(uint32_t) < allocatedImagesDataSize) {
                uint32_t imageOffset = currentPtr - destBytes;
                size_t imageSize = srcImages[l]._imageSize;
                size_t imageFaceSize = srcImages[l]._faceSize;
                *(reinterpret_cast<uint32_t*> (currentPtr)) = (uint32_t)imageFaceSize; // the imageSize written in the ktx is the FACE size
                currentPtr += sizeof(uint32_t);
                currentDataSize += sizeof(uint32_t);

                // If enough data ahead then capture the copy source pointer
                if (currentDataSize + imageSize <= (allocatedImagesDataSize)) {
                    auto padding = evalPadding(imageSize);

                    // Single face vs cubes
                    if (srcImages[l]._numFaces == 1) {
                        memcpy(currentPtr, srcImages[l]._faceBytes[0], imageSize);
                        destImages.emplace_back(Image(imageOffset, (uint32_t) imageSize, padding, currentPtr));
                        currentPtr += imageSize;
                    } else {
                        Image::FaceBytes faceBytes(NUM_CUBEMAPFACES);
                        auto faceSize = srcImages[l]._faceSize;
                        for (uint32_t face = 0; face < NUM_CUBEMAPFACES; face++) {
                             memcpy(currentPtr, srcImages[l]._faceBytes[face], faceSize);
                             faceBytes[face] = currentPtr;
                             currentPtr += faceSize;
                        }
                        destImages.emplace_back(Image(imageOffset, faceSize, padding, faceBytes));
                    }

                    currentPtr += padding;
                    currentDataSize += imageSize + padding;
                }
            }
        }
           
        return destImages;
    }

    void KTX::writeMipData(uint16_t level, const Byte* sourceBytes, size_t sourceSize) {
        Q_ASSERT(level > 0);
        Q_ASSERT(level < _images.size());
        Q_ASSERT(sourceSize == _images[level]._imageSize);

        //memcpy(reinterpret_cast<void*>(_images[level]._faceBytes[0]), sourceBytes, sourceSize);
    }
}
