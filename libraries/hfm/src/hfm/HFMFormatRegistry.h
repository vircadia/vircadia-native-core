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
#include <shared/MIMETypeLibrary.h>
#include <shared/ReadWriteLockable.h>

namespace hfm {

class FormatRegistry : public ReadWriteLockable {
public:
    using MIMETypeID = MIMETypeLibrary::ID;
    static const MIMETypeID INVALID_MIME_TYPE_ID { MIMETypeLibrary::INVALID_ID };

    MIMETypeID registerMIMEType(const MIMEType& mimeType, std::unique_ptr<Serializer::Factory>& supportedFactory);
    void unregisterMIMEType(const MIMETypeID& id);

    std::shared_ptr<Serializer> getSerializerForMIMEType(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url, const std::string& webMediaType) const;
    std::shared_ptr<Serializer> getSerializerForMIMETypeID(MIMETypeID id) const;

protected:
    MIMETypeLibrary _mimeTypeLibrary;
    class SupportedFormat {
    public:
        SupportedFormat(const MIMETypeID& mimeTypeID, std::unique_ptr<Serializer::Factory>& factory) :
            mimeTypeID(mimeTypeID),
            factory(std::move(factory)) {
        }
        MIMETypeID mimeTypeID;
        std::unique_ptr<Serializer::Factory> factory;
    };
    std::vector<SupportedFormat> _supportedFormats;
};

};

#endif // hifi_HFMFormatRegistry_h
