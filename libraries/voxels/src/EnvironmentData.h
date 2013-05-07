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

    EnvironmentData();

    void setAtmosphereCenter(const glm::vec3& center) { _atmosphereCenter = center; }
    void setAtmosphereInnerRadius(float radius) { _atmosphereInnerRadius = radius; }
    void setAtmosphereOuterRadius(float radius) { _atmosphereOuterRadius = radius; }
    const glm::vec3& getAtmosphereCenter() const { return _atmosphereCenter; }
    float getAtmosphereInnerRadius() const { return _atmosphereInnerRadius; }
    float getAtmosphereOuterRadius() const { return _atmosphereOuterRadius; }
    
    void setRayleighScattering(float scattering) { _rayleighScattering = scattering; }
    void setMieScattering(float scattering) { _mieScattering = scattering; }
    float getRayleighScattering() const { return _rayleighScattering; }
    float getMieScattering() const { return _mieScattering; }
    
    void setScatteringWavelengths(const glm::vec3& wavelengths) { _scatteringWavelengths = wavelengths; }
    const glm::vec3& getScatteringWavelengths() const { return _scatteringWavelengths; }
    
    void setSunLocation(const glm::vec3& location) { _sunLocation = location; }
    void setSunBrightness(float brightness) { _sunBrightness = brightness; }
    const glm::vec3& getSunLocation() const { return _sunLocation; }
    float getSunBrightness() const { return _sunBrightness; }

    int getBroadcastData(unsigned char* destinationBuffer) const;
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
private:

    glm::vec3 _atmosphereCenter;
    float _atmosphereInnerRadius;
    float _atmosphereOuterRadius;
    
    float _rayleighScattering;
    float _mieScattering;
    
    glm::vec3 _scatteringWavelengths;
    
    glm::vec3 _sunLocation;
    float _sunBrightness;
};

#endif /* defined(__interface__EnvironmentData__) */
