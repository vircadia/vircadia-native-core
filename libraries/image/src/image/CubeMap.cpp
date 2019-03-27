//
//  CubeMap.h
//  image/src/image
//
//  Created by Olivier Prat on 03/27/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "CubeMap.h"

#include <cmath>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>

#include "RandomAndNoise.h"

#ifndef M_PI
#define M_PI    3.14159265359
#endif

using namespace image;

CubeMap::CubeMap(int width, int height, int mipCount) :
    _width(width), _height(height) {
    assert(mipCount >0 && _width > 0 && _height > 0);
    _mips.resize(mipCount);
    for (auto mipLevel = 0; mipLevel < mipCount; mipLevel++) {
        auto mipWidth = std::max(1, width >> mipLevel);
        auto mipHeight = std::max(1, height >> mipLevel);
        auto mipPixelCount = mipWidth * mipHeight;

        for (auto& face : _mips[mipLevel]) {
            face.resize(mipPixelCount);
        }
    }
}

glm::vec4 CubeMap::fetchLod(const glm::vec3& dir, float lod) const {
    // TODO
    return glm::vec4(0.0f);
}

static glm::vec3 sampleGGX(const glm::vec2& Xi, const float roughness) {
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

static float evaluateGGX(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;
    float denom = (float)(NdotH * NdotH * (alphaSquared - 1.0) + 1.0);
    return alphaSquared / (denom * denom);
}

struct CubeMap::GGXSamples {
    float invTotalWeight;
    std::vector<glm::vec4> points;
};

void CubeMap::generateGGXSamples(GGXSamples& data, float roughness, const int resolution) {
    glm::vec2 xi;
    glm::vec3 L;
    glm::vec3 H;
    const float saTexel = (float)(4.0 * M_PI / (6.0 * resolution * resolution));
    const float mipBias = 3.0f;
    const auto sampleCount = data.points.size();
    const auto hammersleySequenceLength = data.points.size();
    int sampleIndex = 0;
    int hammersleySampleIndex = 0;
    float NdotL;

    data.invTotalWeight = 0.0f;

    // Do some computation in tangent space
    while (sampleIndex < sampleCount) {
        if (hammersleySampleIndex < hammersleySequenceLength) {
            xi = evaluateHammersley((int)hammersleySampleIndex, (int)hammersleySequenceLength);
            H = sampleGGX(xi, roughness);
            L = H * (2.0f * H.z) - glm::vec3(0.0f, 0.0f, 1.0f);
            NdotL = L.z;
            hammersleySampleIndex++;
        } else {
            NdotL = -1.0f;
        }

        while (NdotL <= 0.0f) {
            // Create a purely random sample
            xi.x = rand() / float(RAND_MAX);
            xi.y = rand() / float(RAND_MAX);
            H = sampleGGX(xi, roughness);
            L = H * (2.0f * H.z) - glm::vec3(0.0f, 0.0f, 1.0f);
            NdotL = L.z;
        }

        float NdotH = std::max(0.0f, H.z);
        float HdotV = NdotH;
        float D = evaluateGGX(NdotH, roughness);
        float pdf = (D * NdotH / (4.0f * HdotV)) + 0.0001f;
        float saSample = 1.0f / (float(sampleCount) * pdf + 0.0001f);
        float mipLevel = std::max(0.5f * log2(saSample / saTexel) + mipBias, 0.0f);

        auto& sample = data.points[sampleIndex];
        sample.x = L.x;
        sample.y = L.y;
        sample.z = L.z;
        sample.w = mipLevel;

        data.invTotalWeight += NdotL;

        sampleIndex++;
    }
    data.invTotalWeight = 1.0f / data.invTotalWeight;
}

void CubeMap::convolveForGGX(CubeMap& output, const std::atomic<bool>& abortProcessing) const {
    // This should match fragment.glsl values, too
    static const float ROUGHNESS_1_MIP_RESOLUTION = 1.5f;
    static const gpu::uint16 MAX_SAMPLE_COUNT = 4000;

    const auto mipCount = getMipCount();
    GGXSamples params;

    params.points.reserve(MAX_SAMPLE_COUNT);

    for (gpu::uint16 mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        // This is the inverse code found in fragment.glsl in evaluateAmbientLighting
        float levelAlpha = float(mipLevel) / (mipCount - ROUGHNESS_1_MIP_RESOLUTION);
        float mipRoughness = levelAlpha * (1.0f + 2.0f * levelAlpha) / 3.0f;
        mipRoughness = std::max(1e-3f, mipRoughness);
        mipRoughness = std::min(1.0f, mipRoughness);

        params.points.resize(std::min<size_t>(MAX_SAMPLE_COUNT, 1U + size_t(4000 * mipRoughness * mipRoughness)));
        generateGGXSamples(params, mipRoughness, _width);

        for (int face = 0; face < 6; face++) {
            convolveMipFaceForGGX(params, output, mipLevel, face, abortProcessing);
            if (abortProcessing.load()) {
                return;
            }
        }
    }
}

void CubeMap::convolveMipFaceForGGX(const GGXSamples& samples, CubeMap& output, gpu::uint16 mipLevel, int face, const std::atomic<bool>& abortProcessing) const {
    static const glm::vec3 NORMALS[24] = {
        // POSITIVE X
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f, 1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),
        // NEGATIVE X
        glm::vec3(-1.0f, 1.0f, -1.0f),
        glm::vec3(-1.0f, 1.0f, 1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f, 1.0f),
        // POSITIVE Y
        glm::vec3(-1.0f, 1.0f, -1.0f),
        glm::vec3(1.0f, 1.0f, -1.0f),
        glm::vec3(-1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        // NEGATIVE Y
        glm::vec3(-1.0f, -1.0f, 1.0f),
        glm::vec3(1.0f, -1.0f, 1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),
        // POSITIVE Z
        glm::vec3(-1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(-1.0f, -1.0f, 1.0f),
        glm::vec3(1.0f, -1.0f, 1.0f),
        // NEGATIVE Z
        glm::vec3(1.0f, 1.0f, -1.0f),
        glm::vec3(-1.0f, 1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f)
    };

    const glm::vec3* faceNormals = NORMALS + face * 4;
    const glm::vec3 deltaXNormalLo = faceNormals[1] - faceNormals[0];
    const glm::vec3 deltaXNormalHi = faceNormals[3] - faceNormals[2];
    auto& outputFace = output._mips[mipLevel][face];

    tbb::parallel_for(tbb::blocked_range2d<int, int>(0, _width, 16, 0, _height, 16), [&](const tbb::blocked_range2d<int, int>& range) {
        auto rowRange = range.rows();
        auto colRange = range.cols();

        for (auto x = rowRange.begin(); x < rowRange.end(); x++) {
            const float xAlpha = (x + 0.5f) / _width;
            const glm::vec3 normalYLo = faceNormals[0] + deltaXNormalLo * xAlpha;
            const glm::vec3 normalYHi = faceNormals[2] + deltaXNormalHi * xAlpha;
            const glm::vec3 deltaYNormal = normalYHi - normalYLo;

            for (auto y = colRange.begin(); y < colRange.end(); y++) {
                const float yAlpha = (y + 0.5f) / _width;
                // Interpolate normal for this pixel
                const glm::vec3 normal = glm::normalize(normalYLo + deltaYNormal * yAlpha);

                outputFace[x + y * _width] = computeConvolution(normal, samples);
            }

            if (abortProcessing.load()) {
                break;
            }
        }
    });
}

glm::vec4 CubeMap::computeConvolution(const glm::vec3& N, const GGXSamples& samples) const {
    // from tangent-space vector to world-space
    glm::vec3 bitangent = abs(N.z) < 0.999 ? glm::vec3(0.0, 0.0, 1.0) : glm::vec3(1.0, 0.0, 0.0);
    glm::vec3 tangent = glm::normalize(glm::cross(bitangent, N));
    bitangent = glm::cross(N, tangent);

    const size_t sampleCount = samples.points.size();
    glm::vec4 prefilteredColor = glm::vec4(0.0f);

    for (int i = 0; i < sampleCount; ++i) {
        const auto& sample = samples.points[i];
        glm::vec3 L(sample.x, sample.y, sample.z);
        float NdotL = L.z;
        float mipLevel = sample.w;
        // Now back to world space
        L = tangent * L.x + bitangent * L.y + N * L.z;
        prefilteredColor += fetchLod(L, mipLevel) * NdotL;
    }
    prefilteredColor = prefilteredColor * samples.invTotalWeight;
    prefilteredColor.a = 1.0f;
    return prefilteredColor;
}