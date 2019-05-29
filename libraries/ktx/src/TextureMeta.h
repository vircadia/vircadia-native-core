//
//  TextureMeta.h
//  libraries/shared/src
//
//  Created by Ryan Huffman on 04/10/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextureMeta_h
#define hifi_TextureMeta_h

#include <type_traits>
#include <unordered_map>
#include <QUrl>

#include "khronos/KHR.h"

extern const QString TEXTURE_META_EXTENSION;
extern const uint16_t KTX_VERSION;

namespace std {
    template<> struct hash<khronos::gl::texture::InternalFormat> {
        using enum_type = std::underlying_type<khronos::gl::texture::InternalFormat>::type;
        typedef std::size_t result_type;
        result_type operator()(khronos::gl::texture::InternalFormat const& v) const noexcept {
            return std::hash<enum_type>()(static_cast<enum_type>(v));
        }
    };
}

struct TextureMeta {
    static bool deserialize(const QByteArray& data, TextureMeta* meta);
    QByteArray serialize();

    QUrl original;
    QUrl uncompressed;
    std::unordered_map<khronos::gl::texture::InternalFormat, QUrl> availableTextureTypes;
    uint16_t version { 0 };
};


#endif // hifi_TextureMeta_h
