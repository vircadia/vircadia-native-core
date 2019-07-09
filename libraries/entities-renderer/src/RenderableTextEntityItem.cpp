//
//  RenderableTextEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableTextEntityItem.h"

#include <TextEntityItem.h>
#include <GeometryCache.h>
#include <PerfStat.h>
#include <Transform.h>
#include <TextRenderer3D.h>

#include "GLMHelpers.h"

#include "DeferredLightingEffect.h"

using namespace render;
using namespace render::entities;

static const int FIXED_FONT_POINT_SIZE = 40;
const int FIXED_FONT_SCALING_RATIO = FIXED_FONT_POINT_SIZE * 92.0f; // Determined through experimentation to fit font to line height.
const float LINE_SCALE_RATIO = 1.2f;

TextEntityRenderer::TextEntityRenderer(const EntityItemPointer& entity) :
    Parent(entity),
    _textRenderer(TextRenderer3D::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE / 2.0f)) {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        _geometryID = geometryCache->allocateID();
    }
}

TextEntityRenderer::~TextEntityRenderer() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_geometryID && geometryCache) {
        geometryCache->releaseID(_geometryID);
    }
}

bool TextEntityRenderer::isTransparent() const {
    return Parent::isTransparent() || _backgroundAlpha < 1.0f || _pulseProperties.getAlphaMode() != PulseMode::NONE;
}

bool TextEntityRenderer::isTextTransparent() const {
    return resultWithReadLock<bool>([&] {
        return Parent::isTransparent() || _textAlpha < 1.0f || _pulseProperties.getAlphaMode() != PulseMode::NONE;
    });
}

Item::Bound TextEntityRenderer::getBound() {
    auto bound = Parent::getBound();
    if (_billboardMode != BillboardMode::NONE) {
        glm::vec3 dimensions = bound.getScale();
        float max = glm::max(dimensions.x, glm::max(dimensions.y, dimensions.z));
        const float SQRT_2 = 1.41421356237f;
        bound.setScaleStayCentered(glm::vec3(SQRT_2 * max));
    }
    return bound;
}

ItemKey TextEntityRenderer::getKey() {
    return ItemKey::Builder(Parent::getKey()).withMetaCullGroup();
}

ShapeKey TextEntityRenderer::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withoutCullFace();
    if (isTransparent()) {
        builder.withTranslucent();
    }
    if (_unlit) {
        builder.withUnlit();
    }
    if (_primitiveMode == PrimitiveMode::LINES) {
        builder.withWireframe();
    }
    return builder.build();
}

uint32_t TextEntityRenderer::metaFetchMetaSubItems(ItemIDs& subItems) const {
    auto parentSubs = Parent::metaFetchMetaSubItems(subItems);
    if (Item::isValidID(_textRenderID)) {
        subItems.emplace_back(_textRenderID);
        return parentSubs + 1;
    }
    return parentSubs;
}

bool TextEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (_text != entity->getText()) {
        return true;
    }

    if (_lineHeight != entity->getLineHeight()) {
        return true;
    }

    if (_textColor != toGlm(entity->getTextColor())) {
        return true;
    }

    if (_textAlpha != entity->getTextAlpha()) {
        return true;
    }

    if (_backgroundColor != toGlm(entity->getBackgroundColor())) {
        return true;
    }

    if (_backgroundAlpha != entity->getBackgroundAlpha()) {
        return true;
    }

    if (_dimensions != entity->getScaledDimensions()) {
        return true;
    }

    if (_billboardMode != entity->getBillboardMode()) {
        return true;
    }

    if (_leftMargin != entity->getLeftMargin()) {
        return true;
    }

    if (_rightMargin != entity->getRightMargin()) {
        return true;
    }

    if (_topMargin != entity->getTopMargin()) {
        return true;
    }

    if (_bottomMargin != entity->getBottomMargin()) {
        return true;
    }

    if (_unlit != entity->getUnlit()) {
        return true;
    }

    if (_pulseProperties != entity->getPulseProperties()) {
        return true;
    }

    return false;
}

void TextEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    void* key = (void*)this;
    AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [this, entity] () {
        withWriteLock([&] {
            _dimensions = entity->getScaledDimensions();
            updateModelTransformAndBound();
            _renderTransform = getModelTransform();
            _renderTransform.postScale(_dimensions);
        });
    });
}

void TextEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    withWriteLock([&] {
        _pulseProperties = entity->getPulseProperties();
        _text = entity->getText();
        _lineHeight = entity->getLineHeight();
        _textColor = toGlm(entity->getTextColor());
        _textAlpha = entity->getTextAlpha();
        _backgroundColor = toGlm(entity->getBackgroundColor());
        _backgroundAlpha = entity->getBackgroundAlpha();
        _billboardMode = entity->getBillboardMode();
        _leftMargin = entity->getLeftMargin();
        _rightMargin = entity->getRightMargin();
        _topMargin = entity->getTopMargin();
        _bottomMargin = entity->getBottomMargin();
        _unlit = entity->getUnlit();
        updateTextRenderItem();
    });
}

void TextEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableTextEntityItem::render");
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;

    glm::vec4 backgroundColor;
    Transform modelTransform;
    BillboardMode billboardMode;
    PrimitiveMode primitiveMode;
    RenderLayer renderLayer;
    withReadLock([&] {
        modelTransform = _renderTransform;
        billboardMode = _billboardMode;
        primitiveMode = _primitiveMode;
        renderLayer = _renderLayer;

        float fadeRatio = _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f;
        backgroundColor = glm::vec4(_backgroundColor, fadeRatio * _backgroundAlpha);
        backgroundColor = EntityRenderer::calculatePulseColor(backgroundColor, _pulseProperties, _created);
    });

    if (backgroundColor.a <= 0.0f) {
        return;
    }

    modelTransform.setRotation(EntityItem::getBillboardRotation(modelTransform.getTranslation(), modelTransform.getRotation(), billboardMode, args->getViewFrustum().getPosition()));
    batch.setModelTransform(modelTransform);

    auto geometryCache = DependencyManager::get<GeometryCache>();
    // FIXME: we want to use instanced rendering here, but if textAlpha < 1 and backgroundAlpha < 1, the transparency sorting will be wrong
    //render::ShapePipelinePointer pipeline = geometryCache->getShapePipelinePointer(backgroundColor.a < 1.0f, _unlit,
    //    renderLayer != RenderLayer::WORLD || args->_renderMethod == Args::RenderMethod::FORWARD);
    //if (render::ShapeKey(args->_globalShapeKey).isWireframe() || primitiveMode == PrimitiveMode::LINES) {
    //    geometryCache->renderWireShapeInstance(args, batch, GeometryCache::Quad, backgroundColor, pipeline);
    //} else {
    //    geometryCache->renderSolidShapeInstance(args, batch, GeometryCache::Quad, backgroundColor, pipeline);
    //}

    geometryCache->renderQuad(batch, glm::vec2(-0.5), glm::vec2(0.5), backgroundColor, _geometryID);

    const int TRIANBLES_PER_QUAD = 2;
    args->_details._trianglesRendered += TRIANBLES_PER_QUAD;
}

QSizeF TextEntityRenderer::textSize(const QString& text) const {
    auto extents = _textRenderer->computeExtent(text);
    extents.y *= 2.0f;

    float maxHeight = (float)_textRenderer->computeExtent("Xy").y * LINE_SCALE_RATIO;
    float pointToWorldScale = (maxHeight / FIXED_FONT_SCALING_RATIO) * _lineHeight;

    return QSizeF(extents.x, extents.y) * pointToWorldScale;
}

void TextEntityRenderer::onAddToSceneTyped(const TypedEntityPointer& entity) {
    Parent::onAddToSceneTyped(entity);
    _textPayload = std::make_shared<TextPayload>(entity->getID(), _textRenderer);
    _textRenderID = AbstractViewStateInterface::instance()->getMain3DScene()->allocateID();
    auto renderPayload = std::make_shared<TextPayload::Payload>(_textPayload);
    render::Transaction transaction;
    transaction.resetItem(_textRenderID, renderPayload);
    AbstractViewStateInterface::instance()->getMain3DScene()->enqueueTransaction(transaction);
}

void TextEntityRenderer::onRemoveFromSceneTyped(const TypedEntityPointer& entity) {
    Parent::onRemoveFromSceneTyped(entity);
    render::Transaction transaction;
    transaction.removeItem(_textRenderID);
    AbstractViewStateInterface::instance()->getMain3DScene()->enqueueTransaction(transaction);
    _textPayload.reset();
}

void TextEntityRenderer::updateTextRenderItem() const {
    render::Transaction transaction;
    transaction.updateItem(_textRenderID);
    AbstractViewStateInterface::instance()->getMain3DScene()->enqueueTransaction(transaction);
}

entities::TextPayload::TextPayload(const QUuid& entityID, const std::weak_ptr<TextRenderer3D>& textRenderer) :
    _entityID(entityID), _textRenderer(textRenderer) {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        _geometryID = geometryCache->allocateID();
    }
}

entities::TextPayload::~TextPayload() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_geometryID && geometryCache) {
        geometryCache->releaseID(_geometryID);
    }
}

ItemKey entities::TextPayload::getKey() const {
    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
    if (entityTreeRenderer) {
        auto renderable = entityTreeRenderer->renderableForEntityId(_entityID);
        if (renderable) {
            auto textRenderable = std::static_pointer_cast<TextEntityRenderer>(renderable);
            ItemKey::Builder key;
            // Similar to EntityRenderer::getKey()
            if (textRenderable->isTextTransparent()) {
                key = ItemKey::Builder::transparentShape().withSubMetaCulled().withTagBits(textRenderable->getTagMask()).withLayer(textRenderable->getHifiRenderLayer());
            } else if (textRenderable->_canCastShadow) {
                key = ItemKey::Builder::opaqueShape().withSubMetaCulled().withTagBits(textRenderable->getTagMask()).withShadowCaster().withLayer(textRenderable->getHifiRenderLayer());
            } else {
                key = ItemKey::Builder::opaqueShape().withSubMetaCulled().withTagBits(textRenderable->getTagMask()).withLayer(textRenderable->getHifiRenderLayer());
            }

            if (!textRenderable->_visible) {
                key.withInvisible();
            }
            return key;
        }
    }
    return ItemKey::Builder::opaqueShape();
}

