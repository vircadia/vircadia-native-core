//
//  BlendFace.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "BlendFace.h"
#include "Head.h"

using namespace fs;

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
    glScalef(_owningHead->getScale(), _owningHead->getScale(), _owningHead->getScale());

    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    // update the blended vertices
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, _baseVertices.size() * sizeof(fsVector3f), _baseVertices.data());
    
    // tell OpenGL where to find vertex information
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboID);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, _baseVertices.size() - 1, _indexCount, GL_UNSIGNED_INT, 0);

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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, rig.mesh().m_tris.size() * sizeof(fsVector3i),
        rig.mesh().m_tris.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferData(GL_ARRAY_BUFFER, rig.mesh().m_vertex_data.m_vertices.size() * sizeof(fsVector3f), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    _indexCount = rig.mesh().m_tris.size() * 3;
    _baseVertices = rig.mesh().m_vertex_data.m_vertices;
    _blendshapes = rig.blendshapes();
}
