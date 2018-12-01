//
//  HFMSimpleFormat.cpp
//  libraries/hfm/src/hfm
//
//  Created by Sabrina Shanman on 2018/11/30.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HFMSimpleFormat.h"

namespace hfm {
    template<typename T>
    std::shared_ptr<Serializer> SimpleFactory<T>::get() {
        return std::make_shared<T>();
    }

    template<typename T>
    SimpleFormat<T>::SimpleFormat(const MIMEType& mimeType) : Format(),
        _mimeType(mimeType) {
    }

    template<typename T>
    void SimpleFormat<T>::registerFormat(FormatRegistry& registry) {
        _mimeTypeID = registry.registerMIMEType(_mimeType, std::make_shared<SimpleFactory<T>>());
    }

    template<typename T>
    void SimpleFormat<T>::unregisterFormat(FormatRegistry& registry) {
        registry.unregisterMIMEType(_mimeTypeID);
        _mimeTypeID = hfm::FormatRegistry::INVALID_MIME_TYPE_ID;
    }
};
