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
#include <TBBHelpers.h>

#include "RandomAndNoise.h"
#include "BRDF.h"
#include "ImageLogging.h"

#ifndef M_PI
#define M_PI    3.14159265359
#endif

#include <nvtt/nvtt.h>

using namespace image;

static const glm::vec3 FACE_NORMALS[24] = {
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

struct CubeFaceMip {

    CubeFaceMip(gpu::uint16 level, const CubeMap* cubemap) {
        _dims = cubemap->getMipDimensions(level);
        _lineStride = cubemap->getMipLineStride(level);
    }

    CubeFaceMip(const CubeFaceMip& other) : _dims(other._dims), _lineStride(other._lineStride) {

    }

    gpu::Vec2i _dims;
    size_t _lineStride;
};

class CubeMap::ConstMip : public CubeFaceMip {
public:

    ConstMip(gpu::uint16 level, const CubeMap* cubemap) : 
        CubeFaceMip(level, cubemap), _faces(cubemap->_mips[level]) {
    }

    glm::vec4 fetch(int face, glm::vec2 uv) const {
        glm::vec2 coordFrac = uv * glm::vec2(_dims) - 0.5f;
        glm::vec2 coords = glm::floor(coordFrac);

        coordFrac -= coords;

        coords += (float)EDGE_WIDTH;

        const auto& pixels = _faces[face];
        gpu::Vec2i loCoords(coords);
        gpu::Vec2i hiCoords;

        hiCoords = glm::clamp(loCoords + 1, gpu::Vec2i(0, 0), _dims - 1 + (int)EDGE_WIDTH);
        loCoords = glm::clamp(loCoords, gpu::Vec2i(0, 0), _dims - 1 + (int)EDGE_WIDTH);

        const size_t offsetLL = loCoords.x + loCoords.y * _lineStride;
        const size_t offsetHL = hiCoords.x + loCoords.y * _lineStride;
        const size_t offsetLH = loCoords.x + hiCoords.y * _lineStride;
        const size_t offsetHH = hiCoords.x + hiCoords.y * _lineStride;
        assert(offsetLL < _lineStride * (_dims.y + 2 * EDGE_WIDTH));
        assert(offsetHL < _lineStride * (_dims.y + 2 * EDGE_WIDTH));
        assert(offsetLH < _lineStride * (_dims.y + 2 * EDGE_WIDTH));
        assert(offsetHH < _lineStride * (_dims.y + 2 * EDGE_WIDTH));
        glm::vec4 colorLL = pixels[offsetLL];
        glm::vec4 colorHL = pixels[offsetHL];
        glm::vec4 colorLH = pixels[offsetLH];
        glm::vec4 colorHH = pixels[offsetHH];

        colorLL += (colorHL - colorLL) * coordFrac.x;
        colorLH += (colorHH - colorLH) * coordFrac.x;
        return colorLL + (colorLH - colorLL) * coordFrac.y;
    }

private:

    const Faces& _faces;

};

class CubeMap::Mip : public CubeFaceMip {
public:

    explicit Mip(gpu::uint16 level, CubeMap* cubemap) :
        CubeFaceMip(level, cubemap), _faces(cubemap->_mips[level]) {
    }

    Mip(const Mip& other) : CubeFaceMip(other), _faces(other._faces) {
    }

    void applySeams() {
        if (EDGE_WIDTH == 0) {
            return;
        }

        // Copy edge rows and columns from neighbouring faces to fix seam filtering issues
        seamColumnAndRow(gpu::Texture::CUBE_FACE_TOP_POS_Y, _dims.x, gpu::Texture::CUBE_FACE_RIGHT_POS_X, -1, -1);
        seamColumnAndRow(gpu::Texture::CUBE_FACE_BOTTOM_NEG_Y, _dims.x, gpu::Texture::CUBE_FACE_RIGHT_POS_X, _dims.y, 1);
        seamColumnAndColumn(gpu::Texture::CUBE_FACE_FRONT_NEG_Z, -1, gpu::Texture::CUBE_FACE_RIGHT_POS_X, _dims.x, 1);
        seamColumnAndColumn(gpu::Texture::CUBE_FACE_BACK_POS_Z, _dims.x, gpu::Texture::CUBE_FACE_RIGHT_POS_X, -1, 1);

        seamRowAndRow(gpu::Texture::CUBE_FACE_BACK_POS_Z, -1, gpu::Texture::CUBE_FACE_TOP_POS_Y, _dims.y, 1);
        seamRowAndRow(gpu::Texture::CUBE_FACE_BACK_POS_Z, _dims.y, gpu::Texture::CUBE_FACE_BOTTOM_NEG_Y, -1, 1);
        seamColumnAndColumn(gpu::Texture::CUBE_FACE_BACK_POS_Z, -1, gpu::Texture::CUBE_FACE_LEFT_NEG_X, _dims.x, 1);

        seamRowAndRow(gpu::Texture::CUBE_FACE_TOP_POS_Y, -1, gpu::Texture::CUBE_FACE_FRONT_NEG_Z, -1, -1);
        seamColumnAndRow(gpu::Texture::CUBE_FACE_TOP_POS_Y, -1, gpu::Texture::CUBE_FACE_LEFT_NEG_X, -1, 1);

        seamColumnAndColumn(gpu::Texture::CUBE_FACE_LEFT_NEG_X, -1, gpu::Texture::CUBE_FACE_FRONT_NEG_Z, _dims.x, 1);
        seamColumnAndRow(gpu::Texture::CUBE_FACE_BOTTOM_NEG_Y, -1, gpu::Texture::CUBE_FACE_LEFT_NEG_X, _dims.y, -1);

        seamRowAndRow(gpu::Texture::CUBE_FACE_FRONT_NEG_Z, _dims.y, gpu::Texture::CUBE_FACE_BOTTOM_NEG_Y, _dims.y, -1);

        // Duplicate corner pixels
        for (int face = 0; face < 6; face++) {
            auto& pixels = _faces[face];

            pixels[0] = pixels[1];
            pixels[_dims.x + 1] = pixels[_dims.x];
            pixels[(_dims.y + 1)*(_dims.x + 2)] = pixels[(_dims.y + 1)*(_dims.x + 2) + 1];
            pixels[(_dims.y + 2)*(_dims.x + 2) - 1] = pixels[(_dims.y + 2)*(_dims.x + 2) - 2];
        }
    }

private:

    Faces& _faces;

    inline static void copy(CubeMap::Face::const_iterator srcFirst, CubeMap::Face::const_iterator srcLast, size_t srcStride, CubeMap::Face::iterator dstBegin, size_t dstStride) {
        while (srcFirst <= srcLast) {
            *dstBegin = *srcFirst;
            srcFirst += srcStride;
            dstBegin += dstStride;
        }
    }

    static std::pair<int, int> getSrcAndDst(int dim, int value) {
        int src;
        int dst;

        if (value < 0) {
            src = 1;
            dst = 0;
        } else if (value >= dim) {
            src = dim;
            dst = dim + 1;
        }
        return std::make_pair(src, dst);
    }

    void seamColumnAndColumn(int face0, int col0, int face1, int col1, int inc) {
        auto coords0 = getSrcAndDst(_dims.x, col0);
        auto coords1 = getSrcAndDst(_dims.x, col1);

        copyColumnToColumn(face0, coords0.first, face1, coords1.second, inc);
        copyColumnToColumn(face1, coords1.first, face0, coords0.second, inc);
    }

    void seamColumnAndRow(int face0, int col0, int face1, int row1, int inc) {
        auto coords0 = getSrcAndDst(_dims.x, col0);
        auto coords1 = getSrcAndDst(_dims.y, row1);

        copyColumnToRow(face0, coords0.first, face1, coords1.second, inc);
        copyRowToColumn(face1, coords1.first, face0, coords0.second, inc);
    }

    void seamRowAndRow(int face0, int row0, int face1, int row1, int inc) {
        auto coords0 = getSrcAndDst(_dims.y, row0);
        auto coords1 = getSrcAndDst(_dims.y, row1);

        copyRowToRow(face0, coords0.first, face1, coords1.second, inc);
        copyRowToRow(face1, coords1.first, face0, coords0.second, inc);
    }

    void copyColumnToColumn(int srcFace, int srcCol, int dstFace, int dstCol, const int dstInc) {
        const auto lastOffset = _lineStride * (_dims.y - 1);
        auto srcFirst = _faces[srcFace].begin() + srcCol + _lineStride;
        auto srcLast = srcFirst + lastOffset;

        auto dstFirst = _faces[dstFace].begin() + dstCol + _lineStride;
        auto dstLast = dstFirst + lastOffset;
        const auto dstStride = _lineStride * dstInc;

        assert(srcFirst < _faces[srcFace].end());
        assert(srcLast < _faces[srcFace].end());
        assert(dstFirst < _faces[dstFace].end());
        assert(dstLast < _faces[dstFace].end());

        if (dstInc < 0) {
            std::swap(dstFirst, dstLast);
        }

        copy(srcFirst, srcLast, _lineStride, dstFirst, dstStride);
    }

    void copyRowToRow(int srcFace, int srcRow, int dstFace, int dstRow, const int dstInc) {
        const auto lastOffset =(_dims.x - 1);
        auto srcFirst = _faces[srcFace].begin() + srcRow * _lineStride + 1;
        auto srcLast = srcFirst + lastOffset;

        auto dstFirst = _faces[dstFace].begin() + dstRow * _lineStride + 1;
        auto dstLast = dstFirst + lastOffset;

        assert(srcFirst < _faces[srcFace].end());
        assert(srcLast < _faces[srcFace].end());
        assert(dstFirst < _faces[dstFace].end());
        assert(dstLast < _faces[dstFace].end());

        if (dstInc < 0) {
            std::swap(dstFirst, dstLast);
        }

        copy(srcFirst, srcLast, 1, dstFirst, dstInc);
    }

    void copyColumnToRow(int srcFace, int srcCol, int dstFace, int dstRow, int dstInc) {
        const auto srcLastOffset = _lineStride * (_dims.y - 1);
        auto srcFirst = _faces[srcFace].begin() + srcCol + _lineStride;
        auto srcLast = srcFirst + srcLastOffset;

        const auto dstLastOffset = (_dims.x - 1);
        auto dstFirst = _faces[dstFace].begin() + dstRow * _lineStride + 1;
        auto dstLast = dstFirst + dstLastOffset;

        assert(srcFirst < _faces[srcFace].end());
        assert(srcLast < _faces[srcFace].end());
        assert(dstFirst < _faces[dstFace].end());
        assert(dstLast < _faces[dstFace].end());

        if (dstInc < 0) {
            std::swap(dstFirst, dstLast);
        }

        copy(srcFirst, srcLast, _lineStride, dstFirst, dstInc);
    }

    void copyRowToColumn(int srcFace, int srcRow, int dstFace, int dstCol, int dstInc) {
        const auto srcLastOffset = (_dims.x - 1);
        auto srcFirst = _faces[srcFace].begin() + srcRow * _lineStride + 1;
        auto srcLast = srcFirst + srcLastOffset;

        const auto dstLastOffset = _lineStride * (_dims.y - 1);
        auto dstFirst = _faces[dstFace].begin() + dstCol + _lineStride;
        auto dstLast = dstFirst + dstLastOffset;
        const auto dstStride = _lineStride * dstInc;

        assert(srcFirst < _faces[srcFace].end());
        assert(srcLast < _faces[srcFace].end());
        assert(dstFirst < _faces[dstFace].end());
        assert(dstLast < _faces[dstFace].end());

        if (dstInc < 0) {
            std::swap(dstFirst, dstLast);
        }

        copy(srcFirst, srcLast, 1, dstFirst, dstStride);
    }
};

static void copySurface(const nvtt::Surface& source, glm::vec4* dest, size_t dstLineStride) {
    const float* srcRedIt = source.channel(0);
    const float* srcGreenIt = source.channel(1);
    const float* srcBlueIt = source.channel(2);
    const float* srcAlphaIt = source.channel(3);

    for (int y = 0; y < source.height(); y++) {
        glm::vec4* dstColIt = dest;
        for (int x = 0; x < source.width(); x++) {
            *dstColIt = glm::vec4(*srcRedIt, *srcGreenIt, *srcBlueIt, *srcAlphaIt);
            dstColIt++;
            srcRedIt++;
            srcGreenIt++;
            srcBlueIt++;
            srcAlphaIt++;
        }
        dest += dstLineStride;
    }
}

CubeMap::CubeMap(int width, int height, int mipCount) {
    reset(width, height, mipCount);
}

CubeMap::CubeMap(const std::vector<Image>& faces, int mipCount, const std::atomic<bool>& abortProcessing) {
    reset(faces.front().getWidth(), faces.front().getHeight(), mipCount);

    int face;

    nvtt::Surface surface;
    surface.setAlphaMode(nvtt::AlphaMode_None);
    surface.setWrapMode(nvtt::WrapMode_Mirror);

    // Compute mips
    for (face = 0; face < 6; face++) {
        Image faceImage = faces[face].getConvertedToFormat(Image::Format_RGBAF);

        surface.setImage(nvtt::InputFormat_RGBA_32F, _width, _height, 1, faceImage.editBits());

        auto mipLevel = 0;
        copySurface(surface, editFace(0, face), getMipLineStride(0));

        while (surface.canMakeNextMipmap() && !abortProcessing.load()) {
            surface.buildNextMipmap(nvtt::MipmapFilter_Box);
            mipLevel++;

            copySurface(surface, editFace(mipLevel, face), getMipLineStride(mipLevel));
        }
    }

    if (abortProcessing.load()) {
        return;
    }

    for (gpu::uint16 mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        Mip mip(mipLevel, this);
        mip.applySeams();
    }
}

void CubeMap::applyGamma(float value) {
    for (auto& mip : _mips) {
        for (auto& face : mip) {
            for (auto& pixel : face) {
                pixel.r = std::pow(pixel.r, value);
                pixel.g = std::pow(pixel.g, value);
                pixel.b = std::pow(pixel.b, value);
            }
        }
    }
}

void CubeMap::copyFace(int width, int height, const glm::vec4* source, size_t srcLineStride, glm::vec4* dest, size_t dstLineStride) {
    for (int y = 0; y < height; y++) {
        std::copy(source, source + width, dest);
        source += srcLineStride;
        dest += dstLineStride;
    }
}

Image CubeMap::getFaceImage(gpu::uint16 mipLevel, int face) const {
    auto mipDims = getMipDimensions(mipLevel);
    Image faceImage(mipDims.x, mipDims.y, Image::Format_RGBAF);
    copyFace(mipDims.x, mipDims.y, getFace(mipLevel, face), getMipLineStride(mipLevel), (glm::vec4*)faceImage.editBits(), faceImage.getBytesPerLineCount() / sizeof(glm::vec4));
    return faceImage;
}

void CubeMap::reset(int width, int height, int mipCount) {
    assert(mipCount >0 && width > 0 && height > 0);
    _width = width;
    _height = height;
    _mips.resize(mipCount);
    for (auto mipLevel = 0; mipLevel < mipCount; mipLevel++) {
        auto mipDimensions = getMipDimensions(mipLevel);
        // Add extra pixels on edges to perform edge seam fixup (we will duplicate pixels from
        // neighbouring faces)
        auto mipPixelCount = (mipDimensions.x + 2 * EDGE_WIDTH) * (mipDimensions.y + 2 * EDGE_WIDTH);

        for (auto& face : _mips[mipLevel]) {
            face.resize(mipPixelCount);
        }
    }
}

void CubeMap::copyTo(CubeMap& other) const {
    other._width = _width;
    other._height = _height;
    other._mips = _mips;
}

void CubeMap::getFaceUV(const glm::vec3& dir, int* index, glm::vec2* uv) {
    // Taken from https://en.wikipedia.org/wiki/Cube_mapping
    float absX = std::abs(dir.x);
    float absY = std::abs(dir.y);
    float absZ = std::abs(dir.z);

    auto isXPositive = dir.x > 0;
    auto isYPositive = dir.y > 0;
    auto isZPositive = dir.z > 0;

    float maxAxis = 1.0f;
    float uc = 0.0f;
    float vc = 0.0f;

    // POSITIVE X
    if (isXPositive && absX >= absY && absX >= absZ) {
        // u (0 to 1) goes from +z to -z
        // v (0 to 1) goes from -y to +y
        maxAxis = absX;
        uc = -dir.z;
        vc = -dir.y;
        *index = 0;
    }
    // NEGATIVE X
    else if (!isXPositive && absX >= absY && absX >= absZ) {
        // u (0 to 1) goes from -z to +z
        // v (0 to 1) goes from -y to +y
        maxAxis = absX;
        uc = dir.z;
        vc = -dir.y;
        *index = 1;
    }
    // POSITIVE Y
    else if (isYPositive && absY >= absX && absY >= absZ) {
        // u (0 to 1) goes from -x to +x
        // v (0 to 1) goes from +z to -z
        maxAxis = absY;
        uc = dir.x;
        vc = dir.z;
        *index = 2;
    }
    // NEGATIVE Y
    else if (!isYPositive && absY >= absX && absY >= absZ) {
        // u (0 to 1) goes from -x to +x
        // v (0 to 1) goes from -z to +z
        maxAxis = absY;
        uc = dir.x;
        vc = -dir.z;
        *index = 3;
    }
    // POSITIVE Z
    else if (isZPositive && absZ >= absX && absZ >= absY) {
        // u (0 to 1) goes from -x to +x
        // v (0 to 1) goes from -y to +y
        maxAxis = absZ;
        uc = dir.x;
        vc = -dir.y;
        *index = 4;
    }
    // NEGATIVE Z
    else if (!isZPositive && absZ >= absX && absZ >= absY) {
        // u (0 to 1) goes from +x to -x
        // v (0 to 1) goes from -y to +y
        maxAxis = absZ;
        uc = -dir.x;
        vc = -dir.y;
        *index = 5;
    }

    // Convert range from -1 to 1 to 0 to 1
    uv->x = 0.5f * (uc / maxAxis + 1.0f);
    uv->y = 0.5f * (vc / maxAxis + 1.0f);
}

glm::vec4 CubeMap::fetchLod(const glm::vec3& dir, float lod) const {
    lod = glm::clamp<float>(lod, 0.0f, _mips.size() - 1);

    gpu::uint16 loLevel = (gpu::uint16)std::floor(lod);
    gpu::uint16 hiLevel = (gpu::uint16)std::ceil(lod);
    float lodFrac = lod - (float)loLevel;
    ConstMip loMip(loLevel, this);
    ConstMip hiMip(hiLevel, this);
    int face;
    glm::vec2 uv;
    glm::vec4 loColor;
    glm::vec4 hiColor;

    getFaceUV(dir, &face, &uv);

    loColor = loMip.fetch(face, uv);
    hiColor = hiMip.fetch(face, uv);

    return loColor + (hiColor - loColor) * lodFrac;
}

struct CubeMap::GGXSamples {
    float invTotalWeight;
    std::vector<glm::vec4> points;
};

// All the GGX convolution code is inspired from:
// https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
// Computation is done in tangent space so normal is always (0,0,1) which simplifies a lot of things

void CubeMap::generateGGXSamples(GGXSamples& data, float roughness, const int resolution) {
    glm::vec2 xi;
    glm::vec3 L;
    glm::vec3 H;
    const float saTexel = (float)(4.0 * M_PI / (6.0 * resolution * resolution));
    const float mipBias = 3.0f;
    const auto sampleCount = data.points.size();
    const auto hammersleySequenceLength = data.points.size();
    size_t sampleIndex = 0;
    size_t hammersleySampleIndex = 0;
    float NdotL;

    data.invTotalWeight = 0.0f;

    // Do some computation in tangent space
    while (sampleIndex < sampleCount) {
        if (hammersleySampleIndex < hammersleySequenceLength) {
            xi = hammersley::evaluate((int)hammersleySampleIndex, (int)hammersleySequenceLength);
            H = ggx::sample(xi, roughness);
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
            H = ggx::sample(xi, roughness);
            L = H * (2.0f * H.z) - glm::vec3(0.0f, 0.0f, 1.0f);
            NdotL = L.z;
        }

        float NdotH = std::max(0.0f, H.z);
        float HdotV = NdotH;
        float D = ggx::evaluate(NdotH, roughness);
        float pdf = (D * NdotH / (4.0f * HdotV)) + 0.0001f;
        float saSample = 1.0f / (float(sampleCount) * pdf + 0.0001f);
        float mipLevel = std::max(0.5f * std::log2(saSample / saTexel) + mipBias, 0.0f);

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
    // This should match the value in the getMipLevelFromRoughness function (LightAmbient.slh)
    static const float ROUGHNESS_1_MIP_RESOLUTION = 1.5f;
    static const size_t MAX_SAMPLE_COUNT = 4000;

    const auto mipCount = getMipCount();
    GGXSamples params;

    params.points.reserve(MAX_SAMPLE_COUNT);

    for (gpu::uint16 mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        // This is the inverse code found in LightAmbient.slh in getMipLevelFromRoughness
        float levelAlpha = float(mipLevel) / (mipCount - ROUGHNESS_1_MIP_RESOLUTION);
        float mipRoughness = levelAlpha * (1.0f + 2.0f * levelAlpha) / 3.0f;

        mipRoughness = std::max(1e-3f, mipRoughness);
        mipRoughness = std::min(1.0f, mipRoughness);

        size_t mipTotalPixelCount = getMipWidth(mipLevel) * getMipHeight(mipLevel) * 6;
        size_t sampleCount = 1U + size_t(4000 * mipRoughness * mipRoughness);

        sampleCount = std::min(sampleCount, 2 * mipTotalPixelCount);
        sampleCount = std::min(MAX_SAMPLE_COUNT, sampleCount);

        params.points.resize(sampleCount);
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
    const glm::vec3* faceNormals = FACE_NORMALS + face * 4;
    const glm::vec3 deltaYNormalLo = faceNormals[2] - faceNormals[0];
    const glm::vec3 deltaYNormalHi = faceNormals[3] - faceNormals[1];
    const auto mipDimensions = output.getMipDimensions(mipLevel);
    const auto outputLineStride = output.getMipLineStride(mipLevel);
    auto outputFacePixels = output.editFace(mipLevel, face);

    tbb::parallel_for(tbb::blocked_range2d<int, int>(0, mipDimensions.y, 32, 0, mipDimensions.x, 32), [&](const tbb::blocked_range2d<int, int>& range) {
        auto rowRange = range.rows();
        auto colRange = range.cols();

        for (auto y = rowRange.begin(); y < rowRange.end(); y++) {
            if (abortProcessing.load()) {
                break;
            }

            const float yAlpha = (y + 0.5f) / mipDimensions.y;
            const glm::vec3 normalXLo = faceNormals[0] + deltaYNormalLo * yAlpha;
            const glm::vec3 normalXHi = faceNormals[1] + deltaYNormalHi * yAlpha;
            const glm::vec3 deltaXNormal = normalXHi - normalXLo;

            for (auto x = colRange.begin(); x < colRange.end(); x++) {
                const float xAlpha = (x + 0.5f) / mipDimensions.x;
                // Interpolate normal for this pixel
                const glm::vec3 normal = glm::normalize(normalXLo + deltaXNormal * xAlpha);

                outputFacePixels[x + y * outputLineStride] = computeConvolution(normal, samples);
            }
        }
    });
}

glm::vec4 CubeMap::computeConvolution(const glm::vec3& N, const GGXSamples& samples) const {
    // from tangent-space vector to world-space
    glm::vec3 bitangent = std::abs(N.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 tangent = glm::normalize(glm::cross(bitangent, N));
    bitangent = glm::cross(N, tangent);

    const size_t sampleCount = samples.points.size();
    glm::vec4 prefilteredColor = glm::vec4(0.0f);

    for (size_t i = 0; i < sampleCount; ++i) {
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