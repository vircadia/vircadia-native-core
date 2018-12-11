//
//  MediaTypeLibrary.cpp
//  libraries/shared/src/shared
//
//  Created by Sabrina Shanman on 2018/11/29.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MediaTypeLibrary.h"

MediaType MediaType::NONE = MediaType("");

MediaTypeLibrary::ID MediaTypeLibrary::registerMediaType(const MediaType& mediaType) {
    ID id = nextID++;
    _mediaTypes.emplace_back(id, mediaType);
    return id;
}

void MediaTypeLibrary::unregisterMediaType(const MediaTypeLibrary::ID& id) {
    for (auto it = _mediaTypes.begin(); it != _mediaTypes.end(); it++) {
        if ((*it).id == id) {
            _mediaTypes.erase(it);
            break;
        }
    }
}

MediaType MediaTypeLibrary::getMediaType(const MediaTypeLibrary::ID& id) const {
    for (auto& supportedFormat : _mediaTypes) {
        if (supportedFormat.id == id) {
            return supportedFormat.mediaType;
        }
    }
    return MediaType::NONE;
}

MediaTypeLibrary::ID MediaTypeLibrary::findMediaTypeForData(const hifi::ByteArray& data) const {
    // Check file contents
    for (auto& mediaType : _mediaTypes) {
        for (auto& fileSignature : mediaType.mediaType.fileSignatures) {
            auto testBytes = data.mid(fileSignature.byteOffset, (int)fileSignature.bytes.size()).toStdString();
            if (testBytes == fileSignature.bytes) {
                return mediaType.id;
            }
        }
    }

    return INVALID_ID;
}

MediaTypeLibrary::ID MediaTypeLibrary::findMediaTypeForURL(const hifi::URL& url) const {
    // Check file extension
    std::string urlString = url.path().toStdString();
    std::size_t extensionSeparator = urlString.rfind('.');
    if (extensionSeparator != std::string::npos) {
        std::string detectedExtension = urlString.substr(extensionSeparator + 1);
        for (auto& supportedFormat : _mediaTypes) {
            for (auto& extension : supportedFormat.mediaType.extensions) {
                if (extension == detectedExtension) {
                    return supportedFormat.id;
                }
            }
        }
    }

    return INVALID_ID;
}

MediaTypeLibrary::ID MediaTypeLibrary::findMediaTypeForWebID(const std::string& webMediaType) const {
    // Check web media type
    if (webMediaType != "") {
        for (auto& supportedFormat : _mediaTypes) {
            for (auto& candidateWebMediaType : supportedFormat.mediaType.webMediaTypes) {
                if (candidateWebMediaType == webMediaType) {
                    return supportedFormat.id;
                }
            }
        }
    }

    return INVALID_ID;
}
