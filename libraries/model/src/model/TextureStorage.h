//
//  TextureStorage.h
//  libraries/model/src/model
//
//  Created by Sam Gateau on 5/6/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_TextureStorage_h
#define hifi_model_TextureStorage_h

#include "gpu/Texture.h"

#include "Material.h"

#include <qurl.h>

namespace model {

typedef glm::vec3 Color;

// TextureStorage is a specialized version of the gpu::Texture::Storage
// It provides the mechanism to create a texture from a Url and the intended usage 
// that guides the internal format used
class TextureStorage : public gpu::Texture::Storage {
public:
    TextureStorage(const QUrl& url, gpu::Texture::Type type = gpu::Texture::TEX_2D);
    ~TextureStorage();

    const QUrl& getUrl() const { return _url; }
    const gpu::Texture::Type getType() const { return _type; }
    const gpu::TexturePointer& getGPUTexture() const { return _gpuTexture; }

protected:
    gpu::TexturePointer _gpuTexture;
    QUrl _url;
    gpu::Texture::Type _type;

    void init();
};
typedef std::shared_ptr< TextureStorage > TextureStoragePointer;

};

#endif // hifi_model_TextureStorage_h

