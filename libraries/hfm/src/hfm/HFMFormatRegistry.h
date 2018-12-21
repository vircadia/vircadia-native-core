//
//  HFMFormatRegistry.h
//  libraries/hfm/src/hfm
//
//  Created by Sabrina Shanman on 2018/11/28.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFMFormatRegistry_h
#define hifi_HFMFormatRegistry_h

#include "HFMSerializer.h"
#include <shared/MediaTypeLibrary.h>
#include <shared/ReadWriteLockable.h>

namespace hfm {

class FormatRegistry {
public:
    using MediaTypeID = MediaTypeLibrary::ID;
    static const MediaTypeID INVALID_MEDIA_TYPE_ID { MediaTypeLibrary::INVALID_ID };

    MediaTypeID registerMediaType(const MediaType& mediaType, std::unique_ptr<Serializer::Factory> supportedFactory);
    void unregisterMediaType(const MediaTypeID& id);

    std::shared_ptr<Serializer> getSerializerForMediaType(const hifi::ByteArray& data, const hifi::URL& url, const std::string& webMediaType) const;

protected:
    std::shared_ptr<Serializer> getSerializerForMediaTypeID(MediaTypeID id) const;

    MediaTypeLibrary _mediaTypeLibrary;
    std::mutex _libraryLock;
    class SupportedFormat {
    public:
        SupportedFormat(const MediaTypeID& mediaTypeID, std::unique_ptr<Serializer::Factory>& factory) :
            mediaTypeID(mediaTypeID),
            factory(std::move(factory)) {
        }
        MediaTypeID mediaTypeID;
        std::unique_ptr<Serializer::Factory> factory;
    };
    std::vector<SupportedFormat> _supportedFormats;
};

};

#endif // hifi_HFMFormatRegistry_h
