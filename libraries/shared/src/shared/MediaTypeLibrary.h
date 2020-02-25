//
//  MediaTypeLibrary.h
//  libraries/shared/src/shared
//
//  Created by Sabrina Shanman on 2018/11/28.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MediaTypeLibrary_h
#define hifi_MediaTypeLibrary_h

#include <vector>
#include <string>
#include <functional>
#include <mutex>

#include "HifiTypes.h"

// A short sequence of bytes, typically at the beginning of the file, which identifies the file format
class FileSignature {
public:
    FileSignature(const std::string& bytes, int byteOffset) :
        bytes(bytes),
        byteOffset(byteOffset) {
    }
    FileSignature(const FileSignature& fileSignature) :
        bytes(fileSignature.bytes),
        byteOffset(fileSignature.byteOffset) {
    }

    std::string bytes;
    int byteOffset;
};

// A named file extension with a list of known ways to positively identify the file type
class MediaType {
public:
    MediaType(const std::string& name) :
        name(name) {
    }
    MediaType() {};
    MediaType(const MediaType& mediaType) :
        name(mediaType.name),
        extensions(mediaType.extensions),
        webMediaTypes(mediaType.webMediaTypes),
        fileSignatures(mediaType.fileSignatures) {
    }
    MediaType& operator=(const MediaType&) = default;

    static MediaType NONE;

    std::string name;
    std::vector<std::string> extensions;
    std::vector<std::string> webMediaTypes;
    std::vector<FileSignature> fileSignatures;
};

class MediaTypeLibrary {
public:
    using ID = unsigned int;
    static const ID INVALID_ID { 0 };

    ID registerMediaType(const MediaType& mediaType);
    void unregisterMediaType(const ID& id);

    MediaType getMediaType(const ID& id) const;

    ID findMediaTypeForData(const hifi::ByteArray& data) const;
    ID findMediaTypeForURL(const hifi::URL& url) const;
    ID findMediaTypeForWebID(const std::string& webMediaType) const;

protected:
    ID nextID { 1 };
    
    class Entry {
    public:
        Entry(const ID& id, const MediaType& mediaType) :
            id(id),
            mediaType(mediaType) {
        }
        ID id;
        MediaType mediaType;
    };

    std::vector<Entry> _mediaTypes;
};

#endif // hifi_MeidaTypeLibrary_h
