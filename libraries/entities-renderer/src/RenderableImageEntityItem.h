//
//  Created by Sam Gondelman on 11/29/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableImageEntityItem_h
#define hifi_RenderableImageEntityItem_h

#include "RenderableEntityItem.h"

#include <ImageEntityItem.h>

#include <procedural/Procedural.h>

namespace render { namespace entities {

class ImageEntityRenderer : public TypedEntityRenderer<ImageEntityItem> {
    using Parent = TypedEntityRenderer<ImageEntityItem>;
    using Pointer = std::shared_ptr<ImageEntityRenderer>;
public:
    ImageEntityRenderer(const EntityItemPointer& entity);
    ~ImageEntityRenderer();

    gpu::TexturePointer getTexture() override { return _texture ? _texture->getGPUTexture() : nullptr; }

protected:
    Item::Bound getBound(RenderArgs* args) override;
    ShapeKey getShapeKey() override;

    bool isTransparent() const override;

private:
    virtual bool needsRenderUpdate() const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;

    QString _imageURL;
    NetworkTexturePointer _texture;
    bool _textureIsLoaded { false };

    bool _emissive;
    bool _keepAspectRatio;
    QRect _subImage;

    std::shared_ptr<graphics::ProceduralMaterial> _material { std::make_shared<graphics::ProceduralMaterial>() };
    glm::vec3 _color { NAN };
    float _alpha { NAN };
    PulsePropertyGroup _pulseProperties;

    int _geometryId { 0 };
};

} }
#endif // hifi_RenderableImageEntityItem_h