Item::Bound entities::TextPayload::getBound() const {
    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
    if (entityTreeRenderer) {
        auto renderable = entityTreeRenderer->renderableForEntityId(_entityID);
        if (renderable) {
            return std::static_pointer_cast<TextEntityRenderer>(renderable)->getBound();
        }
    }
    return Item::Bound();
}

ShapeKey entities::TextPayload::getShapeKey() const {
    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
    if (entityTreeRenderer) {
        auto renderable = entityTreeRenderer->renderableForEntityId(_entityID);
        if (renderable) {
            auto textRenderable = std::static_pointer_cast<TextEntityRenderer>(renderable);

            auto builder = render::ShapeKey::Builder().withOwnPipeline();
            if (textRenderable->isTextTransparent()) {
                builder.withTranslucent();
            }
            if (textRenderable->_unlit) {
                builder.withUnlit();
            }
            if (textRenderable->_primitiveMode == PrimitiveMode::LINES) {
                builder.withWireframe();
            }
            return builder.build();
        }
    }
    return ShapeKey::Builder::invalid();
}

void entities::TextPayload::render(RenderArgs* args) {
    PerformanceTimer perfTimer("TextPayload::render");
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;

    auto textRenderer = _textRenderer.lock();
    if (!textRenderer) {
        return;
    }

    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
    if (!entityTreeRenderer) {
        return;
    }
    auto renderable = entityTreeRenderer->renderableForEntityId(_entityID);
    if (!renderable) {
        return;
    }
    auto textRenderable = std::static_pointer_cast<TextEntityRenderer>(renderable);

    glm::vec4 textColor;
    Transform modelTransform;
    BillboardMode billboardMode;
    float lineHeight, leftMargin, rightMargin, topMargin, bottomMargin;
    QString text;
    glm::vec3 dimensions;
    bool forward;
    textRenderable->withReadLock([&] {
        modelTransform = textRenderable->_renderTransform;
        billboardMode = textRenderable->_billboardMode;
        lineHeight = textRenderable->_lineHeight;
        leftMargin = textRenderable->_leftMargin;
        rightMargin = textRenderable->_rightMargin;
        topMargin = textRenderable->_topMargin;
        bottomMargin = textRenderable->_bottomMargin;
        text = textRenderable->_text;
        dimensions = textRenderable->_dimensions;

        float fadeRatio = textRenderable->_isFading ? Interpolate::calculateFadeRatio(textRenderable->_fadeStartTime) : 1.0f;
        textColor = glm::vec4(textRenderable->_textColor, fadeRatio * textRenderable->_textAlpha);
        textColor = EntityRenderer::calculatePulseColor(textColor, textRenderable->_pulseProperties, textRenderable->_created);
        forward = textRenderable->_renderLayer != RenderLayer::WORLD || args->_renderMethod == render::Args::FORWARD;
    });

    if (textColor.a <= 0.0f) {
        return;
    }

    modelTransform.setRotation(EntityItem::getBillboardRotation(modelTransform.getTranslation(), modelTransform.getRotation(), billboardMode, args->getViewFrustum().getPosition()));

    float scale = lineHeight / textRenderer->getFontSize();
    modelTransform.postTranslate(glm::vec3(-0.5, 0.5, 1.0f + EPSILON / dimensions.z));
    modelTransform.setScale(scale);
    batch.setModelTransform(modelTransform);

    glm::vec2 bounds = glm::vec2(dimensions.x - (leftMargin + rightMargin), dimensions.y - (topMargin + bottomMargin));
    textRenderer->draw(batch, leftMargin / scale, -topMargin / scale, text, textColor, bounds / scale, textRenderable->_unlit, forward);
}

namespace render {
template <> const ItemKey payloadGetKey(const TextPayload::Pointer& payload) {
    if (payload) {
        return payload->getKey();
    }
    return ItemKey::Builder::opaqueShape();
}

template <> const Item::Bound payloadGetBound(const TextPayload::Pointer& payload) {
    if (payload) {
        return payload->getBound();
    }
    return Item::Bound();
}

template <> const ShapeKey shapeGetShapeKey(const TextPayload::Pointer& payload) {
    if (payload) {
        return payload->getShapeKey();
    }
    return ShapeKey::Builder::invalid();
}

template <> void payloadRender(const TextPayload::Pointer& payload, RenderArgs* args) {
    return payload->render(args);
}
}
