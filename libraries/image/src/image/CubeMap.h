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

namespace image {

    class CubeMap {
    public:
 
        CubeMap(int width, int height, int mipCount);
        CubeMap(gpu::TexturePointer texture, const std::atomic<bool>& abortProcessing = false);

        void reset(int width, int height, int mipCount);

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

        glm::vec4* editFace(gpu::uint16 mipLevel, int face) {
            return _mips[mipLevel][face].data() + getFaceLineStride(mipLevel) + 1;
        }

        const glm::vec4* getFace(gpu::uint16 mipLevel, int face) const {
            return _mips[mipLevel][face].data() + getFaceLineStride(mipLevel) + 1;
        }

        size_t getFaceLineStride(gpu::uint16 mipLevel) const {
            return getMipWidth(mipLevel)+2;
        }

        void convolveForGGX(CubeMap& output, const std::atomic<bool>& abortProcessing) const;
        glm::vec4 fetchLod(const glm::vec3& dir, float lod) const;

    private:

        struct GGXSamples;

        using Face = std::vector<glm::vec4>;
        using Faces = std::array<Face, 6>;

        int _width;
        int _height;
        std::vector<Faces> _mips;

        static void generateGGXSamples(GGXSamples& data, float roughness, const int resolution);
        void convolveMipFaceForGGX(const GGXSamples& samples, CubeMap& output, gpu::uint16 mipLevel, int face, const std::atomic<bool>& abortProcessing) const;
        glm::vec4 computeConvolution(const glm::vec3& normal, const GGXSamples& samples) const;

        void seamColumnAndColumn(gpu::uint16 mipLevel, int face0, int col0, int face1, int col1, int inc);
        void seamColumnAndRow(gpu::uint16 mipLevel, int face0, int col0, int face1, int row1, int inc);
        void seamRowAndRow(gpu::uint16 mipLevel, int face0, int row0, int face1, int row1, int inc);

        void copyColumnToColumn(gpu::uint16 mipLevel, int srcFace, int srcCol, int dstFace, int dstCol, int dstInc);
        void copyColumnToRow(gpu::uint16 mipLevel, int srcFace, int srcCol, int dstFace, int dstRow, int dstInc);
        void copyRowToRow(gpu::uint16 mipLevel, int srcFace, int srcRow, int dstFace, int dstRow, int dstInc);
        void copyRowToColumn(gpu::uint16 mipLevel, int srcFace, int srcRow, int dstFace, int dstCol, int dstInc);

    };

}

#endif // hifi_image_CubeMap_h
