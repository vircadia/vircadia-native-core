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

#ifndef hifi_image_CubeMap_h
#define hifi_image_CubeMap_h

#include <gpu/Texture.h>
#include <glm/vec4.hpp>
#include <vector>
#include <array>
#include <atomic>

#include "Image.h"

namespace image {

    class CubeMap {

        enum {
            EDGE_WIDTH = 1
        };

    public:
 
        CubeMap(int width, int height, int mipCount);
        CubeMap(const std::vector<Image>& faces, gpu::Element faceFormat, int mipCount, const std::atomic<bool>& abortProcessing = false);

        void reset(int width, int height, int mipCount);
        void copyTo(CubeMap& other) const;

        gpu::uint16 getMipCount() const { return (gpu::uint16)_mips.size(); }
        int getMipWidth(gpu::uint16 mipLevel) const {
            return std::max(1, _width >> mipLevel);
        }
        int getMipHeight(gpu::uint16 mipLevel) const {
            return std::max(1, _height >> mipLevel);
        }
        gpu::Vec2i getMipDimensions(gpu::uint16 mipLevel) const {
            return gpu::Vec2i(getMipWidth(mipLevel), getMipHeight(mipLevel));
        }

        size_t getMipLineStride(gpu::uint16 mipLevel) const {
            return getMipWidth(mipLevel) + 2 * EDGE_WIDTH;
        }

        glm::vec4* editFace(gpu::uint16 mipLevel, int face) {
            return _mips[mipLevel][face].data() + (getMipLineStride(mipLevel) + 1)*EDGE_WIDTH;
        }

        const glm::vec4* getFace(gpu::uint16 mipLevel, int face) const {
            return _mips[mipLevel][face].data() + (getMipLineStride(mipLevel) + 1)*EDGE_WIDTH;
        }

        Image getFaceImage(gpu::uint16 mipLevel, int face) const;

        void convolveForGGX(CubeMap& output, const std::atomic<bool>& abortProcessing) const;
        glm::vec4 fetchLod(const glm::vec3& dir, float lod) const;

    private:

        struct GGXSamples;
        class Mip;
        class ConstMip;

        using Face = std::vector<glm::vec4>;
        using Faces = std::array<Face, 6>;

        int _width;
        int _height;
        std::vector<Faces> _mips;

        static void getFaceUV(const glm::vec3& dir, int* index, glm::vec2* uv);
        static void generateGGXSamples(GGXSamples& data, float roughness, const int resolution);
        static void copyFace(int width, int height, const glm::vec4* source, size_t srcLineStride, glm::vec4* dest, size_t dstLineStride);
        void convolveMipFaceForGGX(const GGXSamples& samples, CubeMap& output, gpu::uint16 mipLevel, int face, const std::atomic<bool>& abortProcessing) const;
        glm::vec4 computeConvolution(const glm::vec3& normal, const GGXSamples& samples) const;

    };

}

#endif // hifi_image_CubeMap_h
