//
//  HFMSimpleFormat.h
//  libraries/hfm/src/hfm
//
//  Created by Sabrina Shanman on 2018/11/30.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFMSimpleFormat_h
#define hifi_HFMSimpleFormat_h

#include "HFMFormat.h"
#include "HFMSerializer.h"

namespace hfm {
    template<typename T>
    class SimpleFactory : public Serializer::Factory {
        std::shared_ptr<Serializer> get() override {
            return std::make_shared<T>();
        }
    };

    template<typename T> // T is an implementation of hfm::Serializer
    class SimpleFormat : public Format {
    public:
        SimpleFormat(const MediaType& mediaType) : Format(),
            _mediaType(mediaType) {
        }

        void registerFormat(FormatRegistry& registry) override {
            _mediaTypeID = registry.registerMediaType(_mediaType, std::make_unique<SimpleFactory<T>>());
        }

        void unregisterFormat(FormatRegistry& registry) override {
            registry.unregisterMediaType(_mediaTypeID);
            _mediaTypeID = hfm::FormatRegistry::INVALID_MEDIA_TYPE_ID;
        }
    protected:
        MediaType _mediaType;
        hfm::FormatRegistry::MediaTypeID _mediaTypeID;
    };
};

#endif // hifi_HFMSimpleFormat_h
