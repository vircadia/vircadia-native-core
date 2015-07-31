//
//  Environment.cpp
//  interface/src
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QByteArray>
#include <QMutexLocker>
#include <QtDebug>

#include "GeometryCache.h"
#include <GeometryUtil.h>
#include <NumericalConstants.h>
#include <OctreePacketData.h>
#include <udt/PacketHeaders.h>
#include <PathUtils.h>
#include <SharedUtil.h>

#include "Environment.h"

#include "SkyFromSpace_vert.h"
#include "SkyFromSpace_frag.h"
#include "SkyFromAtmosphere_vert.h"
#include "SkyFromAtmosphere_frag.h"

Environment::Environment()
    : _initialized(false) {
}

Environment::~Environment() {
}

void Environment::init() {
    if (_initialized) {
        return;
    }

    setupAtmosphereProgram(SkyFromSpace_vert, SkyFromSpace_frag, _skyFromSpaceProgram, _skyFromSpaceUniformLocations);
    setupAtmosphereProgram(SkyFromAtmosphere_vert, SkyFromAtmosphere_frag, _skyFromAtmosphereProgram, _skyFromAtmosphereUniformLocations);
    
    // start off with a default-constructed environment data
    _data[QUuid()][0];

    _initialized = true;
}

void Environment::setupAtmosphereProgram(const char* vertSource, const char* fragSource, gpu::PipelinePointer& pipeline, int* locations) {

    auto VS = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(vertSource)));
    auto PS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(fragSource)));
    
    gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(VS, PS));
    
    gpu::Shader::BindingSet slotBindings;
    gpu::Shader::makeProgram(*program, slotBindings);

    auto state = std::make_shared<gpu::State>();
    
    state->setCullMode(gpu::State::CULL_NONE);
    state->setDepthTest(false);
    state->setBlendFunction(true,
                            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    
    pipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));

    locations[CAMERA_POS_LOCATION] = program->getUniforms().findLocation("v3CameraPos");
    locations[LIGHT_POS_LOCATION] = program->getUniforms().findLocation("v3LightPos");
    locations[INV_WAVELENGTH_LOCATION] = program->getUniforms().findLocation("v3InvWavelength");
    locations[CAMERA_HEIGHT2_LOCATION] = program->getUniforms().findLocation("fCameraHeight2");
    locations[OUTER_RADIUS_LOCATION] = program->getUniforms().findLocation("fOuterRadius");
    locations[OUTER_RADIUS2_LOCATION] = program->getUniforms().findLocation("fOuterRadius2");
    locations[INNER_RADIUS_LOCATION] = program->getUniforms().findLocation("fInnerRadius");
    locations[KR_ESUN_LOCATION] = program->getUniforms().findLocation("fKrESun");
    locations[KM_ESUN_LOCATION] = program->getUniforms().findLocation("fKmESun");
    locations[KR_4PI_LOCATION] = program->getUniforms().findLocation("fKr4PI");
    locations[KM_4PI_LOCATION] = program->getUniforms().findLocation("fKm4PI");
    locations[SCALE_LOCATION] = program->getUniforms().findLocation("fScale");
    locations[SCALE_DEPTH_LOCATION] = program->getUniforms().findLocation("fScaleDepth");
    locations[SCALE_OVER_SCALE_DEPTH_LOCATION] = program->getUniforms().findLocation("fScaleOverScaleDepth");
    locations[G_LOCATION] = program->getUniforms().findLocation("g");
    locations[G2_LOCATION] = program->getUniforms().findLocation("g2");
}

void Environment::resetToDefault() {
    _data.clear();
    _data[QUuid()][0];
}

void Environment::renderAtmospheres(gpu::Batch& batch, ViewFrustum& camera) {
    // get the lock for the duration of the call
    QMutexLocker locker(&_mutex);

    if (_environmentIsOverridden) {
        renderAtmosphere(batch, camera, _overrideData);
    } else {
        foreach (const ServerData& serverData, _data) {
            // TODO: do something about EnvironmentData
            foreach (const EnvironmentData& environmentData, serverData) {
                renderAtmosphere(batch, camera, environmentData);
            }
        }
    }
}

EnvironmentData Environment::getClosestData(const glm::vec3& position) {
    if (_environmentIsOverridden) {
        return _overrideData;
    }
    
    // get the lock for the duration of the call
    QMutexLocker locker(&_mutex);
    
    EnvironmentData closest;
    float closestDistance = FLT_MAX;
    foreach (const ServerData& serverData, _data) {
        foreach (const EnvironmentData& environmentData, serverData) {
            float distance = glm::distance(position, environmentData.getAtmosphereCenter(position)) -
                environmentData.getAtmosphereOuterRadius();
            if (distance < closestDistance) {
                closest = environmentData;
                closestDistance = distance;
            }
        }
    }
    return closest;
}


