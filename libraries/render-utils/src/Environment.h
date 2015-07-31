//
//  Environment.h
//  interface/src
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Environment_h
#define hifi_Environment_h

#include <QHash>
#include <QMutex>

#include <HifiSockAddr.h>
#include <gpu/Batch.h>

#include <EnvironmentData.h>

class ViewFrustum;

class Environment {
public:
    Environment();
    ~Environment();

    void init();
    void resetToDefault();
    void renderAtmospheres(gpu::Batch& batch, ViewFrustum& camera);

    void override(const EnvironmentData& overrideData) { _overrideData = overrideData; _environmentIsOverridden = true; }
    void endOverride() { _environmentIsOverridden = false; }
    
    EnvironmentData getClosestData(const glm::vec3& position);

private:
    glm::vec3 getGravity (const glm::vec3& position); // NOTE: Deprecated
    bool findCapsulePenetration(const glm::vec3& start,
            const glm::vec3& end, float radius, glm::vec3& penetration); // NOTE: Deprecated

    void renderAtmosphere(gpu::Batch& batch, ViewFrustum& camera, const EnvironmentData& data);

    bool _initialized;

    enum {
        CAMERA_POS_LOCATION,
        LIGHT_POS_LOCATION,
        INV_WAVELENGTH_LOCATION,
        CAMERA_HEIGHT2_LOCATION,
        OUTER_RADIUS_LOCATION,
        OUTER_RADIUS2_LOCATION,
        INNER_RADIUS_LOCATION,
        KR_ESUN_LOCATION,
        KM_ESUN_LOCATION,
        KR_4PI_LOCATION,
        KM_4PI_LOCATION,
        SCALE_LOCATION,
        SCALE_DEPTH_LOCATION,
        SCALE_OVER_SCALE_DEPTH_LOCATION,
        G_LOCATION,
        G2_LOCATION,
        LOCATION_COUNT
    };
    
    void setupAtmosphereProgram(const char* vertSource, const char* fragSource, gpu::PipelinePointer& pipelineProgram, int* locations);


    gpu::PipelinePointer _skyFromAtmosphereProgram;
    gpu::PipelinePointer _skyFromSpaceProgram;
    int _skyFromAtmosphereUniformLocations[LOCATION_COUNT];
    int _skyFromSpaceUniformLocations[LOCATION_COUNT];
    
    typedef QHash<int, EnvironmentData> ServerData;
    
    QHash<QUuid, ServerData> _data;
    EnvironmentData _overrideData;
    bool _environmentIsOverridden = false;
    
    QMutex _mutex;
};

#endif // hifi_Environment_h
