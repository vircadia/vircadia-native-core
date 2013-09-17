//
//  BlendFace.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <numeric>

#include "BlendFace.h"
#include "Head.h"

using namespace fs;
using namespace std;

BlendFace::BlendFace(Head* owningHead) :
    _owningHead(owningHead),
    _iboID(0)
{
}

BlendFace::~BlendFace() {
    if (_iboID != 0) {
        glDeleteBuffers(1, &_iboID);
        glDeleteBuffers(1, &_vboID);
    }
}

bool BlendFace::render(float alpha) {
    if (_iboID == 0) {
        return false;
    }

    glPushMatrix();
    glTranslatef(_owningHead->getPosition().x, _owningHead->getPosition().y, _owningHead->getPosition().z);
    glm::quat orientation = _owningHead->getOrientation();
    glm::vec3 axis = glm::axis(orientation);
    glRotatef(glm::angle(orientation), axis.x, axis.y, axis.z);
    glTranslatef(0.0f, -0.025f, -0.025f);
    const float MODEL_SCALE = 0.0006f;
    glScalef(_owningHead->getScale() * MODEL_SCALE, _owningHead->getScale() * MODEL_SCALE,
        -_owningHead->getScale() * MODEL_SCALE);

    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    // find the coefficient total and scale
    const vector<float>& coefficients = _owningHead->getBlendshapeCoefficients();
    float total = accumulate(coefficients.begin(), coefficients.end(), 0.0f);
    float scale = 1.0f / (total < 1.0f ? 1.0f : total);
    
    // start with the base
    int vertexCount = _baseVertices.size();
    _blendedVertices.resize(vertexCount);
    const fsVector3f* source = _baseVertices.data();
    fsVector3f* dest = _blendedVertices.data();
    float baseCoefficient = (total < 1.0f) ? (1.0f - total) : 0.0f;
    for (int i = 0; i < vertexCount; i++) {
        dest->x += source->x * baseCoefficient;
        dest->y += source->y * baseCoefficient;
        dest->z += source->z * baseCoefficient;
        
        source++;
        dest++;
    }
    
    // blend in each coefficient
    for (int i = 0; i < coefficients.size(); i++) {
        float coefficient = coefficients[i] * scale;
        if (coefficient == 0.0f || i >= _blendshapes.size()) {
            continue;
        }
        const fsVector3f* source = _blendshapes[i].m_vertices.data();
        fsVector3f* dest = _blendedVertices.data();
        for (int j = 0; j < vertexCount; j++) {
            dest->x += source->x * coefficient;
            dest->y += source->y * coefficient;
            dest->z += source->z * coefficient;
            
            source++;
            dest++;
        }
    }
    
    // update the blended vertices
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, _blendedVertices.size() * sizeof(fsVector3f), _blendedVertices.data());
    
    // tell OpenGL where to find vertex information
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboID);
    glDrawRangeElementsEXT(GL_QUADS, 0, _baseVertices.size() - 1, _quadIndexCount, GL_UNSIGNED_INT, 0);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, _baseVertices.size() - 1, _triangleIndexCount, GL_UNSIGNED_INT,
        (void*)(_quadIndexCount * sizeof(int)));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // deactivate vertex arrays after drawing
    glDisableClientState(GL_VERTEX_ARRAY);
    
    // bind with 0 to switch back to normal operation
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glPopMatrix();

    return true;
}

void BlendFace::setRig(const fsMsgRig& rig) {
    if (rig.mesh().m_tris.empty()) {
        // clear any existing geometry
        if (_iboID != 0) {
            glDeleteBuffers(1, &_iboID);
            glDeleteBuffers(1, &_vboID);
            _iboID = 0;
        }
        return;
    }
    if (_iboID == 0) {
        glGenBuffers(1, &_iboID);
        glGenBuffers(1, &_vboID);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, rig.mesh().m_quads.size() * sizeof(fsVector4i) +
        rig.mesh().m_tris.size() * sizeof(fsVector3i), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, rig.mesh().m_quads.size() * sizeof(fsVector4i), rig.mesh().m_quads.data());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, rig.mesh().m_quads.size() * sizeof(fsVector4i),
        rig.mesh().m_tris.size() * sizeof(fsVector3i), rig.mesh().m_tris.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferData(GL_ARRAY_BUFFER, rig.mesh().m_vertex_data.m_vertices.size() * sizeof(fsVector3f), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    _quadIndexCount = rig.mesh().m_quads.size() * 4;
    _triangleIndexCount = rig.mesh().m_tris.size() * 3;
    _baseVertices = rig.mesh().m_vertex_data.m_vertices;
    _blendshapes = rig.blendshapes();
}
