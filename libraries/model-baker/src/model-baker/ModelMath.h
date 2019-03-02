//
//  ModelMath.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/01/07.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <hfm/HFM.h>

#include "BakerTypes.h"

namespace baker {
    // Returns a reference to the normal at the specified index, or nullptr if it cannot be accessed
    using NormalAccessor = std::function<glm::vec3*(int index)>;

    // Assigns a vertex to outVertex given the lookup index
    using VertexSetter = std::function<void(int index, glm::vec3& outVertex)>;

    void calculateNormals(const hfm::Mesh& mesh, NormalAccessor normalAccessor, VertexSetter vertexAccessor);

    // firstIndex, secondIndex: the vertex indices to be used for calculation
    // outVertices: should be assigned a 2 element array containing the vertices at firstIndex and secondIndex
    // outTexCoords: same as outVertices but for texture coordinates
    // outNormal: reference to the normal of this triangle
    //
    // Return value: pointer to the tangent you want to be calculated
    using IndexAccessor = std::function<glm::vec3*(int firstIndex, int secondIndex, glm::vec3* outVertices, glm::vec2* outTexCoords, glm::vec3& outNormal)>;

    void calculateTangents(const hfm::Mesh& mesh, IndexAccessor accessor);
};
