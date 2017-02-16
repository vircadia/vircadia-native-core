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


namespace ktx {

    class WriterException : public std::exception {
    public:
        WriterException(std::string explanation) : _explanation(explanation) {}
        const char* what() const override {
            return ("KTX serialization error: " + _explanation).c_str();
        }
    private:
        std::string _explanation;
    };

    std::unique_ptr<Storage> generateStorage(const Header& header, const KeyValues& keyValues, const Images& images) {
        size_t storageSize = sizeof(Header);
        auto numMips = header.getNumberOfLevels();

        for (uint32_t l = 0; l < numMips; l++) {
            if (images.size() > l) {
                storageSize += sizeof(uint32_t);
                storageSize += images[l]._imageSize;
                storageSize += Header::evalPadding(images[l]._imageSize);
            }
        }

        std::unique_ptr<Storage> storage(new Storage(storageSize));
        return storage;
    }


    void KTX::resetHeader(const Header& header) {
         if (!_storage) {
            return;
        }
        memcpy(_storage->_bytes, &header, sizeof(Header));
    }
    void KTX::resetImages(const Images& srcImages) {
        auto imagesDataPtr = getTexelsData();
        if (!imagesDataPtr) {
            return;
        }
        auto allocatedImagesDataSize = getTexelsDataSize();
        size_t currentDataSize = 0;
        auto currentPtr = imagesDataPtr;

        _images.clear();


        for (uint32_t l = 0; l < srcImages.size(); l++) {
            if (currentDataSize + sizeof(uint32_t) < allocatedImagesDataSize) {
                size_t imageSize = srcImages[l]._imageSize;
                *(reinterpret_cast<uint32_t*> (currentPtr)) = imageSize;
                currentPtr += sizeof(uint32_t);
                currentDataSize += sizeof(uint32_t);

                // If enough data ahead then capture the copy source pointer
                if (currentDataSize + imageSize <= (allocatedImagesDataSize)) {

                    auto copied = memcpy(currentPtr, srcImages[l]._bytes, imageSize);

                    auto padding = Header::evalPadding(imageSize);

                    _images.emplace_back(Image(imageSize, padding, currentPtr));

                    currentPtr += imageSize + padding;
                    currentDataSize += imageSize + padding;
                }
            }
        }
    }

    std::unique_ptr<KTX> KTX::create(const Header& header, const KeyValues& keyValues, const Images& images) {

        std::unique_ptr<KTX> result(new KTX());
        result->resetStorage(generateStorage(header, keyValues, images).release());

        result->resetHeader(header);

        // read metadata
        result->_keyValues = keyValues;

        // populate image table
        result->resetImages(images);

        return result;
    }

}
