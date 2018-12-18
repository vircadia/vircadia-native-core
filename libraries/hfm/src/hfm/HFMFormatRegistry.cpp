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

FormatRegistry::MediaTypeID FormatRegistry::registerMediaType(const MediaType& mediaType, std::unique_ptr<Serializer::Factory> supportedFactory) {
    std::lock_guard<std::mutex> lock(_libraryLock);

    MediaTypeID id = _mediaTypeLibrary.registerMediaType(mediaType);
    _supportedFormats.emplace_back(id, supportedFactory);
    return id;
}

void FormatRegistry::unregisterMediaType(const MediaTypeID& mediaTypeID) {
    std::lock_guard<std::mutex> lock(_libraryLock);

    for (auto it = _supportedFormats.begin(); it != _supportedFormats.end(); it++) {
        if ((*it).mediaTypeID == mediaTypeID) {
            _supportedFormats.erase(it);
            break;
        }
    }
    _mediaTypeLibrary.unregisterMediaType(mediaTypeID);
}

std::shared_ptr<Serializer> FormatRegistry::getSerializerForMediaTypeID(FormatRegistry::MediaTypeID mediaTypeID) const {
    // TODO: shared_lock in C++14
    std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&_libraryLock));

    for (auto it = _supportedFormats.begin(); it != _supportedFormats.end(); it++) {
        if ((*it).mediaTypeID == mediaTypeID) {
            return (*it).factory->get();
        }
    }
    return std::shared_ptr<Serializer>();
}

std::shared_ptr<Serializer> FormatRegistry::getSerializerForMediaType(const hifi::ByteArray& data, const hifi::URL& url, const std::string& webMediaType) const {
    MediaTypeID id;
    {
        // TODO: shared_lock in C++14
        std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&_libraryLock));

        id = _mediaTypeLibrary.findMediaTypeForData(data);
        if (id == INVALID_MEDIA_TYPE_ID) {
            id = _mediaTypeLibrary.findMediaTypeForURL(url);
            if (id == INVALID_MEDIA_TYPE_ID) {
                id = _mediaTypeLibrary.findMediaTypeForWebID(webMediaType);
            }
        }
    }
    return getSerializerForMediaTypeID(id);
}

};
