//
//  HFMFormatRegistry.cpp
//  libraries/hfm/src/hfm
//
//  Created by Sabrina Shanman on 2018/11/29.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFMFormatRegistry.h"

namespace hfm {

FormatRegistry::MIMETypeID FormatRegistry::registerMIMEType(const MIMEType& mimeType, const std::shared_ptr<Serializer::Factory>& supportedFactory) {
    MIMETypeID id = _mimeTypeLibrary.registerMIMEType(mimeType);
    withWriteLock([&](){
        _supportedFormats.emplace_back(id, supportedFactory);
    });
    return id;
}

void FormatRegistry::unregisterMIMEType(const MIMETypeID& mimeTypeID) {
    withWriteLock([&](){
        for (auto it = _supportedFormats.begin(); it != _supportedFormats.end(); it++) {
            if ((*it).mimeTypeID == mimeTypeID) {
                _supportedFormats.erase(it);
                break;
            }
        }
    });
    _mimeTypeLibrary.unregisterMIMEType(mimeTypeID);
}

std::shared_ptr<Serializer::Factory> FormatRegistry::getFactoryForMIMETypeID(FormatRegistry::MIMETypeID mimeTypeID) const {
    return resultWithReadLock<std::shared_ptr<Serializer::Factory>>([&](){
        for (auto it = _supportedFormats.begin(); it != _supportedFormats.end(); it++) {
            if ((*it).mimeTypeID == mimeTypeID) {
                return (*it).factory;
            }
        }
        return std::shared_ptr<Serializer::Factory>();
    });
}

std::shared_ptr<Serializer::Factory> FormatRegistry::getFactoryForMIMEType(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url, const std::string& webMediaType) const {
    MIMETypeID id = _mimeTypeLibrary.findMatchingMIMEType(data, mapping, url, webMediaType);
    return getFactoryForMIMETypeID(id);
}

};