// NOTE: Deprecated - I'm leaving this in for now, but it's not actually used. I made it private
// so that if anyone wants to start using this in the future they will consider how to make it
// work with new physics systems.
glm::vec3 Environment::getGravity (const glm::vec3& position) {
    //
    // 'Default' gravity pulls you downward in Y when you are near the X/Z plane
    const glm::vec3 DEFAULT_GRAVITY(0.0f, -1.0f, 0.0f);
    glm::vec3 gravity(DEFAULT_GRAVITY);
    float DEFAULT_SURFACE_RADIUS = 30.0f;
    float gravityStrength;
    
    //  Weaken gravity with height
    if (position.y > 0.0f) {
        gravityStrength = 1.0f / powf((DEFAULT_SURFACE_RADIUS + position.y) / DEFAULT_SURFACE_RADIUS, 2.0f);
        gravity *= gravityStrength;
    }
    
    // get the lock for the duration of the call
    QMutexLocker locker(&_mutex);
    
    foreach (const ServerData& serverData, _data) {
        foreach (const EnvironmentData& environmentData, serverData) {
            glm::vec3 vector = environmentData.getAtmosphereCenter(position) - position;
            float surfaceRadius = environmentData.getAtmosphereInnerRadius();
            if (glm::length(vector) <= surfaceRadius) {
                //  At or inside a planet, gravity is as set for the planet
                gravity += glm::normalize(vector) * environmentData.getGravity();
            } else {
                //  Outside a planet, the gravity falls off with distance
                gravityStrength = 1.0f / powf(glm::length(vector) / surfaceRadius, 2.0f);
                gravity += glm::normalize(vector) * environmentData.getGravity() * gravityStrength;
            }
        }
    }
    
    return gravity;
}

bool Environment::findCapsulePenetration(const glm::vec3& start, const glm::vec3& end,
                                         float radius, glm::vec3& penetration) {
    // collide with the "floor"
    bool found = findCapsulePlanePenetration(start, end, radius, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), penetration);
    
    glm::vec3 middle = (start + end) * 0.5f;
    
    // get the lock for the duration of the call
    QMutexLocker locker(&_mutex);
    
    foreach (const ServerData& serverData, _data) {
        foreach (const EnvironmentData& environmentData, serverData) {
            if (environmentData.getGravity() == 0.0f) {
                continue; // don't bother colliding with gravity-less environments
            }
            glm::vec3 environmentPenetration;
            if (findCapsuleSpherePenetration(start, end, radius, environmentData.getAtmosphereCenter(middle),
                    environmentData.getAtmosphereInnerRadius(), environmentPenetration)) {
                penetration = addPenetrations(penetration, environmentPenetration);
                found = true;
            }
        }
    }
    return found;
}

void Environment::renderAtmosphere(gpu::Batch& batch, ViewFrustum& camera, const EnvironmentData& data) {

    glm::vec3 center = data.getAtmosphereCenter();
    
    Transform transform;
    transform.setTranslation(center);
    batch.setModelTransform(transform);
    
    glm::vec3 relativeCameraPos = camera.getPosition() - center;
    float height = glm::length(relativeCameraPos);

    // use the appropriate shader depending on whether we're inside or outside
    int* locations;
    if (height < data.getAtmosphereOuterRadius()) {
        batch.setPipeline(_skyFromAtmosphereProgram);
        locations = _skyFromAtmosphereUniformLocations;
        
    } else {
        batch.setPipeline(_skyFromSpaceProgram);
        locations = _skyFromSpaceUniformLocations;
    }
    
    // the constants here are from Sean O'Neil's GPU Gems entry
    // (http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html), GameEngine.cpp
    batch._glUniform3f(locations[CAMERA_POS_LOCATION], relativeCameraPos.x, relativeCameraPos.y, relativeCameraPos.z);
    glm::vec3 lightDirection = glm::normalize(data.getSunLocation());
    batch._glUniform3f(locations[LIGHT_POS_LOCATION], lightDirection.x, lightDirection.y, lightDirection.z);
    batch._glUniform3f(locations[INV_WAVELENGTH_LOCATION],
                        1 / powf(data.getScatteringWavelengths().r, 4.0f),
                        1 / powf(data.getScatteringWavelengths().g, 4.0f),
                        1 / powf(data.getScatteringWavelengths().b, 4.0f));
    batch._glUniform1f(locations[CAMERA_HEIGHT2_LOCATION], height * height);
    batch._glUniform1f(locations[OUTER_RADIUS_LOCATION], data.getAtmosphereOuterRadius());
    batch._glUniform1f(locations[OUTER_RADIUS2_LOCATION], data.getAtmosphereOuterRadius() * data.getAtmosphereOuterRadius());
    batch._glUniform1f(locations[INNER_RADIUS_LOCATION], data.getAtmosphereInnerRadius());
    batch._glUniform1f(locations[KR_ESUN_LOCATION], data.getRayleighScattering() * data.getSunBrightness());
    batch._glUniform1f(locations[KM_ESUN_LOCATION], data.getMieScattering() * data.getSunBrightness());
    batch._glUniform1f(locations[KR_4PI_LOCATION], data.getRayleighScattering() * 4.0f * PI);
    batch._glUniform1f(locations[KM_4PI_LOCATION], data.getMieScattering() * 4.0f * PI);
    batch._glUniform1f(locations[SCALE_LOCATION], 1.0f / (data.getAtmosphereOuterRadius() - data.getAtmosphereInnerRadius()));
    batch._glUniform1f(locations[SCALE_DEPTH_LOCATION], 0.25f);
    batch._glUniform1f(locations[SCALE_OVER_SCALE_DEPTH_LOCATION],
        (1.0f / (data.getAtmosphereOuterRadius() - data.getAtmosphereInnerRadius())) / 0.25f);
    batch._glUniform1f(locations[G_LOCATION], -0.990f);
    batch._glUniform1f(locations[G2_LOCATION], -0.990f * -0.990f);

    DependencyManager::get<GeometryCache>()->renderSphere(batch,1.0f, 100, 50, glm::vec4(1.0f, 0.0f, 0.0f, 0.5f)); //Draw a unit sphere
}
