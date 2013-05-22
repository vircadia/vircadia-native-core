//
//  Environment.h
//  interface
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Environment__
#define __interface__Environment__

#include <QHash>
#include <QMutex>

#include <UDPSocket.h>

#include "EnvironmentData.h"
#include "InterfaceConfig.h"

class Camera;
class ProgramObject;

class Environment {
public:

    void init();
    void renderAtmospheres(Camera& camera);
    
    glm::vec3 getGravity (const glm::vec3& position);
    const EnvironmentData getClosestData(const glm::vec3& position);
    
    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration);
    
    int parseData(sockaddr *senderAddress, unsigned char* sourceBuffer, int numBytes);
    
private:

    ProgramObject* createSkyProgram(const char* from, int* locations);

    void renderAtmosphere(Camera& camera, const EnvironmentData& data);

    ProgramObject* _skyFromAtmosphereProgram;
    ProgramObject* _skyFromSpaceProgram;
    
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
    
    int _skyFromAtmosphereUniformLocations[LOCATION_COUNT];
    int _skyFromSpaceUniformLocations[LOCATION_COUNT];
    
    typedef QHash<int, EnvironmentData> ServerData;
    
    QHash<sockaddr, ServerData> _data;
    
    QMutex _mutex;
};

#endif /* defined(__interface__Environment__) */
