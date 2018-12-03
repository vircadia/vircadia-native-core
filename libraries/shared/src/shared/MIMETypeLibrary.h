//
//  MIMETypeLibrary.h
//  libraries/shared/src/shared
//
//  Created by Sabrina Shanman on 2018/11/28.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MIMETypeLibrary_h
#define hifi_MIMETypeLibrary_h

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
class MIMEType {
public:
    MIMEType(const std::string& name) :
        name(name) {
    }
    MIMEType() {};
    MIMEType(const MIMEType& mimeType) :
        name(mimeType.name),
        extensions(mimeType.extensions),
        webMediaTypes(mimeType.webMediaTypes),
        fileSignatures(mimeType.fileSignatures) {
    }

    static MIMEType NONE;

    std::string name;
    std::vector<std::string> extensions;
    std::vector<std::string> webMediaTypes;
    std::vector<FileSignature> fileSignatures;
};

MIMEType MIMEType::NONE = MIMEType("");

class MIMETypeLibrary {
public:
    using ID = unsigned int;
    static const ID INVALID_ID { 0 };

    ID registerMIMEType(const MIMEType& mimeType);
    void unregisterMIMEType(const ID& id);

    MIMEType getMIMEType(const ID& id) const;

    ID findMIMETypeForData(const hifi::ByteArray& data) const;
    ID findMIMETypeForURL(const hifi::URL& url) const;
    ID findMIMETypeForMediaType(const std::string& webMediaType) const;

protected:
    ID nextID { 1 };
    
    class Entry {
    public:
        Entry(const ID& id, const MIMEType& mimeType) :
            id(id),
            mimeType(mimeType) {
        }
        ID id;
        MIMEType mimeType;
    };

    std::vector<Entry> _mimeTypes;
};

#endif // hifi_MIMETypeLibrary_h
