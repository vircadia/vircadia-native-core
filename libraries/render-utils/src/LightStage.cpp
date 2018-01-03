//
//  LightStage.cpp
//  render-utils/src
//
//  Created by Zach Pomerantz on 1/14/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ViewFrustum.h"

#include "LightStage.h"

#include <cmath>

std::string LightStage::_stageName { "LIGHT_STAGE"};
const glm::mat4 LightStage::Shadow::_biasMatrix{
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0 };
const int LightStage::Shadow::MAP_SIZE = 1024;

static const auto MAX_BIAS = 0.006f;

const LightStage::Index LightStage::INVALID_INDEX { render::indexed_container::INVALID_INDEX };

LightStage::LightStage() {
}

LightStage::Shadow::Schema::Schema() {
    ShadowTransform defaultTransform;
    defaultTransform.bias = MAX_BIAS;
    std::fill(cascades, cascades + SHADOW_CASCADE_MAX_COUNT, defaultTransform);
    invMapSize = 1.0f / MAP_SIZE;
    cascadeCount = 1;
    invCascadeBlendWidth = 1.0f / 0.2f;
    invFalloffDistance = 1.0f / 2.0f;
    maxDistance = 20.0f;
}

LightStage::Shadow::Cascade::Cascade() : 
    _frustum{ std::make_shared<ViewFrustum>() },
    _minDistance{ 0.0f },
    _maxDistance{ 20.0f } {
    framebuffer = gpu::FramebufferPointer(gpu::Framebuffer::createShadowmap(MAP_SIZE));
    map = framebuffer->getDepthStencilBuffer();
}

const glm::mat4& LightStage::Shadow::Cascade::getView() const {
    return _frustum->getView();
}

const glm::mat4& LightStage::Shadow::Cascade::getProjection() const {
    return _frustum->getProjection();
}

float LightStage::Shadow::Cascade::computeFarDistance(const ViewFrustum& viewFrustum, const Transform& shadowViewInverse,
                                                      float left, float right, float bottom, float top, float viewMaxShadowDistance) const {
    // Far distance should be extended to the intersection of the infinitely extruded shadow frustum 
    // with the view frustum side planes. To do so, we generate 10 triangles in shadow space which are the result of
    // tesselating the side and far faces of the view frustum and clip them with the 4 side planes of the
    // shadow frustum. The resulting clipped triangle vertices with the farthest Z gives the desired
    // shadow frustum far distance.
    std::array<Triangle, 10> viewFrustumTriangles;
    Plane shadowClipPlanes[4] = {
        Plane(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, top, 0.0f)),
        Plane(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, bottom, 0.0f)),
        Plane(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(left, 0.0f, 0.0f)),
        Plane(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(right, 0.0f, 0.0f))
    };

    viewFrustum.tesselateSidesAndFar(shadowViewInverse, viewFrustumTriangles.data(), viewMaxShadowDistance);

    static const int MAX_TRIANGLE_COUNT = 16;
    auto far = 0.0f;

    for (auto& triangle : viewFrustumTriangles) {
        Triangle clippedTriangles[MAX_TRIANGLE_COUNT];
        auto clippedTriangleCount = clipTriangleWithPlanes(triangle, shadowClipPlanes, 4, clippedTriangles, MAX_TRIANGLE_COUNT);

        for (auto i = 0; i < clippedTriangleCount; i++) {
            const auto& clippedTriangle = clippedTriangles[i];
            far = glm::max(far, -clippedTriangle.v0.z);
            far = glm::max(far, -clippedTriangle.v1.z);
            far = glm::max(far, -clippedTriangle.v2.z);
        }
    }

    return far;
}

LightStage::Shadow::Shadow(model::LightPointer light, float maxDistance, unsigned int cascadeCount) : 
    _light{ light } {
    cascadeCount = std::min(cascadeCount, (unsigned int)SHADOW_CASCADE_MAX_COUNT);
    Schema schema;
    schema.cascadeCount = cascadeCount;
    _schemaBuffer = std::make_shared<gpu::Buffer>(sizeof(Schema), (const gpu::Byte*) &schema);
    _cascades.resize(cascadeCount);

    setMaxDistance(maxDistance);
}

