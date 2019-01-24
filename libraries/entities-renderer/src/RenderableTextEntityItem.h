//
//  RenderableTextEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableTextEntityItem_h
#define hifi_RenderableTextEntityItem_h

#include "RenderableEntityItem.h"

class TextEntityItem;
class TextRenderer3D;

namespace render { namespace entities {

class TextEntityRenderer : public TypedEntityRenderer<TextEntityItem> {
    using Parent = TypedEntityRenderer<TextEntityItem>;
    using Pointer = std::shared_ptr<TextEntityRenderer>;
public:
    TextEntityRenderer(const EntityItemPointer& entity);
    ~TextEntityRenderer();

    bool isTransparent() const override;
    ShapeKey getShapeKey() override;

    QSizeF textSize(const QString& text) const;

private:
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;

    int _geometryID{ 0 };
    std::shared_ptr<TextRenderer3D> _textRenderer;

    PulsePropertyGroup _pulseProperties;

    QString _text;
    float _lineHeight;
    glm::vec3 _textColor;
    float _textAlpha;
    glm::vec3 _backgroundColor;
    float _backgroundAlpha;

    float _leftMargin;
    float _rightMargin;
    float _topMargin;
    float _bottomMargin;

    BillboardMode _billboardMode;
    glm::vec3 _dimensions;
};

} }

#endif // hifi_RenderableTextEntityItem_h
