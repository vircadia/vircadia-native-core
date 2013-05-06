//
//  EnvironmentData.h
//  interface
//
//  Created by Andrzej Kapolka on 5/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__EnvironmentData__
#define __interface__EnvironmentData__

#include <glm/glm.hpp>

class EnvironmentData {
public:

    void setAtmosphereCenter(const glm::vec3& center);
    void setAtmosphereInnerRadius(float radius);
    void setAtmosphereOuterRadius(float radius);
    
    void setRayleighScattering(float scattering);
    void setMieScattering(float scattering);
    
    void setSunLocation(const glm::vec3 location);
    void setSunBrightness(float brightness);

    int getBroadcastData(unsigned char* destinationBuffer) const;
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
private:

    glm::vec3 _atmosphereCenter;
    float _atmosphereInnerRadius;
    float _atmosphereOuterRadius;
    
    float _rayleighScattering;
    float _mieScattering;
    
    glm::vec3 _sunLocation;
    float _sunBrightness;
};

#endif /* defined(__interface__EnvironmentData__) */
