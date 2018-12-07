//
//  FormatSerializerRegister.h
//  libraries/hfm/src/hfm
//
//  Created by Sabrina Shanman on 2018/11/30.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFMFormat_h
#define hifi_HFMFormat_h

#include "HFMFormatRegistry.h"
#include "HFMSerializer.h"

namespace hfm {
    // A helper class which allows early de-registration of a Serializer from ModelFormatRegistry
    class FormatSerializerRegister {
    public:
        virtual void unregisterFormat() = 0;
    };

    class DoFormatSerializerRegister : public FormatSerializerRegister {
    public:
        DoFormatSerializerRegister(const MediaType& mediaType, std::unique_ptr<Serializer::Factory> factory);

        void unregisterFormat() override;
    protected:
        FormatRegistry::MediaTypeID _mediaTypeID { FormatRegistry::INVALID_MEDIA_TYPE_ID };
    };
};

#endif // hifi_HFMFormat_h
