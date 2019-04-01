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
#include "TextureProcessing.h"
#include "ImageLogging.h"

#include <nvtt/nvtt.h>

#ifndef M_PI
#define M_PI    3.14159265359
#endif

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
        _lineStride = _dims.x + 2;
    }

    CubeFaceMip(const CubeFaceMip& other) : _dims(other._dims), _lineStride(other._lineStride) {

    }

    gpu::Vec2i _dims;
    int _lineStride;
};

class CubeMap::ConstMip : public CubeFaceMip {
public:

    ConstMip(gpu::uint16 level, const CubeMap* cubemap) : 
        CubeFaceMip(level, cubemap), _faces(cubemap->_mips[level]) {
    }

    glm::vec4 fetch(int face, glm::vec2 uv) const {
        glm::vec2 coordFrac = uv * glm::vec2(_dims) + 0.5f;
        glm::vec2 coords = glm::floor(coordFrac);

        coordFrac -= coords;

        const auto* pixels = _faces[face].data();
        gpu::Vec2i loCoords(coords);
        const int offset = loCoords.x + loCoords.y * _lineStride;
        glm::vec4 colorLL = pixels[offset];
        glm::vec4 colorHL = pixels[offset +1 ];
        glm::vec4 colorLH = pixels[offset + _lineStride];
        glm::vec4 colorHH = pixels[offset + 1 + _lineStride];

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
        // Copy edge rows and columns from neighbouring faces to fix seam filtering issues
        seamColumnAndRow(gpu::Texture::CUBE_FACE_TOP_POS_Y, _dims.x, gpu::Texture::CUBE_FACE_RIGHT_POS_X, -1, -1);
        seamColumnAndRow(gpu::Texture::CUBE_FACE_BOTTOM_NEG_Y, _dims.x, gpu::Texture::CUBE_FACE_RIGHT_POS_X, _dims.y, 1);
        seamColumnAndColumn(gpu::Texture::CUBE_FACE_FRONT_NEG_Z, 0, gpu::Texture::CUBE_FACE_RIGHT_POS_X, _dims.x, 1);
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

    inline static void copy(const glm::vec4* srcFirst, const glm::vec4* srcLast, int srcStride, glm::vec4* dstBegin, int dstStride) {
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
        auto srcFirst = _faces[srcFace].data() + srcCol + _lineStride;
        auto srcLast = srcFirst + lastOffset;

        auto dstFirst = _faces[dstFace].data() + dstCol + _lineStride;
        auto dstLast = dstFirst + lastOffset;
        const auto dstStride = _lineStride * dstInc;

        if (dstInc < 0) {
            std::swap(dstFirst, dstLast);
        }

        copy(srcFirst, srcLast, _lineStride, dstFirst, dstStride);
    }

    void copyRowToRow(int srcFace, int srcRow, int dstFace, int dstRow, const int dstInc) {
        const auto lastOffset =(_dims.x - 1);
        auto srcFirst = _faces[srcFace].data() + srcRow * _lineStride + 1;
        auto srcLast = srcFirst + lastOffset;

        auto dstFirst = _faces[dstFace].data() + dstRow * _lineStride + 1;
        auto dstLast = dstFirst + lastOffset;

        if (dstInc < 0) {
            std::swap(dstFirst, dstLast);
        }

        copy(srcFirst, srcLast, 1, dstFirst, dstInc);
    }

    void copyColumnToRow(int srcFace, int srcCol, int dstFace, int dstRow, int dstInc) {
        const auto srcLastOffset = _lineStride * (_dims.y - 1);
        auto srcFirst = _faces[srcFace].data() + srcCol + _lineStride;
        auto srcLast = srcFirst + srcLastOffset;

        const auto dstLastOffset = (_dims.x - 1);
        auto dstFirst = _faces[dstFace].data() + dstRow * _lineStride + 1;
        auto dstLast = dstFirst + dstLastOffset;

        if (dstInc < 0) {
            std::swap(dstFirst, dstLast);
        }

        copy(srcFirst, srcLast, _lineStride, dstFirst, dstInc);
    }

    void copyRowToColumn(int srcFace, int srcRow, int dstFace, int dstCol, int dstInc) {
        const auto srcLastOffset = (_dims.x - 1);
        auto srcFirst = _faces[srcFace].data() + srcRow * _lineStride + 1;
        auto srcLast = srcFirst + srcLastOffset;

        const auto dstLastOffset = _lineStride * (_dims.y - 1);
        auto dstFirst = _faces[dstFace].data() + dstCol + _lineStride;
        auto dstLast = dstFirst + dstLastOffset;
        const auto dstStride = _lineStride * dstInc;

        if (dstInc < 0) {
            std::swap(dstFirst, dstLast);
        }

        copy(srcFirst, srcLast, 1, dstFirst, dstStride);
    }
};

CubeMap::CubeMap(int width, int height, int mipCount) {
    reset(width, height, mipCount);
}

struct CubeMap::MipMapOutputHandler : public nvtt::OutputHandler {
    MipMapOutputHandler(CubeMap* cube) : _cubemap(cube) {}

    void beginImage(int size, int width, int height, int depth, int face, int miplevel) override {
        _data = _cubemap->editFace(miplevel, face);
        _current = _data;
    }

    bool writeData(const void* data, int size) override {
        assert((size % sizeof(glm::vec4)) == 0);
        memcpy(_current, data, size);
        _current += size / sizeof(glm::vec4);
        return true;
    }

    void endImage() override {
        _data = nullptr;
        _current = nullptr;
    }

