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
#include "RenderPipelines.h"

using namespace render;
using namespace render::entities;

static const int FIXED_FONT_POINT_SIZE = 40;
const int FIXED_FONT_SCALING_RATIO = FIXED_FONT_POINT_SIZE * 92.0f; // Determined through experimentation to fit font to line height.
const float LINE_SCALE_RATIO = 1.2f;

TextEntityRenderer::TextEntityRenderer(const EntityItemPointer& entity) :
    Parent(entity),
    _textRenderer(TextRenderer3D::getInstance(ROBOTO_FONT_FAMILY)) {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        _geometryID = geometryCache->allocateID();
    }
    _material->setCullFaceMode(graphics::MaterialKey::CullFaceMode::CULL_NONE);
    addMaterial(graphics::MaterialLayer(_material, 0), "0");
}

TextEntityRenderer::~TextEntityRenderer() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_geometryID && geometryCache) {
        geometryCache->releaseID(_geometryID);
    }
}

bool TextEntityRenderer::needsRenderUpdate() const {
    return needsRenderUpdateFromMaterials() || Parent::needsRenderUpdate();
}

void TextEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    void* key = (void*)this;
    AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [this, entity] {
        withWriteLock([&] {
            _dimensions = entity->getScaledDimensions();
            _renderTransform = getModelTransform();
            _renderTransform.postScale(_dimensions);
        });
    });
}

void TextEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    _pulseProperties = entity->getPulseProperties();
    _text = entity->getText();
    _lineHeight = entity->getLineHeight();
    _textColor = toGlm(entity->getTextColor());
    _textAlpha = entity->getTextAlpha();
    _leftMargin = entity->getLeftMargin();
    _rightMargin = entity->getRightMargin();
    _topMargin = entity->getTopMargin();
    _bottomMargin = entity->getBottomMargin();
    _font = entity->getFont();
    _effect = entity->getTextEffect();
    _effectColor = toGlm(entity->getTextEffectColor());
    _effectThickness = entity->getTextEffectThickness();
    _alignment = entity->getAlignment();

    bool materialChanged = false;
    glm::vec3 color = toGlm(entity->getBackgroundColor());
    if (_backgroundColor != color) {
        _backgroundColor = color;
        _material->setAlbedo(color);
        materialChanged = true;
    }

    float alpha = entity->getBackgroundAlpha();
    if (_backgroundAlpha != alpha) {
        _backgroundAlpha = alpha;
        _material->setOpacity(alpha);
        materialChanged = true;
    }

    auto unlit = entity->getUnlit();
    if (_unlit != unlit) {
        _unlit = unlit;
        _material->setUnlit(_unlit);
        materialChanged = true;
    }

    updateMaterials(materialChanged);

    updateTextRenderItem();
}

bool TextEntityRenderer::isTransparent() const {
    bool backgroundTransparent = _backgroundAlpha < 1.0f || _pulseProperties.getAlphaMode() != PulseMode::NONE;
    return backgroundTransparent || Parent::isTransparent() || materialsTransparent();
}

bool TextEntityRenderer::isTextTransparent() const {
    return Parent::isTransparent() || _textAlpha < 1.0f || _pulseProperties.getAlphaMode() != PulseMode::NONE;
}

Item::Bound TextEntityRenderer::getBound(RenderArgs* args) {
    return Parent::getMaterialBound(args);
}

ItemKey TextEntityRenderer::getKey() {
    return ItemKey::Builder(Parent::getKey()).withMetaCullGroup();
}

ShapeKey TextEntityRenderer::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withDepthBias();
    updateShapeKeyBuilderFromMaterials(builder);
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

void TextEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableTextEntityItem::render");
    Q_ASSERT(args->_batch);

    graphics::MultiMaterial materials;
    {
        std::lock_guard<std::mutex> lock(_materialsLock);
        materials = _materials["0"];
    }

    auto& schema = materials.getSchemaBuffer().get<graphics::MultiMaterial::Schema>();
    glm::vec4 backgroundColor = glm::vec4(ColorUtils::tosRGBVec3(schema._albedo), schema._opacity);
    backgroundColor = EntityRenderer::calculatePulseColor(backgroundColor, _pulseProperties, _created);

    if (backgroundColor.a <= 0.0f) {
        return;
    }

    gpu::Batch& batch = *args->_batch;

    bool transparent;
    Transform transform;
    withReadLock([&] {
        transparent = isTransparent();
        transform = _renderTransform;
    });

    transform.setRotation(BillboardModeHelpers::getBillboardRotation(transform.getTranslation(), transform.getRotation(), _billboardMode,
        args->_renderMode == RenderArgs::RenderMode::SHADOW_RENDER_MODE ? BillboardModeHelpers::getPrimaryViewFrustumPosition() : args->getViewFrustum().getPosition()));
    batch.setModelTransform(transform);

    Pipeline pipelineType = getPipelineType(materials);
    if (pipelineType == Pipeline::PROCEDURAL) {
        auto procedural = std::static_pointer_cast<graphics::ProceduralMaterial>(materials.top().material);
        transparent |= procedural->isFading();
        procedural->prepare(batch, transform.getTranslation(), transform.getScale(), transform.getRotation(), _created, ProceduralProgramKey(transparent));
    } else if (pipelineType == Pipeline::MATERIAL) {
        if (RenderPipelines::bindMaterials(materials, batch, args->_renderMode, args->_enableTexturing)) {
            args->_details._materialSwitches++;
        }
    }

    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (pipelineType == Pipeline::SIMPLE) {
        geometryCache->renderQuad(batch, glm::vec2(-0.5f), glm::vec2(0.5f), backgroundColor, _geometryID);
    } else {
        geometryCache->renderQuad(batch, glm::vec2(-0.5f), glm::vec2(0.5f), glm::vec2(0.0f), glm::vec2(1.0f), backgroundColor, _geometryID);
    }

    const int TRIANGLES_PER_QUAD = 2;
    args->_details._trianglesRendered += TRIANGLES_PER_QUAD;
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

            // Similar to RenderableEntityItem::getKey()
            ItemKey::Builder builder = ItemKey::Builder().withTypeShape().withTypeMeta().withTagBits(textRenderable->getTagMask()).withLayer(textRenderable->getHifiRenderLayer());
            builder.withSubMetaCulled();

            if (textRenderable->isTextTransparent()) {
                builder.withTransparent();
            } else if (textRenderable->_canCastShadow) {
                builder.withShadowCaster();
            }

            if (!textRenderable->_visible) {
                builder.withInvisible();
            }

            return builder;
        }
    }
    return ItemKey::Builder::opaqueShape();
}

Item::Bound entities::TextPayload::getBound(RenderArgs* args) const {
    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
    if (entityTreeRenderer) {
        auto renderable = entityTreeRenderer->renderableForEntityId(_entityID);
        if (renderable) {
            return std::static_pointer_cast<TextEntityRenderer>(renderable)->getBound(args);
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

bool entities::TextPayload::passesZoneOcclusionTest(const std::unordered_set<QUuid>& containingZones) const {
    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
    if (entityTreeRenderer) {
        auto renderable = entityTreeRenderer->renderableForEntityId(_entityID);
        if (renderable) {
            return std::static_pointer_cast<TextEntityRenderer>(renderable)->passesZoneOcclusionTest(containingZones);
        }
    }
    return false;
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

    Transform transform;
    glm::vec3 dimensions;

    glm::vec4 textColor;
    textRenderable->withReadLock([&] {
        transform = textRenderable->_renderTransform;
        dimensions = textRenderable->_dimensions;

        float fadeRatio = textRenderable->_isFading ? Interpolate::calculateFadeRatio(textRenderable->_fadeStartTime) : 1.0f;
        textColor = glm::vec4(textRenderable->_textColor, fadeRatio * textRenderable->_textAlpha);
    });

    bool forward = textRenderable->_renderLayer != RenderLayer::WORLD || args->_renderMethod == render::Args::FORWARD;

    textColor = EntityRenderer::calculatePulseColor(textColor, textRenderable->_pulseProperties, textRenderable->_created);
    glm::vec3 effectColor = EntityRenderer::calculatePulseColor(textRenderable->_effectColor, textRenderable->_pulseProperties, textRenderable->_created);

    if (textColor.a <= 0.0f) {
        return;
    }

    transform.setRotation(BillboardModeHelpers::getBillboardRotation(transform.getTranslation(), transform.getRotation(), textRenderable->_billboardMode,
        args->_renderMode == RenderArgs::RenderMode::SHADOW_RENDER_MODE ? BillboardModeHelpers::getPrimaryViewFrustumPosition() : args->getViewFrustum().getPosition()));

    float scale = textRenderable->_lineHeight / textRenderer->getFontSize();
    transform.postTranslate(glm::vec3(-0.5, 0.5, 1.0f + EPSILON / dimensions.z));
    transform.setScale(scale);
    batch.setModelTransform(transform);

    glm::vec2 bounds = glm::vec2(dimensions.x - (textRenderable->_leftMargin + textRenderable->_rightMargin), dimensions.y - (textRenderable->_topMargin + textRenderable->_bottomMargin));
    textRenderer->draw(batch, textRenderable->_leftMargin / scale, -textRenderable->_topMargin / scale, bounds / scale, scale,
                       textRenderable->_text, textRenderable->_font, textColor, effectColor, textRenderable->_effectThickness, textRenderable->_effect,
                       textRenderable->_alignment, textRenderable->_unlit, forward);
}

namespace render {
template <> const ItemKey payloadGetKey(const TextPayload::Pointer& payload) {
    if (payload) {
        return payload->getKey();
    }
    return ItemKey::Builder::opaqueShape();
}

template <> const Item::Bound payloadGetBound(const TextPayload::Pointer& payload, RenderArgs* args) {
    if (payload) {
        return payload->getBound(args);
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

template <> bool payloadPassesZoneOcclusionTest(const entities::TextPayload::Pointer& payload, const std::unordered_set<QUuid>& containingZones) {
    if (payload) {
        return payload->passesZoneOcclusionTest(containingZones);
    }
    return false;
}

}
