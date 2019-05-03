//
//  RandomAndNoise.h
//
//  Created by Olivier Prat on 05/16/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef RANDOM_AND_NOISE_H
#define RANDOM_AND_NOISE_H

#include <glm/vec2.hpp>

namespace halton {
    // Low discrepancy Halton sequence generator
    template <int B>
    float evaluate(int index) {
        float f = 1.0f;
        float r = 0.0f;
        float invB = 1.0f / (float)B;
        index++; // Indices start at 1, not 0

        while (index > 0) {
            f = f * invB;
            r = r + f * (float)(index % B);
            index = index / B;

        }

        return r;
    }
}

inline float getRadicalInverseVdC(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f; // / 0x100000000\n"
}

namespace hammersley {
    // Low discrepancy Hammersley 2D sequence generator
    inline glm::vec2 evaluate(int k, const int sequenceLength) {
        return glm::vec2(float(k) / float(sequenceLength), getRadicalInverseVdC(k));
    }
}


#endif