    CubeMap* _cubemap{ nullptr };
    glm::vec4* _data{ nullptr };
    glm::vec4* _current{ nullptr };
};

CubeMap::CubeMap(const std::vector<Image>& faces, gpu::Element srcTextureFormat, int mipCount, const std::atomic<bool>& abortProcessing) {
    reset(faces.front().getWidth(), faces.front().getHeight(), mipCount);

    int face;

    struct MipMapErrorHandler : public nvtt::ErrorHandler {
        virtual void error(nvtt::Error e) override {
            qCWarning(imagelogging) << "Texture mip map creation error:" << nvtt::errorString(e);
        }
    };

    // Compute mips
    for (face = 0; face < 6; face++) {
        auto sourcePixels = faces[face].getBits();
        auto floatPixels = editFace(0, face);

        convertToFloat(sourcePixels, _width, _height, faces[face].getBytesPerLineCount(), srcTextureFormat, floatPixels, _width);

        nvtt::Surface surface;
        surface.setImage(nvtt::InputFormat_RGBA_32F, _width, _height, 1, floatPixels);
        surface.setAlphaMode(nvtt::AlphaMode_None);
        surface.setWrapMode(nvtt::WrapMode_Clamp);

        auto mipLevel = 0;
        copyFace(_width, _height, reinterpret_cast<const glm::vec4*>(surface.data()), surface.width(), editFace(0, face), getFaceLineStride(0));

        while (surface.canMakeNextMipmap() && !abortProcessing.load()) {
            surface.buildNextMipmap(nvtt::MipmapFilter_Box);
            mipLevel++;

            copyFace(surface.width(), surface.height(), reinterpret_cast<const glm::vec4*>(surface.data()), surface.width(), editFace(mipLevel, face), getFaceLineStride(mipLevel));
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

void CubeMap::copyFace(int width, int height, const glm::vec4* source, int srcLineStride, glm::vec4* dest, int dstLineStride) {
    for (int y = 0; y < height; y++) {
        std::copy(source, source + width, dest);
        source += srcLineStride;
        dest += dstLineStride;
    }
}

void CubeMap::reset(int width, int height, int mipCount) {
    assert(mipCount >0 && _width > 0 && _height > 0);
    _width = width;
    _height = height;
    _mips.resize(mipCount);
    for (auto mipLevel = 0; mipLevel < mipCount; mipLevel++) {
        auto mipDimensions = getMipDimensions(mipLevel);
        // Add extra pixels on edges to perform edge seam fixup (we will duplicate pixels from
        // neighbouring faces)
        auto mipPixelCount = (mipDimensions.x+2) * (mipDimensions.y+2);

        for (auto& face : _mips[mipLevel]) {
            face.resize(mipPixelCount);
        }
    }
}

void CubeMap::copyTo(gpu::Texture* texture, const std::atomic<bool>& abortProcessing) const {
    assert(_width == texture->getWidth() && _height == texture->getHeight() && texture->getNumMips() == _mips.size());

    struct CompressionpErrorHandler : public nvtt::ErrorHandler {
        virtual void error(nvtt::Error e) override {
            qCWarning(imagelogging) << "Texture compression error:" << nvtt::errorString(e);
        }
    };

    CompressionpErrorHandler errorHandler;
    nvtt::OutputOptions outputOptions;
    outputOptions.setOutputHeader(false);
    outputOptions.setErrorHandler(&errorHandler);

    nvtt::Surface surface;
    surface.setAlphaMode(nvtt::AlphaMode_None);
    surface.setWrapMode(nvtt::WrapMode_Clamp);

    glm::vec4* packedPixels = new glm::vec4[_width * _height];
    for (int face = 0; face < 6; face++) {
        nvtt::CompressionOptions compressionOptions;
        std::unique_ptr<nvtt::OutputHandler> outputHandler{ getNVTTCompressionOutputHandler(texture, face, compressionOptions) };

        outputOptions.setOutputHandler(outputHandler.get());

        SequentialTaskDispatcher dispatcher(abortProcessing);
        nvtt::Context context;
        context.setTaskDispatcher(&dispatcher);

        for (gpu::uint16 mipLevel = 0; mipLevel < _mips.size() && !abortProcessing.load(); mipLevel++) {
            auto mipDims = getMipDimensions(mipLevel);

            copyFace(mipDims.x, mipDims.y, getFace(mipLevel, face), getFaceLineStride(mipLevel), packedPixels, mipDims.x);
            surface.setImage(nvtt::InputFormat_RGBA_32F, mipDims.x, mipDims.y, 1, packedPixels);
            context.compress(surface, face, mipLevel, compressionOptions, outputOptions);
        }

        if (abortProcessing.load()) {
            break;
        }
    }
    delete[] packedPixels;
}

void CubeMap::getFaceUV(const glm::vec3& dir, int* index, glm::vec2* uv) {
    // Taken from https://en.wikipedia.org/wiki/Cube_mapping
    float absX = std::abs(dir.x);
    float absY = std::abs(dir.y);
    float absZ = std::abs(dir.z);

    auto isXPositive = dir.x > 0;
    auto isYPositive = dir.y > 0;
    auto isZPositive = dir.z > 0;

    float maxAxis, uc, vc;

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
    const glm::vec3 deltaXNormalLo = faceNormals[1] - faceNormals[0];
    const glm::vec3 deltaXNormalHi = faceNormals[3] - faceNormals[2];
    auto outputFacePixels = output.editFace(mipLevel, face);
    auto outputLineStride = output.getFaceLineStride(mipLevel);

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

                outputFacePixels[x + y * outputLineStride] = computeConvolution(normal, samples);
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