void LightStage::Shadow::setMaxDistance(float value) {
    // This overlaping factor isn't really used directly for blending of shadow cascades. It
    // just there to be sure the cascades do overlap. The blending width used is relative
    // to the UV space and is set in the Schema with invCascadeBlendWidth.
    static const auto OVERLAP_FACTOR = 1.0f / 5.0f;

    _maxDistance = std::max(0.0f, value);

    if (_cascades.size() == 1) {
        _cascades.front().setMinDistance(0.0f);
        _cascades.front().setMaxDistance(_maxDistance);
    } else {
        // Distribute the cascades along that distance
        // TODO : these parameters should be exposed to the user as part of the light entity parameters, no?
        static const auto LOW_MAX_DISTANCE = 2.0f;
        static const auto MAX_RESOLUTION_LOSS = 0.6f; // Between 0 and 1, 0 giving tighter distributions

        // The max cascade distance is computed by multiplying the previous cascade's max distance by a certain
        // factor. There is a "user" factor that is computed from a desired max resolution loss in the shadow
        // and an optimal one based on the global min and max shadow distance, all cascades considered. The final
        // distance is a gradual blend between the two
        const auto userDistanceScale = 1.0f / (1.0f - MAX_RESOLUTION_LOSS);
        const auto optimalDistanceScale = powf(_maxDistance / LOW_MAX_DISTANCE, 1.0f / (_cascades.size() - 1));

        float maxCascadeUserDistance = LOW_MAX_DISTANCE;
        float maxCascadeOptimalDistance = LOW_MAX_DISTANCE;
        float minCascadeDistance = 0.0f;

        for (size_t cascadeIndex = 0; cascadeIndex < _cascades.size(); ++cascadeIndex) {
            float blendFactor = cascadeIndex / float(_cascades.size() - 1);
            float maxCascadeDistance;

            if (cascadeIndex == size_t(_cascades.size() - 1)) {
                maxCascadeDistance = _maxDistance;
            } else {
                maxCascadeDistance = maxCascadeUserDistance + (maxCascadeOptimalDistance - maxCascadeUserDistance)*blendFactor*blendFactor;
            }

            float shadowOverlapDistance = maxCascadeDistance * OVERLAP_FACTOR;

            _cascades[cascadeIndex].setMinDistance(minCascadeDistance);
            _cascades[cascadeIndex].setMaxDistance(maxCascadeDistance + shadowOverlapDistance);

            // Compute distances for next cascade
            minCascadeDistance = maxCascadeDistance;
            maxCascadeUserDistance = maxCascadeUserDistance * userDistanceScale;
            maxCascadeOptimalDistance = maxCascadeOptimalDistance * optimalDistanceScale;
            maxCascadeUserDistance = std::min(maxCascadeUserDistance, _maxDistance);
        }
    }

    // Update the buffer
    const auto& lastCascade = _cascades.back();
    auto& schema = _schemaBuffer.edit<Schema>();
    schema.maxDistance = _maxDistance;
    schema.invFalloffDistance = 1.0f / (OVERLAP_FACTOR*lastCascade.getMaxDistance());
}

void LightStage::Shadow::setKeylightFrustum(const ViewFrustum& viewFrustum,
                                            float nearDepth, float farDepth) {
    assert(nearDepth < farDepth);
    // Orient the keylight frustum
    auto lightDirection = glm::normalize(_light->getDirection());
    glm::quat orientation;
    if (lightDirection == IDENTITY_UP) {
        orientation = glm::quat(glm::mat3(-IDENTITY_RIGHT, IDENTITY_FORWARD, -IDENTITY_UP));
    } else if (lightDirection == -IDENTITY_UP) {
        orientation = glm::quat(glm::mat3(IDENTITY_RIGHT, IDENTITY_FORWARD, IDENTITY_UP));
    } else {
        auto side = glm::normalize(glm::cross(lightDirection, IDENTITY_UP));
        auto up = glm::normalize(glm::cross(side, lightDirection));
        orientation = glm::quat_cast(glm::mat3(side, up, -lightDirection));
    }

    // Position the keylight frustum
    auto position = viewFrustum.getPosition() - (nearDepth + farDepth)*lightDirection;
    for (auto& cascade : _cascades) {
        cascade._frustum->setOrientation(orientation);
        cascade._frustum->setPosition(position);
    }
    // Update the buffer
    auto& schema = _schemaBuffer.edit<Schema>();
    schema.lightDirInViewSpace = glm::inverse(viewFrustum.getView()) * glm::vec4(lightDirection, 0.f);
}

