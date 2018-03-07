//
//  Created by Bradley Austin Davis on 2016/05/26
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Shapes.h"

#include "qmath.h"

namespace geometry {

using glm::vec2;
using glm::vec3;

// The golden ratio
static const float PHI = 1.61803398874f;

Solid<3> tesselate(const Solid<3>& solid_, int count) {
    Solid<3> solid = solid_;
    float length = glm::length(solid.vertices[0]);
    for (int i = 0; i < count; ++i) {
        Solid<3> result { solid.vertices, {}, {} };
        result.vertices.reserve(solid.vertices.size() + solid.faces.size() * 6);
        for (size_t f = 0; f < solid.faces.size(); ++f) {
            Index baseVertex = (Index)result.vertices.size();
            const Face<3>& oldFace = solid.faces[f];
            const vec3& a = solid.vertices[oldFace[0]];
            const vec3& b = solid.vertices[oldFace[1]];
            const vec3& c = solid.vertices[oldFace[2]];
            vec3 ab = glm::normalize(a + b) * length;
            vec3 bc = glm::normalize(b + c) * length;
            vec3 ca = glm::normalize(c + a) * length;

            result.vertices.push_back(a);
            result.vertices.push_back(ab);
            result.vertices.push_back(ca);
            result.faces.push_back(Face<3>{ { baseVertex, baseVertex + 1, baseVertex + 2 } });

            result.vertices.push_back(ab);
            result.vertices.push_back(b);
            result.vertices.push_back(bc);
            result.faces.push_back(Face<3>{ { baseVertex + 3, baseVertex + 4, baseVertex + 5 } });

            result.vertices.push_back(bc);
            result.vertices.push_back(c);
            result.vertices.push_back(ca);
            result.faces.push_back(Face<3>{ { baseVertex + 6, baseVertex + 7, baseVertex + 8 } });

            result.vertices.push_back(ab);
            result.vertices.push_back(bc);
            result.vertices.push_back(ca);
            result.faces.push_back(Face<3>{ { baseVertex + 9, baseVertex + 10, baseVertex + 11 } });
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
        { vec2(0.75f, 0.5f), vec2(0.5f, 0.0f), vec2(0.25f, 0.5f),
          vec2(0.5f, 1.0f), vec2(1.0f, 1.0f), vec2(0.75f, 0.5f),
          vec2(0.25f, 0.5f), vec2(0.5f, 1.0f), vec2(0.75f, 0.5f),
          vec2(0.25f, 0.5f), vec2(0.0f, 1.0f), vec2(0.5f, 1.0f) },
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
    static const float THIRD = 1.0f / 3.0f;
    static const Solid<4> CUBE = Solid<4>{
        { A, B, C, D, -A, -B, -C, -D },
        { vec2(0.5f, 0.0f), vec2(0.25f, 0.0f), vec2(0.25f, THIRD), vec2(0.5f, THIRD),
          vec2(0.5f, THIRD), vec2(0.25f, THIRD), vec2(0.25f, 2.0f * THIRD), vec2(0.5f, 2.0f * THIRD),
          vec2(0.25f, THIRD), vec2(0.0f, THIRD), vec2(0.0f, 2.0f * THIRD), vec2(0.25f, 2.0f * THIRD),
          vec2(1.0f, THIRD), vec2(0.75f, THIRD), vec2(0.75f, 2.0f * THIRD), vec2(1.0f, 2.0f * THIRD),
          vec2(0.75f, THIRD), vec2(0.5f, THIRD), vec2(0.5f, 2.0f * THIRD), vec2(0.75f, 2.0f * THIRD),
          vec2(0.25f, 1.0f), vec2(0.5f, 1.0f), vec2(0.5f, 2.0f * THIRD), vec2(0.25f, 2.0f * THIRD) },
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
    static const float THIRD = 1.0f / 3.0f;
    static const float SEVENTH = 1.0f / 7.0f;
    static const Solid<3> OCTAHEDRON = Solid<3>{
        { A, B, C, D, E, F},
        { vec2(2.0f * SEVENTH, THIRD), vec2(SEVENTH, 2.0f * THIRD), vec2(3.0f * SEVENTH, 2.0f * THIRD),
          vec2(2.0f * SEVENTH, THIRD), vec2(3.0f * SEVENTH, 2.0f * THIRD), vec2(4.0f * SEVENTH, THIRD),
          vec2(5.0f * SEVENTH, 0.0f), vec2(4.0f * SEVENTH, THIRD), vec2(6.0f * SEVENTH, THIRD),
          vec2(2.0f * SEVENTH, THIRD), vec2(0.0f, THIRD), vec2(1.0f * SEVENTH, 2.0f * THIRD),
          vec2(2.0f * SEVENTH, 1.0f), vec2(3.0f * SEVENTH, 2.0f * THIRD), vec2(1.0f * SEVENTH, 2.0f * THIRD),
          vec2(5.0f * SEVENTH, 2.0f * THIRD), vec2(4.0f * SEVENTH, THIRD), vec2(3.0f * SEVENTH, 2.0f * THIRD),
          vec2(5.0f * SEVENTH, 2.0f * THIRD), vec2(6.0f * SEVENTH, THIRD), vec2(4.0f * SEVENTH, THIRD),
          vec2(5.0f * SEVENTH, 2.0f * THIRD), vec2(1.0f, 2.0f * THIRD), vec2(6.0f * SEVENTH, THIRD) },
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


    /*                                   _
                          / \            |
                        /     \          y2
                      /         \        |
                    /             \      _
                   \               /     |
                    \             /      y1
                     \           /       |
                      ___________        _
                  |x3|- - x1 - -||x3|
                  |- - - - x2- - - -|
    */

    // x1, x2, and x3 are the solutions to the following system of equations:
    // 1 = 3 * x1 + 3 * x2 + x3
    // x1 + 2 * x3 = (golden ratio) * x1
    // x2 = x1 + 2 * x3
    static const float x1 = 4.0f / (17.0f + 7.0f * sqrtf(5.0f));
    static const float x2 = (1.0f / 11.0f) * (5.0f * sqrtf(5.0f) - 9.0f);
    static const float x2_2 = x2 / 2.0f;
    static const float x3 = (1.0f / 11.0f) * (6.0f * sqrtf(5.0f) - 13.0f);
    // y1 and y2 are the solutions to the following system of equations (x is the sidelength, but is different than x1 because the scale in the y direction is different):
    // 1 = 3 * y1 + 2 * y2
    // y1 = sin(108 deg) * x
    // y1 + y2 = x * sqrtf(5 + 2 * sqrtf(5)) / 2
    static const float y1 = sqrtf(2.0f * (5.0f + sqrtf(5.0f))) / (sqrtf(2.0f * (5.0f + sqrtf(5.0f))) + 4.0f * sqrtf(5.0f + 2.0f * sqrtf(5.0f)));
    static const float y2 = -(sqrtf(2.0f * (5.0f + sqrtf(5.0f))) - 2.0f * sqrtf(5.0f + 2.0f * sqrtf(5.0f))) / (sqrtf(2.0f * (5.0f + sqrtf(5.0f))) + 4.0f * sqrtf(5.0f + 2.0f * sqrtf(5.0f)));

    static const Solid<5> DODECAHEDRON = Solid<5>{
        {
            A,  B,  C,  D,  E,  F,  G,  H,  I,  J,
            -A, -B, -C, -D, -E, -F, -G, -H, -I, -J,
        },
        { vec2(x1 + x2_2, 0.0f), vec2(x2_2, 0.0f), vec2(x2_2 - x3, y1), vec2(x3 + x1, y1 + y2), vec2(x3 + x1 + x2_2, y1),
          vec2(1.0f - (x2 - x3 + x2_2), 0.0f), vec2(1.0f - (x2 + x1 + x3), y2), vec2(1.0f - (x2 + x1), y1 + y2), vec2(1.0f - x2, y1 + y2), vec2(1.0f - (x2 - x3), y2),
          vec2(1.0f - x2_2, y1), vec2(1.0f - x2, y1 + y2), vec2(1.0f - (x3 + x1), y1 + y2 + y1), vec2(1.0f - x3, y1 + y2 + y1), vec2(1.0f, y1 + y2),
          vec2(x3, y1 + y2), vec2(0.0f, y1 + y2 + y1), vec2(x2_2, y1 + y2 + y1 + y2), vec2(x2, y1 + y2 + y1), vec2(x1 + x3, y1 + y2),
          vec2(x3 + x1, y1 + y2), vec2(x2, y1 + y2 + y1), vec2(x2 + x1, y1 + y2 + y1), vec2(x3 + x1 + x2, y1 + y2), vec2(x3 + x1 + x2_2, y1),
          vec2(x3 + x1 + x2_2, y1), vec2(x3 + x1 + x2, y1 + y2), vec2(x3 + x1 + x2 + x2_2, y1), vec2(x1 + x2 + x2_2, 0.0f), vec2(x3 + x1 + x2_2 + x3, 0.0f),
          vec2(1.0f - (x3 + x1 + x2_2), 1.0f - y1), vec2(1.0f - (x3 + x1 + x2), 1.0f - (y1 + y2)), vec2(1.0f - (x3 + x1 + x2 + x2_2), 1.0f - y1), vec2(1.0f - (x1 + x2 + x2_2), 1.0f), vec2(1.0f - (x3 + x1 + x2_2 + x3), 1.0f),
          vec2(x2 + x1 + x3, 1.0f - y2), vec2(x2 + x1, 1.0f - (y1 + y2)), vec2(x2, 1.0f - (y1 + y2)), vec2(x2 - x3, 1.0f - y2), vec2(x2 - x3 + x2_2, 1.0f),
          vec2(x2 + x1 + x2, y1 + y2 + y1), vec2(x3 + x1 + x2 + x1, y1 + y2), vec2(x3 + x1 + x2, y1 + y2), vec2(x2 + x1, y1 + y2 + y1), vec2(x2 + x1 + x2_2, y1 + y2 + y1 + y2),
          vec2(1.0f - (x3 + x1 + x2), y1 + y2 + y1), vec2(1.0f - (x2 + x1), y1 + y2), vec2(1.0f - (x2 + x1 + x2_2), y1), vec2(1.0f - (x2 + x1 + x2), y1 + y2), vec2(1.0f - (x3 + x1 + x2 + x1), y1 + y2 + y1),
          vec2(1.0f - (x3 + x1 + x2_2), y1 + y2 + y1 + y2), vec2(1.0f - (x3 + x1), y1 + y2 + y1), vec2(1.0f - x2, y1 + y2), vec2(1.0f - (x2 + x1), y2 + y1), vec2(1.0f - (x3 + x1 + x2), y1 + y2 + y1),
          vec2(1.0f - (x1 + x2_2), 1.0f), vec2(1.0f - x2_2, 1.0f), vec2(1.0f - (x2_2 - x3), 1.0f - y1), vec2(1.0f - (x1 + x3), 1.0f - (y1 + y2)), vec2(1.0f - (x3 + x1 + x2_2), 1.0f - y1) },
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
    static const float THIRD = 1.0f / 3.0f;
    static const float ELEVENTH = 1.0f / 11.0f;
    static const Solid<3> ICOSAHEDRON = Solid<3> {
        {
            A, B, C, D, E, F,
            -A, -B, -C, -D, -E, -F,
        },
        { vec2(3.0f * ELEVENTH, 0.0f), vec2(2.0f * ELEVENTH, THIRD), vec2(4.0f * ELEVENTH, THIRD),
          vec2(2.0f * ELEVENTH, THIRD), vec2(3.0f * ELEVENTH, 2.0f * THIRD), vec2(4.0f * ELEVENTH, THIRD),
          vec2(3.0f * ELEVENTH, 2.0f * THIRD), vec2(5.0f * ELEVENTH, 2.0f * THIRD), vec2(4.0f * ELEVENTH, THIRD),
          vec2(5.0f * ELEVENTH, 2.0f * THIRD), vec2(6.0f * ELEVENTH, THIRD), vec2(4.0f * ELEVENTH, THIRD),
          vec2(6.0f * ELEVENTH, THIRD), vec2(5.0f * ELEVENTH, 0.0f), vec2(4.0f * ELEVENTH, THIRD),
          vec2(1.0f * ELEVENTH, 0.0f), vec2(0.0f, THIRD), vec2(2.0f * ELEVENTH, THIRD),
          vec2(1.0f * ELEVENTH, 2.0f * THIRD), vec2(2.0f * ELEVENTH, THIRD), vec2(0.0f, THIRD),
          vec2(2.0f * ELEVENTH, THIRD), vec2(1.0f * ELEVENTH, 2.0f * THIRD), vec2(3.0f * ELEVENTH, 2.0f * THIRD),
          vec2(2.0f * ELEVENTH, 1.0f), vec2(3.0f * ELEVENTH, 2.0f * THIRD), vec2(1.0f * ELEVENTH, 2.0f * THIRD),
          vec2(3.0f * ELEVENTH, 2.0f * THIRD), vec2(4.0f * ELEVENTH, 1.0f), vec2(5.0f * ELEVENTH, 2.0f * THIRD),
          vec2(7.0f * ELEVENTH, 2.0f * THIRD), vec2(5.0f * ELEVENTH, 2.0f * THIRD), vec2(6.0f * ELEVENTH, 1.0f),
          vec2(5.0f * ELEVENTH, 2.0f * THIRD), vec2(7.0f * ELEVENTH, 2.0f * THIRD), vec2(6.0f * ELEVENTH, THIRD),
          vec2(8.0f * ELEVENTH, THIRD), vec2(6.0f * ELEVENTH, THIRD), vec2(7.0f * ELEVENTH, 2.0f * THIRD),
          vec2(6.0f * ELEVENTH, THIRD), vec2(8.0f * ELEVENTH, THIRD), vec2(7.0f * ELEVENTH, 0.0f),
          vec2(10.0f * ELEVENTH, THIRD), vec2(9.0f * ELEVENTH, 0.0f), vec2(8.0f * ELEVENTH, THIRD),
          vec2(7.0f * ELEVENTH, 2.0f * THIRD), vec2(8.0f * ELEVENTH, 1.0f), vec2(9.0f * ELEVENTH, 2.0f * THIRD),
          vec2(8.0f * ELEVENTH, THIRD), vec2(7.0f * ELEVENTH, 2.0f * THIRD), vec2(9.0f * ELEVENTH, 2.0f * THIRD),
          vec2(10.0f * ELEVENTH, THIRD), vec2(8.0f * ELEVENTH, THIRD), vec2(9.0f * ELEVENTH, 2.0f * THIRD),
          vec2(1.0f, 2.0f * THIRD), vec2(10.0f * ELEVENTH, THIRD), vec2(9.0f * ELEVENTH, 2.0f * THIRD),
          vec2(10.0f * ELEVENTH, 1.0f), vec2(1.0f, 2.0f * THIRD), vec2(9.0f * ELEVENTH, 2.0f * THIRD) },
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

