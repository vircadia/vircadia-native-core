#pragma once
//
//  BRDF.h
//
//  Created by Olivier Prat on 04/04/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef SHARED_BRDF_H
#define SHARED_BRDF_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// GGX micro-facet model
namespace ggx {
    float evaluate(float NdotH, float roughness);
    glm::vec3 sample(const glm::vec2& Xi, const float roughness);
}

// Smith visibility function
namespace smith {
    float evaluateFastWithoutNdotV(float alphaSquared, float NdotV, float NdotL);

    inline float evaluateFast(float alphaSquared, float NdotV, float NdotL) {
        return evaluateFastWithoutNdotV(alphaSquared, NdotV, NdotL) * NdotV;
    }

    inline float evaluate(float roughness, float NdotV, float NdotL) {
        return evaluateFast(roughness*roughness*roughness*roughness, NdotV, NdotL);
    }
}

#endif // SHARED_BRDF_H