void LightStage::Shadow::setKeylightCascadeFrustum(unsigned int cascadeIndex, const ViewFrustum& viewFrustum,
                                            float nearDepth, float farDepth) {
    assert(nearDepth < farDepth);
    assert(cascadeIndex < _cascades.size());

    auto& cascade = _cascades[cascadeIndex];
    const auto viewMinCascadeShadowDistance = std::max(viewFrustum.getNearClip(), cascade.getMinDistance());
    const auto viewMaxCascadeShadowDistance = std::min(viewFrustum.getFarClip(), cascade.getMaxDistance());
    const auto viewMaxShadowDistance = _cascades.back().getMaxDistance();

    const Transform shadowView{ cascade._frustum->getView()};
    const Transform shadowViewInverse{ shadowView.getInverseMatrix() };

    auto nearCorners = viewFrustum.getCorners(viewMinCascadeShadowDistance);
    auto farCorners = viewFrustum.getCorners(viewMaxCascadeShadowDistance);

    vec3 min{ shadowViewInverse.transform(nearCorners.bottomLeft) };
    vec3 max{ min };
    // Expand keylight frustum  to fit view frustum
    auto fitFrustum = [&min, &max, &shadowViewInverse](const vec3& viewCorner) {
        const auto corner = shadowViewInverse.transform(viewCorner);

        min.x = glm::min(min.x, corner.x);
        min.y = glm::min(min.y, corner.y);
        min.z = glm::min(min.z, corner.z);

        max.x = glm::max(max.x, corner.x);
        max.y = glm::max(max.y, corner.y);
        max.z = glm::max(max.z, corner.z);
    };
    fitFrustum(nearCorners.bottomRight);
    fitFrustum(nearCorners.topLeft);
    fitFrustum(nearCorners.topRight);
    fitFrustum(farCorners.bottomLeft);
    fitFrustum(farCorners.bottomRight);
    fitFrustum(farCorners.topLeft);
    fitFrustum(farCorners.topRight);

    // Re-adjust near and far shadow distance
    auto near = glm::min(-max.z, nearDepth);
    auto far = cascade.computeFarDistance(viewFrustum, shadowViewInverse, min.x, max.x, min.y, max.y, viewMaxShadowDistance);

    glm::mat4 ortho = glm::ortho<float>(min.x, max.x, min.y, max.y, near, far);
    cascade._frustum->setProjection(ortho);

    // Calculate the frustum's internal state
    cascade._frustum->calculate();

    // Update the buffer
    auto& schema = _schemaBuffer.edit<Schema>();
    schema.cascades[cascadeIndex].reprojection = _biasMatrix * ortho * shadowViewInverse.getMatrix();
    // Adapt shadow bias to shadow resolution with a totally empirical formula
    const auto maxShadowFrustumDim = std::max(fabsf(min.x - max.x), fabsf(min.y - max.y));
    const auto REFERENCE_TEXEL_DENSITY = 7.5f;
    const auto cascadeTexelDensity = MAP_SIZE / maxShadowFrustumDim;
    schema.cascades[cascadeIndex].bias = MAX_BIAS * std::min(1.0f, REFERENCE_TEXEL_DENSITY / cascadeTexelDensity);
}

void LightStage::Shadow::setCascadeFrustum(unsigned int cascadeIndex, const ViewFrustum& shadowFrustum) {
    assert(cascadeIndex < _cascades.size());
    const Transform view{ shadowFrustum.getView() };
    const Transform viewInverse{ view.getInverseMatrix() };
    auto& cascade = _cascades[cascadeIndex];

    *cascade._frustum = shadowFrustum;
    // Update the buffer
    _schemaBuffer.edit<Schema>().cascades[cascadeIndex].reprojection = _biasMatrix * shadowFrustum.getProjection() * viewInverse.getMatrix();
}

LightStage::Index LightStage::findLight(const LightPointer& light) const {
    auto found = _lightMap.find(light);
    if (found != _lightMap.end()) {
        return INVALID_INDEX;
    } else {
        return (*found).second;
    }
}

