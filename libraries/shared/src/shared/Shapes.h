//
//  Created by Bradley Austin Davis on 2016/05/26
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_shared_shapes
#define hifi_shared_shapes

#include <stdint.h>

#include <vector>
#include <array>

#include <glm/glm.hpp>

namespace geometry {

    using Index = uint32_t;
    using Vec = glm::vec3;
    using VertexVector = std::vector<Vec>;
    using IndexVector = std::vector<Index>;

    template <size_t N>
    using Face = std::array<Index, N>;

    template <size_t N>
    using FaceVector = std::vector<Face<N>>;

    template <size_t N>
    struct Solid {
        VertexVector vertices;
        FaceVector<N> faces;

        Solid<N>& fitDimension(float newMaxDimension) {
            float maxDimension = 0;
            for (const auto& vertex : vertices) {
                maxDimension = std::max(maxDimension, std::max(std::max(vertex.x, vertex.y), vertex.z));
            }
            float multiplier = newMaxDimension / maxDimension;
            for (auto& vertex : vertices) {
                vertex *= multiplier;
            }
            return *this;
        }

        Vec getFaceNormal(size_t faceIndex) const {
            Vec result;
            const auto& face = faces[faceIndex];
            for (size_t i = 0; i < N; ++i) {
                result += vertices[face[i]];
            }
            result /= N;
            return glm::normalize(result);
        }
    };

    template <size_t N>
    size_t triangulatedFaceTriangleCount() {
        return N - 2;
    }

    template <size_t N>
    size_t triangulatedFaceIndexCount() {
        return triangulatedFaceTriangleCount<N>() * 3;
    }

    Solid<3> tesselate(const Solid<3>& solid, int count);
    const Solid<3>& tetrahedron();
    const Solid<4>& cube();
    const Solid<3>& octahedron();
    const Solid<5>& dodecahedron();
    const Solid<3>& icosahedron();
}

#endif
