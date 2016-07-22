//
//  Created by Bradley Austin Davis on 2016/05/26
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Shapes.h"

namespace geometry {

using glm::vec3;

// The golden ratio
static const float PHI = 1.61803398874f;

Solid<3> tesselate(const Solid<3>& solid_, int count) {
    Solid<3> solid = solid_;
    float length = glm::length(solid.vertices[0]);
    for (int i = 0; i < count; ++i) {
        Solid<3> result { solid.vertices, {} };
        result.vertices.reserve(solid.vertices.size() + solid.faces.size() * 3);
        for (size_t f = 0; f < solid.faces.size(); ++f) {
            Index baseVertex = (Index)result.vertices.size();
            const Face<3>& oldFace = solid.faces[f];
            const vec3& a = solid.vertices[oldFace[0]];
            const vec3& b = solid.vertices[oldFace[1]];
            const vec3& c = solid.vertices[oldFace[2]];
            vec3 ab = glm::normalize(a + b) * length;
            vec3 bc = glm::normalize(b + c) * length;
            vec3 ca = glm::normalize(c + a) * length;
            result.vertices.push_back(ab);
            result.vertices.push_back(bc);
            result.vertices.push_back(ca);
            result.faces.push_back(Face<3>{ { oldFace[0], baseVertex, baseVertex + 2 } });
            result.faces.push_back(Face<3>{ { baseVertex, oldFace[1], baseVertex + 1 } });
            result.faces.push_back(Face<3>{ { baseVertex + 1, oldFace[2], baseVertex + 2 } });
            result.faces.push_back(Face<3>{ { baseVertex, baseVertex + 1, baseVertex + 2 } });
        }
        solid = result;
    }
    return solid;
}

const Solid<3>& tetrahedron() {
    static const auto A = vec3(1, 1, 1);
    static const auto B = vec3(1, -1, -1);
    static const auto C = vec3(-1, 1, -1);
    static const auto D = vec3(-1, -1, 1);
    static const Solid<3> TETRAHEDRON = Solid<3>{
        { A, B, C, D },
        FaceVector<3>{
            Face<3> { { 0, 1, 2 } },
                Face<3> { { 3, 1, 0 } },
                Face<3> { { 2, 3, 0 } },
                Face<3> { { 2, 1, 3 } },
        }
    }.fitDimension(0.5f);
    return TETRAHEDRON;
}

const Solid<4>& cube() {
    static const auto A = vec3(1, 1, 1);
    static const auto B = vec3(-1, 1, 1);
    static const auto C = vec3(-1, 1, -1);
    static const auto D = vec3(1, 1, -1);
    static const Solid<4> CUBE = Solid<4>{
        { A, B, C, D, -A, -B, -C, -D },
        FaceVector<4>{
            Face<4> { { 3, 2, 1, 0 } },
                Face<4> { { 0, 1, 7, 6 } },
                Face<4> { { 1, 2, 4, 7 } },
                Face<4> { { 2, 3, 5, 4 } },
                Face<4> { { 3, 0, 6, 5 } },
                Face<4> { { 4, 5, 6, 7 } },
        }
    }.fitDimension(0.5f);
    return CUBE;
}

const Solid<3>& octahedron() {
    static const auto A = vec3(0, 1, 0);
    static const auto B = vec3(0, -1, 0);
    static const auto C = vec3(0, 0, 1);
    static const auto D = vec3(0, 0, -1);
    static const auto E = vec3(1, 0, 0);
    static const auto F = vec3(-1, 0, 0);
    static const Solid<3> OCTAHEDRON = Solid<3>{
        { A, B, C, D, E, F},
        FaceVector<3> {
            Face<3> { { 0, 2, 4, } },
            Face<3> { { 0, 4, 3, } },
            Face<3> { { 0, 3, 5, } },
            Face<3> { { 0, 5, 2, } },
            Face<3> { { 1, 4, 2, } },
            Face<3> { { 1, 3, 4, } },
            Face<3> { { 1, 5, 3, } },
            Face<3> { { 1, 2, 5, } },
        }
    }.fitDimension(0.5f);
    return OCTAHEDRON;
}

const Solid<5>& dodecahedron() {
    static const float P = PHI;
    static const float IP = 1.0f / PHI;
    static const vec3 A = vec3(IP, P, 0);
    static const vec3 B = vec3(-IP, P, 0);
    static const vec3 C = vec3(-1, 1, 1);
    static const vec3 D = vec3(0, IP, P);
    static const vec3 E = vec3(1, 1, 1);
    static const vec3 F = vec3(1, 1, -1);
    static const vec3 G = vec3(-1, 1, -1);
    static const vec3 H = vec3(-P, 0, IP);
    static const vec3 I = vec3(0, -IP, P);
    static const vec3 J = vec3(P, 0, IP);

    static const Solid<5> DODECAHEDRON = Solid<5>{
        {
            A,  B,  C,  D,  E,  F,  G,  H,  I,  J,
            -A, -B, -C, -D, -E, -F, -G, -H, -I, -J,
        },
        FaceVector<5> {
            Face<5> { { 0, 1, 2, 3, 4 } },
            Face<5> { { 0, 5, 18, 6, 1 } },
            Face<5> { { 1, 6, 19, 7, 2 } },
            Face<5> { { 2, 7, 15, 8, 3 } },
            Face<5> { { 3, 8, 16, 9, 4 } },
            Face<5> { { 4, 9, 17, 5, 0 } },
            Face<5> { { 14, 13, 12, 11, 10 } },
            Face<5> { { 11, 16, 8, 15, 10 } },
            Face<5> { { 12, 17, 9, 16, 11 } },
            Face<5> { { 13, 18, 5, 17, 12 } },
            Face<5> { { 14, 19, 6, 18, 13 } },
            Face<5> { { 10, 15, 7, 19, 14 } },
        }
    }.fitDimension(0.5f);
    return DODECAHEDRON;
}

const Solid<3>& icosahedron() {
    static const float N = 1.0f / PHI;
    static const float P = 1.0f;
    static const auto A = vec3(N, P, 0);
    static const auto B = vec3(-N, P, 0);
    static const auto C = vec3(0, N, P);
    static const auto D = vec3(P, 0, N);
    static const auto E = vec3(P, 0, -N);
    static const auto F = vec3(0, N, -P);

    static const Solid<3> ICOSAHEDRON = Solid<3> {
        {
            A, B, C, D, E, F,
            -A, -B, -C, -D, -E, -F,
        },
        FaceVector<3> {
            Face<3> { { 1, 2, 0 } },
            Face<3> { { 2, 3, 0 } },
            Face<3> { { 3, 4, 0 } },
            Face<3> { { 4, 5, 0 } },
            Face<3> { { 5, 1, 0 } },

            Face<3> { { 1, 10, 2 } },
            Face<3> { { 11, 2, 10 } },
            Face<3> { { 2, 11, 3 } },
            Face<3> { { 7, 3, 11 } },
            Face<3> { { 3, 7, 4 } },
            Face<3> { { 8, 4, 7 } },
            Face<3> { { 4, 8, 5 } },
            Face<3> { { 9, 5, 8 } },
            Face<3> { { 5, 9, 1 } },
            Face<3> { { 10, 1, 9 } },

            Face<3> { { 8, 7, 6 } },
            Face<3> { { 9, 8, 6 } },
            Face<3> { { 10, 9, 6 } },
            Face<3> { { 11, 10, 6 } },
            Face<3> { { 7, 11, 6 } },
        }
    }.fitDimension(0.5f);
    return ICOSAHEDRON;
}

}

