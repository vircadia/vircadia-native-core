//
//  RenderablePolyLineEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Eric Levin on 8/10/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderablePolyLineEntityItem.h"
#include <ParticleEffectEntityItem.h>

#include <GeometryCache.h>
#include <StencilMaskPass.h>
#include <TextureCache.h>
#include <PathUtils.h>
#include <PerfStat.h>

//#define POLYLINE_ENTITY_USE_FADE_EFFECT
#ifdef POLYLINE_ENTITY_USE_FADE_EFFECT
#   include <FadeEffect.h>
#endif

#include "paintStroke_vert.h"
#include "paintStroke_frag.h"

#include "paintStroke_fade_vert.h"
#include "paintStroke_fade_frag.h"

using namespace render;
using namespace render::entities;

static uint8_t CUSTOM_PIPELINE_NUMBER { 0 };
static const int32_t PAINTSTROKE_TEXTURE_SLOT{ 0 };
static const int32_t PAINTSTROKE_UNIFORM_SLOT{ 0 };
static gpu::Stream::FormatPointer polylineFormat;
static gpu::PipelinePointer polylinePipeline;
#ifdef POLYLINE_ENTITY_USE_FADE_EFFECT
static gpu::PipelinePointer polylineFadePipeline;
#endif

struct PolyLineUniforms {
    glm::vec3 color;
};

static render::ShapePipelinePointer shapePipelineFactory(const render::ShapePlumber& plumber, const render::ShapeKey& key) {
    if (!polylinePipeline) {
        auto VS = gpu::Shader::createVertex(std::string(paintStroke_vert));
        auto PS = gpu::Shader::createPixel(std::string(paintStroke_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(VS, PS);
#ifdef POLYLINE_ENTITY_USE_FADE_EFFECT
        auto fadeVS = gpu::Shader::createVertex(std::string(paintStroke_fade_vert));
        auto fadePS = gpu::Shader::createPixel(std::string(paintStroke_fade_frag));
        gpu::ShaderPointer fadeProgram = gpu::Shader::createProgram(fadeVS, fadePS);
#endif
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("originalTexture"), PAINTSTROKE_TEXTURE_SLOT));
        slotBindings.insert(gpu::Shader::Binding(std::string("polyLineBuffer"), PAINTSTROKE_UNIFORM_SLOT));
        gpu::Shader::makeProgram(*program, slotBindings);
#ifdef POLYLINE_ENTITY_USE_FADE_EFFECT
        slotBindings.insert(gpu::Shader::Binding(std::string("fadeMaskMap"), PAINTSTROKE_TEXTURE_SLOT + 1));
        slotBindings.insert(gpu::Shader::Binding(std::string("fadeParametersBuffer"), PAINTSTROKE_UNIFORM_SLOT + 1));
        gpu::Shader::makeProgram(*fadeProgram, slotBindings);
#endif
        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        PrepareStencil::testMask(*state);
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        polylinePipeline = gpu::Pipeline::create(program, state);
#ifdef POLYLINE_ENTITY_USE_FADE_EFFECT
        _fadePipeline = gpu::Pipeline::create(fadeProgram, state);
#endif
    }

#ifdef POLYLINE_ENTITY_USE_FADE_EFFECT
    if (key.isFaded()) {
        auto fadeEffect = DependencyManager::get<FadeEffect>();
        return std::make_shared<render::ShapePipeline>(_fadePipeline, nullptr, fadeEffect->getBatchSetter(), fadeEffect->getItemUniformSetter());
    } else {
#endif
        return std::make_shared<render::ShapePipeline>(polylinePipeline, nullptr, nullptr, nullptr);
#ifdef POLYLINE_ENTITY_USE_FADE_EFFECT
    }
#endif
}

PolyLineEntityRenderer::PolyLineEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {
    static std::once_flag once;
    std::call_once(once, [&] {
        CUSTOM_PIPELINE_NUMBER = render::ShapePipeline::registerCustomShapePipelineFactory(shapePipelineFactory);
        polylineFormat.reset(new gpu::Stream::Format());
        polylineFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), offsetof(Vertex, position));
        polylineFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), offsetof(Vertex, normal));
        polylineFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), offsetof(Vertex, uv));
    });

    PolyLineUniforms uniforms;
    _uniformBuffer = std::make_shared<gpu::Buffer>(sizeof(PolyLineUniforms), (const gpu::Byte*) &uniforms);
    _verticesBuffer = std::make_shared<gpu::Buffer>();
}

ItemKey PolyLineEntityRenderer::getKey() {
    return ItemKey::Builder::transparentShape().withTypeMeta();
}

ShapeKey PolyLineEntityRenderer::getShapeKey() {
    return ShapeKey::Builder().withCustom(CUSTOM_PIPELINE_NUMBER).build();
}

bool PolyLineEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    return (
        entity->pointsChanged() ||
        entity->strokeWidthsChanged() ||
        entity->normalsChanged() ||
        entity->texturesChanged()
    );
}

void PolyLineEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    static const QUrl DEFAULT_POLYLINE_TEXTURE = QUrl(PathUtils::resourcesPath() + "images/paintStroke.png");
    QUrl entityTextures = DEFAULT_POLYLINE_TEXTURE;
    if (entity->texturesChanged()) {
        entity->resetTexturesChanged();
        auto textures = entity->getTextures();
        if (!textures.isEmpty()) {
            entityTextures = QUrl(textures);
        }
    }

    if (!_texture || _texture->getURL() != entityTextures) {
        _texture = DependencyManager::get<TextureCache>()->getTexture(entityTextures);
    }
}

void PolyLineEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    PolyLineUniforms uniforms;
    uniforms.color = toGlm(entity->getXColor());
    memcpy(&_uniformBuffer.edit<PolyLineUniforms>(), &uniforms, sizeof(PolyLineUniforms));
    auto pointsChanged = entity->pointsChanged();
    auto strokeWidthsChanged = entity->strokeWidthsChanged();
    auto normalsChanged = entity->normalsChanged();
    entity->resetPolyLineChanged();

    _polylineTransform = Transform();
    _polylineTransform.setTranslation(entity->getPosition());
    _polylineTransform.setRotation(entity->getRotation());

    if (pointsChanged) {
        _lastPoints = entity->getLinePoints();
    }
    if (strokeWidthsChanged) {
        _lastStrokeWidths = entity->getStrokeWidths();
    }
    if (normalsChanged) {
        _lastNormals = entity->getNormals();
    }
    if (pointsChanged || strokeWidthsChanged || normalsChanged) {
        _empty = std::min(_lastPoints.size(), std::min(_lastNormals.size(), _lastStrokeWidths.size())) < 2;
        if (!_empty) {
            updateGeometry(updateVertices(_lastPoints, _lastNormals, _lastStrokeWidths));
        }
    }
}

void PolyLineEntityRenderer::updateGeometry(const std::vector<Vertex>& vertices) {
    _numVertices = (uint32_t)vertices.size();
    auto bufferSize = _numVertices * sizeof(Vertex);
    if (bufferSize > _verticesBuffer->getSize()) {
        _verticesBuffer->resize(bufferSize);
    }
    _verticesBuffer->setSubData(0, vertices);
}

std::vector<PolyLineEntityRenderer::Vertex> PolyLineEntityRenderer::updateVertices(const QVector<glm::vec3>& points, const QVector<glm::vec3>& normals, const QVector<float>& strokeWidths) {
    // Calculate the minimum vector size out of normals, points, and stroke widths
    int size = std::min(points.size(), std::min(normals.size(), strokeWidths.size()));

    std::vector<Vertex> vertices;

    // Guard against an empty polyline
    if (size <= 0) {
        return vertices;
    }

    float uCoordInc = 1.0f / size;
    float uCoord = 0.0f;
    int finalIndex = size - 1;
    glm::vec3 binormal;
    for (int i = 0; i <= finalIndex; i++) {
        const float& width = strokeWidths.at(i);
        const auto& point = points.at(i);
        const auto& normal = normals.at(i);

        // For last point we can assume binormals are the same since it represents the last two vertices of quad
        if (i < finalIndex) {
            const auto tangent = points.at(i + 1) - point;
            binormal = glm::normalize(glm::cross(tangent, normal)) * width;

            // Check to make sure binormal is not a NAN. If it is, don't add to vertices vector
            if (binormal.x != binormal.x) {
                continue;
            }
        }

        const auto v1 = point + binormal;
        const auto v2 = point - binormal;
        vertices.emplace_back(v1, normal, vec2(uCoord, 0.0f));
        vertices.emplace_back(v2, normal, vec2(uCoord, 1.0f));
        uCoord += uCoordInc;
    }

    return vertices;
}


void PolyLineEntityRenderer::doRender(RenderArgs* args) {
    if (_empty) {
        return;
    }

    PerformanceTimer perfTimer("RenderablePolyLineEntityItem::render");
    Q_ASSERT(args->_batch);

    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(_polylineTransform);
    batch.setUniformBuffer(PAINTSTROKE_UNIFORM_SLOT, _uniformBuffer);

    if (_texture && _texture->isLoaded()) {
        batch.setResourceTexture(PAINTSTROKE_TEXTURE_SLOT, _texture->getGPUTexture());
    } else {
        batch.setResourceTexture(PAINTSTROKE_TEXTURE_SLOT, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }

    batch.setInputFormat(polylineFormat);
    batch.setInputBuffer(0, _verticesBuffer, 0, sizeof(Vertex));

#ifndef POLYLINE_ENTITY_USE_FADE_EFFECT
    if (_isFading) {
        batch._glColor4f(1.0f, 1.0f, 1.0f, Interpolate::calculateFadeRatio(_fadeStartTime));
    } else {
        batch._glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
#endif

    batch.draw(gpu::TRIANGLE_STRIP, _numVertices, 0);
}