LightStage::Index LightStage::addLight(const LightPointer& light) {
    auto found = _lightMap.find(light);
    if (found == _lightMap.end()) {
        auto lightId = _lights.newElement(light);
        // Avoid failing to allocate a light, just pass
        if (lightId != INVALID_INDEX) {

            // Allocate the matching Desc to the light
            if (lightId >= (Index) _descs.size()) {
                _descs.emplace_back(Desc());
            } else {
                assert(_descs[lightId].shadowId == INVALID_INDEX);
                _descs[lightId] = Desc();
            }

            // INsert the light and its index in the reverese map
            _lightMap.insert(LightMap::value_type(light, lightId));

            updateLightArrayBuffer(lightId);
        }
        return lightId;
    } else {
        return (*found).second;
    }
}

LightStage::Index LightStage::addShadow(Index lightIndex, float maxDistance, unsigned int cascadeCount) {
    auto light = getLight(lightIndex);
    Index shadowId = INVALID_INDEX;
    if (light) {
        assert(_descs[lightIndex].shadowId == INVALID_INDEX);
        shadowId = _shadows.newElement(std::make_shared<Shadow>(light, maxDistance, cascadeCount));
        _descs[lightIndex].shadowId = shadowId;
    }
    return shadowId;
}

LightStage::LightPointer LightStage::removeLight(Index index) {
    LightPointer removedLight = _lights.freeElement(index);
    if (removedLight) {
        auto shadowId = _descs[index].shadowId;
        // Remove shadow if one exists for this light
        if (shadowId != INVALID_INDEX) {
            auto removedShadow = _shadows.freeElement(shadowId);
            assert(removedShadow);
            assert(removedShadow->getLight() == removedLight);
        }
        _lightMap.erase(removedLight);
        _descs[index] = Desc();
    }
    assert(_descs.size() <= (size_t)index || _descs[index].shadowId == INVALID_INDEX);
    return removedLight;
}

LightStage::LightPointer LightStage::getCurrentKeyLight() const {
    Index keyLightId{ 0 };
    if (!_currentFrame._sunLights.empty()) {
        keyLightId = _currentFrame._sunLights.front();
    }
    return _lights.get(keyLightId);
}

LightStage::LightPointer LightStage::getCurrentAmbientLight() const {
    Index keyLightId{ 0 };
    if (!_currentFrame._ambientLights.empty()) {
        keyLightId = _currentFrame._ambientLights.front();
    }
    return _lights.get(keyLightId);
}

LightStage::ShadowPointer LightStage::getCurrentKeyShadow() const {
    Index keyLightId{ 0 };
    if (!_currentFrame._sunLights.empty()) {
        keyLightId = _currentFrame._sunLights.front();
    }
    auto shadow = getShadow(keyLightId);
    assert(shadow == nullptr || shadow->getLight() == getLight(keyLightId));
    return shadow;
}

LightStage::LightAndShadow LightStage::getCurrentKeyLightAndShadow() const {
    Index keyLightId{ 0 };
    if (!_currentFrame._sunLights.empty()) {
        keyLightId = _currentFrame._sunLights.front();
    }
    auto shadow = getShadow(keyLightId);
    auto light = getLight(keyLightId);
    assert(shadow == nullptr || shadow->getLight() == light);
    return LightAndShadow(light, shadow);
}

LightStage::Index LightStage::getShadowId(Index lightId) const {
    if (checkLightId(lightId)) {
        return _descs[lightId].shadowId;
    } else {
        return INVALID_INDEX;
    }
}

void LightStage::updateLightArrayBuffer(Index lightId) {
    auto lightSize = sizeof(model::Light::LightSchema);
    if (!_lightArrayBuffer) {
        _lightArrayBuffer = std::make_shared<gpu::Buffer>(lightSize);
    }

    assert(checkLightId(lightId));

    if (lightId > (Index)_lightArrayBuffer->getNumTypedElements<model::Light::LightSchema>()) {
        _lightArrayBuffer->resize(lightSize * (lightId + 10));
    }

    // lightArray is big enough so we can remap
    auto light = _lights._elements[lightId];
    if (light) {
        const auto& lightSchema = light->getLightSchemaBuffer().get();
        _lightArrayBuffer->setSubData<model::Light::LightSchema>(lightId, lightSchema);
    } else {
        // this should not happen ?
    }
}

LightStageSetup::LightStageSetup() {
}

void LightStageSetup::run(const render::RenderContextPointer& renderContext) {
    auto stage = renderContext->_scene->getStage(LightStage::getName());
    if (!stage) {
        stage = std::make_shared<LightStage>();
        renderContext->_scene->resetStage(LightStage::getName(), stage);
    }
}

