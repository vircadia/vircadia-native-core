//
//  MeshUtil.h
//  tests/physics/src
//
//  Created by Andrew Meadows 2016.07.14
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MeshUtil_h
#define hifi_MeshUtil_h

#include <functional>

namespace MeshUtil {

class TriangleEdge {
public:
    TriangleEdge() {}
    TriangleEdge(uint32_t A, uint32_t B) {
        setIndices(A, B);
    }
    void setIndices(uint32_t A, uint32_t B) {
        if (A < B) {
            _indexA = A;
            _indexB = B;
        } else {
            _indexA = B;
            _indexB = A;
        }
    }
    bool operator==(const TriangleEdge& other) const {
        return _indexA == other._indexA && _indexB == other._indexB;
    }

    uint32_t getIndexA() const { return _indexA; }
    uint32_t getIndexB() const { return _indexB; }
private:
    uint32_t _indexA { (uint32_t)(-1) };
    uint32_t _indexB { (uint32_t)(-1) };
};

} // MeshUtil namespace

namespace std {
    template <>
    struct hash<MeshUtil::TriangleEdge> {
        std::size_t operator()(const MeshUtil::TriangleEdge& edge) const {
            // use Cantor's pairing function to generate a hash of ZxZ --> Z
            uint32_t ab = edge.getIndexA() + edge.getIndexB();
            return hash<uint32_t>()((ab * (ab + 1)) / 2 + edge.getIndexB());
        }
    };
} // std namesspace

namespace MeshUtil {
bool isClosedManifold(const uint32_t* meshIndices, uint32_t numIndices) {
    using EdgeList = std::unordered_map<TriangleEdge, uint32_t>;
    EdgeList edges;

    // count the triangles for each edge
    const uint32_t TRIANGLE_STRIDE = 3;
    for (uint32_t i = 0; i < numIndices; i += TRIANGLE_STRIDE) {
        TriangleEdge edge;
        // the triangles indices are stored in sequential order
        for (uint32_t j = 0; j < 3; ++j) {
            edge.setIndices(meshIndices[i + j], meshIndices[i + ((j + 1) % 3)]);

            EdgeList::iterator edgeEntry = edges.find(edge);
            if (edgeEntry == edges.end()) {
                edges.insert(std::pair<TriangleEdge, uint32_t>(edge, 1));
            } else {
                edgeEntry->second += 1;
            }
        }
    }
    // scan for outside edge
    for (auto& edgeEntry : edges) {
        if (edgeEntry.second == 1) {
             return false;
        }
    }
    return true;
}

} // MeshUtil namespace


#endif // hifi_MeshUtil_h
