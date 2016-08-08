//
//  ColorUtils.h
//  libraries/shared/src
//
//  Created by Sam Gateau on 10/24/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ColorUtils_h
#define hifi_ColorUtils_h

#include <glm/glm.hpp>

#include "SharedUtil.h"

#include "DependencyManager.h"

extern const float srgbToLinearLookupTable[256];

class ColorUtils {
public:
    inline static glm::vec3 toVec3(const xColor& color);

    // Convert to gamma 2.2 space from linear
    inline static glm::vec3 toGamma22Vec3(const glm::vec3& linear);
    
    // Convert from sRGB gamma space to linear.
    // This is pretty different from converting from 2.2.
    inline static glm::vec3 sRGBToLinearVec3(const glm::vec3& srgb);
    inline static glm::vec3 tosRGBVec3(const glm::vec3& srgb);
    
    inline static glm::vec4 sRGBToLinearVec4(const glm::vec4& srgb);
    inline static glm::vec4 tosRGBVec4(const glm::vec4& srgb);
    
    inline static float sRGBToLinearFloat(const float& srgb);
    inline static float sRGB8ToLinearFloat(const uint8_t srgb);
    inline static float tosRGBFloat(const float& linear);
};

inline glm::vec3 ColorUtils::toVec3(const xColor& color) {
    const float ONE_OVER_255 = 1.0f / 255.0f;
    return glm::vec3(color.red * ONE_OVER_255, color.green * ONE_OVER_255, color.blue * ONE_OVER_255);
}

inline glm::vec3 ColorUtils::toGamma22Vec3(const glm::vec3& linear) {
    const float INV_GAMMA_22 = 1.0f / 2.2f;
    // Couldn't find glm::pow(vec3, vec3) ? so did it myself...
    return glm::vec3(glm::pow(linear.x, INV_GAMMA_22), glm::pow(linear.y, INV_GAMMA_22), glm::pow(linear.z, INV_GAMMA_22));
}

// Convert from sRGB color space to linear color space.
inline glm::vec3 ColorUtils::sRGBToLinearVec3(const glm::vec3& srgb) {
    return glm::vec3(sRGBToLinearFloat(srgb.x), sRGBToLinearFloat(srgb.y), sRGBToLinearFloat(srgb.z));
}

// Convert from linear color space to sRGB color space.
inline glm::vec3 ColorUtils::tosRGBVec3(const glm::vec3& linear) {
    return glm::vec3(tosRGBFloat(linear.x), tosRGBFloat(linear.y), tosRGBFloat(linear.z));
}

// Convert from sRGB color space with alpha to linear color space with alpha.
inline glm::vec4 ColorUtils::sRGBToLinearVec4(const glm::vec4& srgb) {
    return glm::vec4(sRGBToLinearFloat(srgb.x), sRGBToLinearFloat(srgb.y), sRGBToLinearFloat(srgb.z), srgb.w);
}

// Convert from linear color space with alpha to sRGB color space with alpha.
inline glm::vec4 ColorUtils::tosRGBVec4(const glm::vec4& linear) {
    return glm::vec4(tosRGBFloat(linear.x), tosRGBFloat(linear.y), tosRGBFloat(linear.z), linear.w);
}

// This is based upon the conversions found in section 8.24 of the OpenGL 4.4 4.4 specification.
// glm::pow(color, 2.2f) is approximate, and will cause subtle differences when used with sRGB framebuffers.
inline float ColorUtils::sRGBToLinearFloat(const float &srgb) {
    const float SRGB_ELBOW = 0.04045f;
    float linearValue = 0.0f;
    
    // This should mirror the conversion table found in section 8.24: sRGB Texture Color Conversion
    if (srgb <= SRGB_ELBOW) {
        linearValue = srgb / 12.92f;
    } else {
        linearValue = powf(((srgb + 0.055f) / 1.055f), 2.4f);
    }
    
    return linearValue;
}
inline float ColorUtils::sRGB8ToLinearFloat(const uint8_t srgb) {
    return srgbToLinearLookupTable[srgb];
}

// This is based upon the conversions found in section 17.3.9 of the OpenGL 4.4 specification.
// glm::pow(color, 1.0f/2.2f) is approximate, and will cause subtle differences when used with sRGB framebuffers.
inline float ColorUtils::tosRGBFloat(const float &linear) {
    const float SRGB_ELBOW_INV = 0.0031308f;
    float sRGBValue = 0.0f;
    
    // This should mirror the conversion table found in section 17.3.9: sRGB Conversion
    if (linear <= 0.0f) {
        sRGBValue = 0.0f;
    } else if (0 < linear && linear < SRGB_ELBOW_INV) {
        sRGBValue = 12.92f * linear;
    } else if (SRGB_ELBOW_INV <= linear && linear < 1) {
        sRGBValue = 1.055f * powf(linear, 0.41666f - 0.055f);
    } else {
        sRGBValue = 1.0f;
    }
    
    return sRGBValue;
}

#endif // hifi_ColorUtils_h