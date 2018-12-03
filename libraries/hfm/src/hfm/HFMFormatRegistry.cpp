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

FormatRegistry::MIMETypeID FormatRegistry::registerMIMEType(const MIMEType& mimeType, std::unique_ptr<Serializer::Factory>& supportedFactory) {
    std::lock_guard<std::mutex> lock(_libraryLock);

    MIMETypeID id = _mimeTypeLibrary.registerMIMEType(mimeType);
    _supportedFormats.emplace_back(id, supportedFactory);
    return id;
}

void FormatRegistry::unregisterMIMEType(const MIMETypeID& mimeTypeID) {
    std::lock_guard<std::mutex> lock(_libraryLock);

    for (auto it = _supportedFormats.begin(); it != _supportedFormats.end(); it++) {
        if ((*it).mimeTypeID == mimeTypeID) {
            _supportedFormats.erase(it);
            break;
        }
    }
    _mimeTypeLibrary.unregisterMIMEType(mimeTypeID);
}

std::shared_ptr<Serializer> FormatRegistry::getSerializerForMIMETypeID(FormatRegistry::MIMETypeID mimeTypeID) const {
    // TODO: shared_lock in C++14
    std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&_libraryLock));

    for (auto it = _supportedFormats.begin(); it != _supportedFormats.end(); it++) {
        if ((*it).mimeTypeID == mimeTypeID) {
            return (*it).factory->get();
        }
    }
    return std::shared_ptr<Serializer>();
}

std::shared_ptr<Serializer> FormatRegistry::getSerializerForMIMEType(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url, const std::string& webMediaType) const {
    MIMETypeID id;
    {
        // TODO: shared_lock in C++14
        std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&_libraryLock));
        id = _mimeTypeLibrary.findMatchingMIMEType(data, mapping, url, webMediaType);
    }
    return getSerializerForMIMETypeID(id);
}

};
