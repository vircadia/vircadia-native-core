//
//  MIMETypeLibrary.cpp
//  libraries/shared/src/shared
//
//  Created by Sabrina Shanman on 2018/11/29.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MIMETypeLibrary.h"

MIMETypeLibrary::ID MIMETypeLibrary::registerMIMEType(const MIMEType& mimeType) {
    ID id = nextID++;
    _mimeTypes.emplace_back(id, mimeType);
    return id;
}

void MIMETypeLibrary::unregisterMIMEType(const MIMETypeLibrary::ID& id) {
    for (auto it = _mimeTypes.begin(); it != _mimeTypes.end(); it++) {
        if ((*it).id == id) {
            _mimeTypes.erase(it);
            break;
        }
    }
}

MIMEType MIMETypeLibrary::getMIMEType(const MIMETypeLibrary::ID& id) const {
    for (auto& supportedFormat : _mimeTypes) {
        if (supportedFormat.id == id) {
            return supportedFormat.mimeType;
        }
    }
    return MIMEType::NONE;
}

MIMETypeLibrary::ID MIMETypeLibrary::findMatchingMIMEType(const hifi::ByteArray& data, const hifi::URL& url, const std::string& webMediaType) const {
    // Check file contents
    for (auto& mimeType : _mimeTypes) {
        for (auto& fileSignature : mimeType.mimeType.fileSignatures) {
            auto testBytes = data.mid(fileSignature.byteOffset, (int)fileSignature.bytes.size()).toStdString();
            if (testBytes == fileSignature.bytes) {
                return mimeType.id;
            }
        }
    }

    // Check file extension
    std::string urlString = url.path().toStdString();
    std::size_t extensionSeparator = urlString.rfind('.');
    if (extensionSeparator != std::string::npos) {
        std::string detectedExtension = urlString.substr(extensionSeparator + 1);
        for (auto& supportedFormat : _mimeTypes) {
            for (auto& extension : supportedFormat.mimeType.extensions) {
                if (extension == detectedExtension) {
                    return supportedFormat.id;
                }
            }
        }
    }

    // Check web media type
    if (webMediaType != "") {
        for (auto& supportedFormat : _mimeTypes) {
            for (auto& candidateWebMediaType : supportedFormat.mimeType.webMediaTypes) {
                if (candidateWebMediaType == webMediaType) {
                    return supportedFormat.id;
                }
            }
        }
    }

    // Supported file type not found.
    return INVALID_ID;
}
