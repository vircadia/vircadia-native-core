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
        auto numMips = header.numberOfMipmapLevels + 1;

        for (uint32_t l = 0; l < numMips; l++) {
            if (images.size() > l) {

                storageSize += images[l]._imageSize;
                storageSize += images[l]._imageSize;
            }
           
        }

    }

    std::unique_ptr<KTX> KTX::create(const Header& header, const KeyValues& keyValues, const Images& images) {

        std::unique_ptr<KTX> result(new KTX());
        result->resetStorage(generateStorage(header, keyValues, images).release());

        // read metadata
        result->_keyValues = keyValues;

        // populate image table
        result->_images = images;

        return result;
    }

}
