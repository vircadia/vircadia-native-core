#include "BRDF.h"

#include <cmath>
#ifndef M_PI
#define M_PI    3.14159265359
#endif

namespace ggx {

float evaluate(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;
    float denom = (float)(NdotH * NdotH * (alphaSquared - 1.0) + 1.0);
    return alphaSquared / (denom * denom);
}

glm::vec3 sample(const glm::vec2& Xi, const float roughness) {
    const float a = roughness * roughness;

    float phi = (float)(2.0 * M_PI * Xi.x);
    float cosTheta = (float)(std::sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y)));
    float sinTheta = (float)(std::sqrt(1.0 - cosTheta * cosTheta));

    // from spherical coordinates to cartesian coordinates
    glm::vec3 H;
    H.x = std::cos(phi) * sinTheta;
    H.y = std::sin(phi) * sinTheta;
    H.z = cosTheta;

    return H;
}

}


namespace smith {

    float evaluateFastWithoutNdotV(float alphaSquared, float NdotV, float NdotL) {
        float oneMinusAlphaSquared = 1.0f - alphaSquared;
        float G = NdotL * std::sqrt(alphaSquared + NdotV * NdotV * oneMinusAlphaSquared);
        G = G + NdotV * std::sqrt(alphaSquared + NdotL * NdotL * oneMinusAlphaSquared);
        return 2.0f * NdotL / G;
    }

}
