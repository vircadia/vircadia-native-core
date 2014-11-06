//
//  GeometryCache.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cmath>

#include <QNetworkReply>
#include <QRunnable>
#include <QThreadPool>

#include "Application.h"
#include "GeometryCache.h"
#include "Model.h"
#include "world.h"

GeometryCache::GeometryCache() :
    _pendingBlenders(0) {
}

GeometryCache::~GeometryCache() {
    foreach (const VerticesIndices& vbo, _hemisphereVBOs) {
        glDeleteBuffers(1, &vbo.first);
        glDeleteBuffers(1, &vbo.second);
    }
}

void GeometryCache::renderHemisphere(int slices, int stacks) {
    VerticesIndices& vbo = _hemisphereVBOs[IntPair(slices, stacks)];
    int vertices = slices * (stacks - 1) + 1;
    int indices = slices * 2 * 3 * (stacks - 2) + slices * 3;
    if (vbo.first == 0) {    
        GLfloat* vertexData = new GLfloat[vertices * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i < stacks - 1; i++) {
            float phi = PI_OVER_TWO * (float)i / (float)(stacks - 1);
            float z = sinf(phi), radius = cosf(phi);
            
            for (int j = 0; j < slices; j++) {
                float theta = TWO_PI * (float)j / (float)slices;

                *(vertex++) = sinf(theta) * radius;
                *(vertex++) = cosf(theta) * radius;
                *(vertex++) = z;
            }
        }
        *(vertex++) = 0.0f;
        *(vertex++) = 0.0f;
        *(vertex++) = 1.0f;
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        const int BYTES_PER_VERTEX = 3 * sizeof(GLfloat);
        glBufferData(GL_ARRAY_BUFFER, vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < stacks - 2; i++) {
            GLushort bottom = i * slices;
            GLushort top = bottom + slices;
            for (int j = 0; j < slices; j++) {
                int next = (j + 1) % slices;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        GLushort bottom = (stacks - 2) * slices;
        GLushort top = bottom + slices;
        for (int i = 0; i < slices; i++) {    
            *(index++) = bottom + i;
            *(index++) = bottom + (i + 1) % slices;
            *(index++) = top;
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        const int BYTES_PER_INDEX = sizeof(GLushort);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
    
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
 
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glNormalPointer(GL_FLOAT, 0, 0);
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

const int NUM_VERTICES_PER_TRIANGLE = 3;
const int NUM_TRIANGLES_PER_QUAD = 2;
const int NUM_VERTICES_PER_TRIANGULATED_QUAD = NUM_VERTICES_PER_TRIANGLE * NUM_TRIANGLES_PER_QUAD;
const int NUM_COORDS_PER_VERTEX = 3;
const int NUM_BYTES_PER_VERTEX = NUM_COORDS_PER_VERTEX * sizeof(GLfloat);
const int NUM_BYTES_PER_INDEX = sizeof(GLushort);

void GeometryCache::renderSphere(float radius, int slices, int stacks) {
    VerticesIndices& vbo = _sphereVBOs[IntPair(slices, stacks)];
    int vertices = slices * (stacks - 1) + 2;    
    int indices = slices * stacks * NUM_VERTICES_PER_TRIANGULATED_QUAD;
    if (vbo.first == 0) {        
        GLfloat* vertexData = new GLfloat[vertices * NUM_COORDS_PER_VERTEX];
        GLfloat* vertex = vertexData;

        // south pole
        *(vertex++) = 0.0f;
        *(vertex++) = 0.0f;
        *(vertex++) = -1.0f;

        //add stacks vertices climbing up Y axis
        for (int i = 1; i < stacks; i++) {
            float phi = PI * (float)i / (float)(stacks) - PI_OVER_TWO;
            float z = sinf(phi), radius = cosf(phi);
            
            for (int j = 0; j < slices; j++) {
                float theta = TWO_PI * (float)j / (float)slices;

                *(vertex++) = sinf(theta) * radius;
                *(vertex++) = cosf(theta) * radius;
                *(vertex++) = z;
            }
        }

        // north pole
        *(vertex++) = 0.0f;
        *(vertex++) = 0.0f;
        *(vertex++) = 1.0f;
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBufferData(GL_ARRAY_BUFFER, vertices * NUM_BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;

        // South cap
        GLushort bottom = 0;
        GLushort top = 1;
        for (int i = 0; i < slices; i++) {    
            *(index++) = bottom;
            *(index++) = top + i;
            *(index++) = top + (i + 1) % slices;
        }

        // (stacks - 2) ribbons
        for (int i = 0; i < stacks - 2; i++) {
            bottom = i * slices + 1;
            top = bottom + slices;
            for (int j = 0; j < slices; j++) {
                int next = (j + 1) % slices;
                
                *(index++) = top + next;
                *(index++) = bottom + j;
                *(index++) = top + j;
                
                *(index++) = bottom + next;
                *(index++) = bottom + j;
                *(index++) = top + next;
            }
        }
        
        // north cap
        bottom = (stacks - 2) * slices + 1;
        top = bottom + slices;
        for (int i = 0; i < slices; i++) {    
            *(index++) = bottom + (i + 1) % slices;
            *(index++) = bottom + i;
            *(index++) = top;
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * NUM_BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
    
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, 0);
    glNormalPointer(GL_FLOAT, 0, 0);

    glPushMatrix();
    glScalef(radius, radius, radius);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
    glPopMatrix();

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GeometryCache::renderSquare(int xDivisions, int yDivisions) {
    VerticesIndices& vbo = _squareVBOs[IntPair(xDivisions, yDivisions)];
    int xVertices = xDivisions + 1;
    int yVertices = yDivisions + 1;
    int vertices = xVertices * yVertices;
    int indices = 2 * 3 * xDivisions * yDivisions;
    if (vbo.first == 0) {
        GLfloat* vertexData = new GLfloat[vertices * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i <= yDivisions; i++) {
            float y = (float)i / yDivisions;
            
            for (int j = 0; j <= xDivisions; j++) {
                *(vertex++) = (float)j / xDivisions;
                *(vertex++) = y;
                *(vertex++) = 0.0f;
            }
        }
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBufferData(GL_ARRAY_BUFFER, vertices * NUM_BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < yDivisions; i++) {
            GLushort bottom = i * xVertices;
            GLushort top = bottom + xVertices;
            for (int j = 0; j < xDivisions; j++) {
                int next = j + 1;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * NUM_BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
        
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
 
    // all vertices have the same normal
    glNormal3f(0.0f, 0.0f, 1.0f);
    
    glVertexPointer(3, GL_FLOAT, 0, 0);
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GeometryCache::renderHalfCylinder(int slices, int stacks) {
    VerticesIndices& vbo = _halfCylinderVBOs[IntPair(slices, stacks)];
    int vertices = (slices + 1) * stacks;
    int indices = 2 * 3 * slices * (stacks - 1);
    if (vbo.first == 0) {    
        GLfloat* vertexData = new GLfloat[vertices * 2 * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i <= (stacks - 1); i++) {
            float y = (float)i / (stacks - 1);
            
            for (int j = 0; j <= slices; j++) {
                float theta = 3.f * PI_OVER_TWO + PI * (float)j / (float)slices;

                //normals
                *(vertex++) = sinf(theta);
                *(vertex++) = 0;
                *(vertex++) = cosf(theta);

                // vertices
                *(vertex++) = sinf(theta);
                *(vertex++) = y;
                *(vertex++) = cosf(theta);
            }
        }
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBufferData(GL_ARRAY_BUFFER, 2 * vertices * NUM_BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < stacks - 1; i++) {
            GLushort bottom = i * (slices + 1);
            GLushort top = bottom + slices + 1;
            for (int j = 0; j < slices; j++) {
                int next = j + 1;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * NUM_BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
    
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glNormalPointer(GL_FLOAT, 6 * sizeof(float), 0);
    glVertexPointer(3, GL_FLOAT, (6 * sizeof(float)), (const void *)(3 * sizeof(float)));
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GeometryCache::renderCone(float base, float height, int slices, int stacks) {
    VerticesIndices& vbo = _halfCylinderVBOs[IntPair(slices, stacks)];
    int vertices = (stacks + 2) * slices;
    int baseTriangles = slices - 2;
    int indices = NUM_VERTICES_PER_TRIANGULATED_QUAD * slices * stacks + NUM_VERTICES_PER_TRIANGLE * baseTriangles;
    if (vbo.first == 0) {
        GLfloat* vertexData = new GLfloat[vertices * NUM_COORDS_PER_VERTEX * 2];
        GLfloat* vertex = vertexData;
        // cap
        for (int i = 0; i < slices; i++) {
            float theta = TWO_PI * i / slices;
            
            //normals
            *(vertex++) = 0.0f;
            *(vertex++) = 0.0f;
            *(vertex++) = -1.0f;

            // vertices
            *(vertex++) = cosf(theta);
            *(vertex++) = sinf(theta);
            *(vertex++) = 0.0f;
        }
        // body
        for (int i = 0; i <= stacks; i++) {
            float z = (float)i / stacks;
            float radius = 1.0f - z;
            
            for (int j = 0; j < slices; j++) {
                float theta = TWO_PI * j / slices;

                //normals
                *(vertex++) = cosf(theta) / SQUARE_ROOT_OF_2;
                *(vertex++) = sinf(theta) / SQUARE_ROOT_OF_2;
                *(vertex++) = 1.0f / SQUARE_ROOT_OF_2;

                // vertices
                *(vertex++) = radius * cosf(theta);
                *(vertex++) = radius * sinf(theta);
                *(vertex++) = z;
            }
        }
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBufferData(GL_ARRAY_BUFFER, 2 * vertices * NUM_BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < baseTriangles; i++) {
            *(index++) = 0;
            *(index++) = i + 2;
            *(index++) = i + 1;
        }
        for (int i = 1; i <= stacks; i++) {
            GLushort bottom = i * slices;
            GLushort top = bottom + slices;
            for (int j = 0; j < slices; j++) {
                int next = (j + 1) % slices;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * NUM_BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
        
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    int stride = NUM_VERTICES_PER_TRIANGULATED_QUAD * sizeof(float);
    glNormalPointer(GL_FLOAT, stride, 0);
    glVertexPointer(NUM_COORDS_PER_VERTEX, GL_FLOAT, stride, (const void *)(NUM_COORDS_PER_VERTEX * sizeof(float)));
    
    glPushMatrix();
    glScalef(base, base, height);
    
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
    
    glPopMatrix();
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GeometryCache::renderGrid(int xDivisions, int yDivisions) {
    QOpenGLBuffer& buffer = _gridBuffers[IntPair(xDivisions, yDivisions)];
    int vertices = (xDivisions + 1 + yDivisions + 1) * 2;
    if (!buffer.isCreated()) {
        GLfloat* vertexData = new GLfloat[vertices * 2];
        GLfloat* vertex = vertexData;
        for (int i = 0; i <= xDivisions; i++) {
            float x = (float)i / xDivisions;
        
            *(vertex++) = x;
            *(vertex++) = 0.0f;
            
            *(vertex++) = x;
            *(vertex++) = 1.0f;
        }
        for (int i = 0; i <= yDivisions; i++) {
            float y = (float)i / yDivisions;
            
            *(vertex++) = 0.0f;
            *(vertex++) = y;
            
            *(vertex++) = 1.0f;
            *(vertex++) = y;
        }
        buffer.create();
        buffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        buffer.bind();
        buffer.allocate(vertexData, vertices * 2 * sizeof(GLfloat));
        delete[] vertexData;
        
    } else {
        buffer.bind();
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glVertexPointer(2, GL_FLOAT, 0, 0);
    
    glDrawArrays(GL_LINES, 0, vertices);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    
    buffer.release();
}

QSharedPointer<NetworkGeometry> GeometryCache::getGeometry(const QUrl& url, const QUrl& fallback, bool delayLoad) {
    return getResource(url, fallback, delayLoad).staticCast<NetworkGeometry>();
}

void GeometryCache::noteRequiresBlend(Model* model) {
    if (_pendingBlenders < QThread::idealThreadCount()) {
        if (model->maybeStartBlender()) {
            _pendingBlenders++;
        }
        return;
    }
    if (!_modelsRequiringBlends.contains(model)) {
        _modelsRequiringBlends.append(model);
    }
}

void GeometryCache::setBlendedVertices(const QPointer<Model>& model, int blendNumber,
        const QWeakPointer<NetworkGeometry>& geometry, const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals) {
    if (!model.isNull()) {
        model->setBlendedVertices(blendNumber, geometry, vertices, normals);
    }
    _pendingBlenders--;
    while (!_modelsRequiringBlends.isEmpty()) {
        Model* nextModel = _modelsRequiringBlends.takeFirst();
        if (nextModel && nextModel->maybeStartBlender()) {
            _pendingBlenders++;
            return;
        }
    }
}

QSharedPointer<Resource> GeometryCache::createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra) {
    QSharedPointer<NetworkGeometry> geometry(new NetworkGeometry(url, fallback.staticCast<NetworkGeometry>(), delayLoad),
        &Resource::allReferencesCleared);
    geometry->setLODParent(geometry);
    return geometry.staticCast<Resource>();
}

const float NetworkGeometry::NO_HYSTERESIS = -1.0f;

NetworkGeometry::NetworkGeometry(const QUrl& url, const QSharedPointer<NetworkGeometry>& fallback, bool delayLoad,
        const QVariantHash& mapping, const QUrl& textureBase) :
    Resource(url, delayLoad),
    _mapping(mapping),
    _textureBase(textureBase.isValid() ? textureBase : url),
    _fallback(fallback) {
    
    if (url.isEmpty()) {
        // make the minimal amount of dummy geometry to satisfy Model
        FBXJoint joint = { false, QVector<int>(), -1 };
        _geometry.joints.append(joint);
        _geometry.leftEyeJointIndex = -1;
        _geometry.rightEyeJointIndex = -1;
        _geometry.neckJointIndex = -1;
        _geometry.rootJointIndex = -1;
        _geometry.leanJointIndex = -1;
        _geometry.headJointIndex = -1;
        _geometry.leftHandJointIndex = -1;
        _geometry.rightHandJointIndex = -1;
    }
}

bool NetworkGeometry::isLoadedWithTextures() const {
    if (!isLoaded()) {
        return false;
    }
    foreach (const NetworkMesh& mesh, _meshes) {
        foreach (const NetworkMeshPart& part, mesh.parts) {
            if ((part.diffuseTexture && !part.diffuseTexture->isLoaded()) ||
                    (part.normalTexture && !part.normalTexture->isLoaded()) ||
                    (part.specularTexture && !part.specularTexture->isLoaded())) {
                return false;
            }
        }
    }
    return true;
}

QSharedPointer<NetworkGeometry> NetworkGeometry::getLODOrFallback(float distance, float& hysteresis, bool delayLoad) const {
    if (_lodParent.data() != this) {
        return _lodParent.data()->getLODOrFallback(distance, hysteresis, delayLoad);
    }
    if (_failedToLoad && _fallback) {
        return _fallback;
    }
    QSharedPointer<NetworkGeometry> lod = _lodParent;
    float lodDistance = 0.0f;
    QMap<float, QSharedPointer<NetworkGeometry> >::const_iterator it = _lods.upperBound(distance);
    if (it != _lods.constBegin()) {
        it = it - 1;
        lod = it.value();
        lodDistance = it.key();
    }
    if (hysteresis != NO_HYSTERESIS && hysteresis != lodDistance) {
        // if we previously selected a different distance, make sure we've moved far enough to justify switching
        const float HYSTERESIS_PROPORTION = 0.1f;
        if (glm::abs(distance - qMax(hysteresis, lodDistance)) / fabsf(hysteresis - lodDistance) < HYSTERESIS_PROPORTION) {
            lod = _lodParent;
            lodDistance = 0.0f;
            it = _lods.upperBound(hysteresis);
            if (it != _lods.constBegin()) {
                it = it - 1;
                lod = it.value();
                lodDistance = it.key();
            }
        }
    }
    if (lod->isLoaded()) {
        hysteresis = lodDistance;
        return lod;
    }
    // if the ideal LOD isn't loaded, we need to make sure it's started to load, and possibly return the closest loaded one
    if (!delayLoad) {
        lod->ensureLoading();
    }
    float closestDistance = FLT_MAX;
    if (isLoaded()) {
        lod = _lodParent;
        closestDistance = distance;
    }
    for (it = _lods.constBegin(); it != _lods.constEnd(); it++) {
        float distanceToLOD = glm::abs(distance - it.key());
        if (it.value()->isLoaded() && distanceToLOD < closestDistance) {
            lod = it.value();
            closestDistance = distanceToLOD;
        }    
    }
    hysteresis = NO_HYSTERESIS;
    return lod;
}

uint qHash(const QWeakPointer<Animation>& animation, uint seed = 0) {
    return qHash(animation.data(), seed);
}

QVector<int> NetworkGeometry::getJointMappings(const AnimationPointer& animation) {
    QVector<int> mappings = _jointMappings.value(animation);
    if (mappings.isEmpty() && isLoaded() && animation && animation->isLoaded()) {
        const FBXGeometry& animationGeometry = animation->getGeometry();
        for (int i = 0; i < animationGeometry.joints.size(); i++) { 
            mappings.append(_geometry.jointIndices.value(animationGeometry.joints.at(i).name) - 1);
        }
        _jointMappings.insert(animation, mappings);
    }
    return mappings;
}

void NetworkGeometry::setLoadPriority(const QPointer<QObject>& owner, float priority) {
    Resource::setLoadPriority(owner, priority);
    
    for (int i = 0; i < _meshes.size(); i++) {
        NetworkMesh& mesh = _meshes[i];
        for (int j = 0; j < mesh.parts.size(); j++) {
            NetworkMeshPart& part = mesh.parts[j];
            if (part.diffuseTexture) {
                part.diffuseTexture->setLoadPriority(owner, priority);
            }
            if (part.normalTexture) {
                part.normalTexture->setLoadPriority(owner, priority);
            }
            if (part.specularTexture) {
                part.specularTexture->setLoadPriority(owner, priority);
            }
        }
    }
}

void NetworkGeometry::setLoadPriorities(const QHash<QPointer<QObject>, float>& priorities) {
    Resource::setLoadPriorities(priorities);
    
    for (int i = 0; i < _meshes.size(); i++) {
        NetworkMesh& mesh = _meshes[i];
        for (int j = 0; j < mesh.parts.size(); j++) {
            NetworkMeshPart& part = mesh.parts[j];
            if (part.diffuseTexture) {
                part.diffuseTexture->setLoadPriorities(priorities);
            }
            if (part.normalTexture) {
                part.normalTexture->setLoadPriorities(priorities);
            }
            if (part.specularTexture) {
                part.specularTexture->setLoadPriorities(priorities);
            }
        }
    }
}

void NetworkGeometry::clearLoadPriority(const QPointer<QObject>& owner) {
    Resource::clearLoadPriority(owner);
    
    for (int i = 0; i < _meshes.size(); i++) {
        NetworkMesh& mesh = _meshes[i];
        for (int j = 0; j < mesh.parts.size(); j++) {
            NetworkMeshPart& part = mesh.parts[j];
            if (part.diffuseTexture) {
                part.diffuseTexture->clearLoadPriority(owner);
            }
            if (part.normalTexture) {
                part.normalTexture->clearLoadPriority(owner);
            }
            if (part.specularTexture) {
                part.specularTexture->clearLoadPriority(owner);
            }
        }
    }
}

void NetworkGeometry::setTextureWithNameToURL(const QString& name, const QUrl& url) {
    for (int i = 0; i < _meshes.size(); i++) {
        NetworkMesh& mesh = _meshes[i];
        for (int j = 0; j < mesh.parts.size(); j++) {
            NetworkMeshPart& part = mesh.parts[j];
            
            QSharedPointer<NetworkTexture> matchingTexture = QSharedPointer<NetworkTexture>();
            if (part.diffuseTextureName == name) {
                part.diffuseTexture =
                    Application::getInstance()->getTextureCache()->getTexture(url, DEFAULT_TEXTURE,
                                                                              _geometry.meshes[i].isEye, QByteArray());
                part.diffuseTexture->setLoadPriorities(_loadPriorities);
            } else if (part.normalTextureName == name) {
                part.normalTexture = Application::getInstance()->getTextureCache()->getTexture(url, DEFAULT_TEXTURE,
                                                                                               false, QByteArray());
                part.normalTexture->setLoadPriorities(_loadPriorities);
            } else if (part.specularTextureName == name) {
                part.specularTexture = Application::getInstance()->getTextureCache()->getTexture(url, DEFAULT_TEXTURE,
                                                                                                 false, QByteArray());
                part.specularTexture->setLoadPriorities(_loadPriorities);
            }
        }
    }
}

/// Reads geometry in a worker thread.
class GeometryReader : public QRunnable {
public:

    GeometryReader(const QWeakPointer<Resource>& geometry, const QUrl& url,
        QNetworkReply* reply, const QVariantHash& mapping);

    virtual void run();

private:
     
    QWeakPointer<Resource> _geometry;
    QUrl _url;
    QNetworkReply* _reply;
    QVariantHash _mapping;
};

GeometryReader::GeometryReader(const QWeakPointer<Resource>& geometry, const QUrl& url,
        QNetworkReply* reply, const QVariantHash& mapping) :
    _geometry(geometry),
    _url(url),
    _reply(reply),
    _mapping(mapping) {
}

void GeometryReader::run() {
    QSharedPointer<Resource> geometry = _geometry.toStrongRef();
    if (geometry.isNull()) {
        _reply->deleteLater();
        return;
    }
    try {
        QMetaObject::invokeMethod(geometry.data(), "setGeometry", Q_ARG(const FBXGeometry&,
            _url.path().toLower().endsWith(".svo") ? readSVO(_reply->readAll()) : readFBX(_reply->readAll(), _mapping)));
        
    } catch (const QString& error) {
        qDebug() << "Error reading " << _url << ": " << error;
        QMetaObject::invokeMethod(geometry.data(), "finishedLoading", Q_ARG(bool, false));
    }
    _reply->deleteLater();
}

void NetworkGeometry::init() {
    _mapping = QVariantHash();
    _geometry = FBXGeometry();
    _meshes.clear();
    _lods.clear();
    _request.setUrl(_url);
    Resource::init();
}

void NetworkGeometry::downloadFinished(QNetworkReply* reply) {
    QUrl url = reply->url();
    if (url.path().toLower().endsWith(".fst")) {
        // it's a mapping file; parse it and get the mesh filename
        _mapping = readMapping(reply->readAll());
        reply->deleteLater();
        QString filename = _mapping.value("filename").toString();
        if (filename.isNull()) {
            qDebug() << "Mapping file " << url << " has no filename.";
            finishedLoading(false);
            
        } else {
            QString texdir = _mapping.value("texdir").toString();
            if (!texdir.isNull()) {
                if (!texdir.endsWith('/')) {
                    texdir += '/';
                }
                _textureBase = url.resolved(texdir);
            }
            QVariantHash lods = _mapping.value("lod").toHash();
            for (QVariantHash::const_iterator it = lods.begin(); it != lods.end(); it++) {
                QSharedPointer<NetworkGeometry> geometry(new NetworkGeometry(url.resolved(it.key()),
                    QSharedPointer<NetworkGeometry>(), true, _mapping, _textureBase));    
                geometry->setSelf(geometry.staticCast<Resource>());
                geometry->setLODParent(_lodParent);
                _lods.insert(it.value().toFloat(), geometry);
            }     
            _request.setUrl(url.resolved(filename));
            
            // make the request immediately only if we have no LODs to switch between
            _startedLoading = false;
            if (_lods.isEmpty()) {
                attemptRequest();
            }
        }
        return;
    }
    
    // send the reader off to the thread pool
    QThreadPool::globalInstance()->start(new GeometryReader(_self, url, reply, _mapping));
}

void NetworkGeometry::reinsert() {
    Resource::reinsert();
    
    _lodParent = qWeakPointerCast<NetworkGeometry, Resource>(_self);
    foreach (const QSharedPointer<NetworkGeometry>& lod, _lods) {
        lod->setLODParent(_lodParent);
    }
}

void NetworkGeometry::setGeometry(const FBXGeometry& geometry) {
    _geometry = geometry;
    
    foreach (const FBXMesh& mesh, _geometry.meshes) {
        NetworkMesh networkMesh = { QOpenGLBuffer(QOpenGLBuffer::IndexBuffer), QOpenGLBuffer(QOpenGLBuffer::VertexBuffer) };
        
        int totalIndices = 0;
        foreach (const FBXMeshPart& part, mesh.parts) {
            NetworkMeshPart networkPart;
            if (!part.diffuseTexture.filename.isEmpty()) {
                networkPart.diffuseTexture = Application::getInstance()->getTextureCache()->getTexture(
                    _textureBase.resolved(QUrl(part.diffuseTexture.filename)), DEFAULT_TEXTURE,
                    mesh.isEye, part.diffuseTexture.content);
                networkPart.diffuseTextureName = part.diffuseTexture.name;
                networkPart.diffuseTexture->setLoadPriorities(_loadPriorities);
            }
            if (!part.normalTexture.filename.isEmpty()) {
                networkPart.normalTexture = Application::getInstance()->getTextureCache()->getTexture(
                    _textureBase.resolved(QUrl(part.normalTexture.filename)), NORMAL_TEXTURE,
                    false, part.normalTexture.content);
                networkPart.normalTextureName = part.normalTexture.name;
                networkPart.normalTexture->setLoadPriorities(_loadPriorities);
            }
            if (!part.specularTexture.filename.isEmpty()) {
                networkPart.specularTexture = Application::getInstance()->getTextureCache()->getTexture(
                    _textureBase.resolved(QUrl(part.specularTexture.filename)), SPECULAR_TEXTURE,
                    false, part.specularTexture.content);
                networkPart.specularTextureName = part.specularTexture.name;
                networkPart.specularTexture->setLoadPriorities(_loadPriorities);
            }
            networkMesh.parts.append(networkPart);
                        
            totalIndices += (part.quadIndices.size() + part.triangleIndices.size());
        }
        
        networkMesh.indexBuffer.create();
        networkMesh.indexBuffer.bind();
        networkMesh.indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        networkMesh.indexBuffer.allocate(totalIndices * sizeof(int));
        int offset = 0;
        foreach (const FBXMeshPart& part, mesh.parts) {
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, part.quadIndices.size() * sizeof(int),
                part.quadIndices.constData());
            offset += part.quadIndices.size() * sizeof(int);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, part.triangleIndices.size() * sizeof(int),
                part.triangleIndices.constData());
            offset += part.triangleIndices.size() * sizeof(int);
        }
        networkMesh.indexBuffer.release();
        
        networkMesh.vertexBuffer.create();
        networkMesh.vertexBuffer.bind();
        networkMesh.vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        
        // if we don't need to do any blending, the positions/normals can be static
        if (mesh.blendshapes.isEmpty()) {
            int normalsOffset = mesh.vertices.size() * sizeof(glm::vec3);
            int tangentsOffset = normalsOffset + mesh.normals.size() * sizeof(glm::vec3);
            int colorsOffset = tangentsOffset + mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int clusterIndicesOffset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);
            
            networkMesh.vertexBuffer.allocate(clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4));
            networkMesh.vertexBuffer.write(0, mesh.vertices.constData(), mesh.vertices.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(normalsOffset, mesh.normals.constData(), mesh.normals.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(tangentsOffset, mesh.tangents.constData(),
                mesh.tangents.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(colorsOffset, mesh.colors.constData(), mesh.colors.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(texCoordsOffset, mesh.texCoords.constData(),
                mesh.texCoords.size() * sizeof(glm::vec2));
            networkMesh.vertexBuffer.write(clusterIndicesOffset, mesh.clusterIndices.constData(),
                mesh.clusterIndices.size() * sizeof(glm::vec4));
            networkMesh.vertexBuffer.write(clusterWeightsOffset, mesh.clusterWeights.constData(),
                mesh.clusterWeights.size() * sizeof(glm::vec4));
        
        // otherwise, at least the cluster indices/weights can be static
        } else {
            int colorsOffset = mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int clusterIndicesOffset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);
            networkMesh.vertexBuffer.allocate(clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4));
            networkMesh.vertexBuffer.write(0, mesh.tangents.constData(), mesh.tangents.size() * sizeof(glm::vec3));        
            networkMesh.vertexBuffer.write(colorsOffset, mesh.colors.constData(), mesh.colors.size() * sizeof(glm::vec3));    
            networkMesh.vertexBuffer.write(texCoordsOffset, mesh.texCoords.constData(),
                mesh.texCoords.size() * sizeof(glm::vec2));
            networkMesh.vertexBuffer.write(clusterIndicesOffset, mesh.clusterIndices.constData(),
                mesh.clusterIndices.size() * sizeof(glm::vec4));
            networkMesh.vertexBuffer.write(clusterWeightsOffset, mesh.clusterWeights.constData(),
                mesh.clusterWeights.size() * sizeof(glm::vec4));   
        }
        
        networkMesh.vertexBuffer.release();
        
        _meshes.append(networkMesh);
    }
    
    finishedLoading(true);
}

bool NetworkMeshPart::isTranslucent() const {
    return diffuseTexture && diffuseTexture->isTranslucent();
}

int NetworkMesh::getTranslucentPartCount(const FBXMesh& fbxMesh) const {
    int count = 0;
    for (int i = 0; i < parts.size(); i++) {
        if (parts.at(i).isTranslucent() || fbxMesh.parts.at(i).opacity != 1.0f) {
            count++;
        }
    }
    return count;
}
