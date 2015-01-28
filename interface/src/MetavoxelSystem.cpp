//
//  MetavoxelSystem.cpp
//  interface/src
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <limits>

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QMutexLocker>
#include <QOpenGLFramebufferObject>
#include <QReadLocker>
#include <QWriteLocker>
#include <QThreadPool>
#include <QtDebug>

#include <glm/gtx/transform.hpp>

#include <DeferredLightingEffect.h>
#include <GeometryUtil.h>
#include <Model.h>
#include <SharedUtil.h>

#include <MetavoxelMessages.h>
#include <MetavoxelUtil.h>
#include <PathUtils.h>
#include <ScriptCache.h>

#include "Application.h"
#include "MetavoxelSystem.h"

using namespace std;

REGISTER_META_OBJECT(DefaultMetavoxelRendererImplementation)
REGISTER_META_OBJECT(SphereRenderer)
REGISTER_META_OBJECT(CuboidRenderer)
REGISTER_META_OBJECT(StaticModelRenderer)
REGISTER_META_OBJECT(HeightfieldRenderer)

MetavoxelSystem::NetworkSimulation::NetworkSimulation(float dropRate, float repeatRate,
        int minimumDelay, int maximumDelay, int bandwidthLimit) :
    dropRate(dropRate),
    repeatRate(repeatRate),
    minimumDelay(minimumDelay),
    maximumDelay(maximumDelay),
    bandwidthLimit(bandwidthLimit) {
}    

MetavoxelSystem::~MetavoxelSystem() {
    // kill the updater before we delete our network simulation objects
    _updater->thread()->quit();
    _updater->thread()->wait();
    _updater = NULL;
}

void MetavoxelSystem::init() {
    MetavoxelClientManager::init();
    
    _baseHeightfieldProgram.addShaderFromSourceFile(QGLShader::Vertex, PathUtils::resourcesPath() +
            "shaders/metavoxel_heightfield_base.vert");
    _baseHeightfieldProgram.addShaderFromSourceFile(QGLShader::Fragment, PathUtils::resourcesPath() +
        "shaders/metavoxel_heightfield_base.frag");
    _baseHeightfieldProgram.link();
    
    _baseHeightfieldProgram.bind();
    _baseHeightfieldProgram.setUniformValue("heightMap", 0);
    _baseHeightfieldProgram.setUniformValue("diffuseMap", 1);
    _baseHeightScaleLocation = _baseHeightfieldProgram.uniformLocation("heightScale");
    _baseColorScaleLocation = _baseHeightfieldProgram.uniformLocation("colorScale");
    _baseHeightfieldProgram.release();
    
    loadSplatProgram("heightfield", _splatHeightfieldProgram, _splatHeightfieldLocations);
    
    _heightfieldCursorProgram.addShaderFromSourceFile(QGLShader::Vertex, PathUtils::resourcesPath() +
        "shaders/metavoxel_heightfield_cursor.vert");
    _heightfieldCursorProgram.addShaderFromSourceFile(QGLShader::Fragment, PathUtils::resourcesPath() +
        "shaders/metavoxel_cursor.frag");
    _heightfieldCursorProgram.link();
    
    _heightfieldCursorProgram.bind();
    _heightfieldCursorProgram.setUniformValue("heightMap", 0);
    _heightfieldCursorProgram.release();
    
    _baseVoxelProgram.addShaderFromSourceFile(QGLShader::Vertex, PathUtils::resourcesPath() +
        "shaders/metavoxel_voxel_base.vert");
    _baseVoxelProgram.addShaderFromSourceFile(QGLShader::Fragment, PathUtils::resourcesPath() +
        "shaders/metavoxel_voxel_base.frag");
    _baseVoxelProgram.link();
    
    loadSplatProgram("voxel", _splatVoxelProgram, _splatVoxelLocations);
    
    _voxelCursorProgram.addShaderFromSourceFile(QGLShader::Vertex, PathUtils::resourcesPath() +
        "shaders/metavoxel_voxel_cursor.vert");
    _voxelCursorProgram.addShaderFromSourceFile(QGLShader::Fragment, PathUtils::resourcesPath() +
        "shaders/metavoxel_cursor.frag");
    _voxelCursorProgram.link();
}

MetavoxelLOD MetavoxelSystem::getLOD() {
    QReadLocker locker(&_lodLock);
    return _lod;
}

void MetavoxelSystem::setNetworkSimulation(const NetworkSimulation& simulation) {
    QWriteLocker locker(&_networkSimulationLock);
    _networkSimulation = simulation;
}

MetavoxelSystem::NetworkSimulation MetavoxelSystem::getNetworkSimulation() {
    QReadLocker locker(&_networkSimulationLock);
    return _networkSimulation;
}

class SimulateVisitor : public MetavoxelVisitor {
public:
    
    SimulateVisitor(float deltaTime, const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    float _deltaTime;
};

SimulateVisitor::SimulateVisitor(float deltaTime, const MetavoxelLOD& lod) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getRendererAttribute(),
        QVector<AttributePointer>(), lod),
    _deltaTime(deltaTime) {
}

int SimulateVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    static_cast<MetavoxelRenderer*>(info.inputValues.at(0).getInlineValue<
        SharedObjectPointer>().data())->getImplementation()->simulate(*_data, _deltaTime, info, _lod);
    return STOP_RECURSION;
}

void MetavoxelSystem::simulate(float deltaTime) {
    // update the lod
    {
        QWriteLocker locker(&_lodLock);
        const float DEFAULT_LOD_THRESHOLD = 0.01f;
        _lod = MetavoxelLOD(Application::getInstance()->getCamera()->getPosition(), DEFAULT_LOD_THRESHOLD);
    }

    SimulateVisitor simulateVisitor(deltaTime, getLOD());
    guideToAugmented(simulateVisitor);
}

class RenderVisitor : public MetavoxelVisitor {
public:
    
    RenderVisitor(const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);
};

RenderVisitor::RenderVisitor(const MetavoxelLOD& lod) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getRendererAttribute(),
        QVector<AttributePointer>(), lod) {
}

int RenderVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    static_cast<MetavoxelRenderer*>(info.inputValues.at(0).getInlineValue<
        SharedObjectPointer>().data())->getImplementation()->render(*_data, info, _lod);
    return STOP_RECURSION;
}

class HeightfieldPoint {
public:
    glm::vec3 vertex;
    glm::vec2 textureCoord;
};

const int SPLAT_COUNT = 4;
const GLint SPLAT_TEXTURE_UNITS[] = { 3, 4, 5, 6 };

static const int EIGHT_BIT_MAXIMUM = 255;
static const float EIGHT_BIT_MAXIMUM_RECIPROCAL = 1.0f / EIGHT_BIT_MAXIMUM;

void MetavoxelSystem::render() {
    // update the frustum
    ViewFrustum* viewFrustum = Application::getInstance()->getDisplayViewFrustum();
    _frustum.set(viewFrustum->getFarTopLeft(), viewFrustum->getFarTopRight(), viewFrustum->getFarBottomLeft(),
        viewFrustum->getFarBottomRight(), viewFrustum->getNearTopLeft(), viewFrustum->getNearTopRight(),
        viewFrustum->getNearBottomLeft(), viewFrustum->getNearBottomRight());
   
    RenderVisitor renderVisitor(getLOD());
    guideToAugmented(renderVisitor, true);
    
    if (!_heightfieldBaseBatches.isEmpty() && Menu::getInstance()->isOptionChecked(MenuOption::RenderHeightfields)) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
        DependencyManager::get<TextureCache>()->setPrimaryDrawBuffers(true, true);
    
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_EQUAL, 0.0f);
        
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        
        _baseHeightfieldProgram.bind();
        
        foreach (const HeightfieldBaseLayerBatch& batch, _heightfieldBaseBatches) {
            glPushMatrix();
            glTranslatef(batch.translation.x, batch.translation.y, batch.translation.z);
            glm::vec3 axis = glm::axis(batch.rotation);
            glRotatef(glm::degrees(glm::angle(batch.rotation)), axis.x, axis.y, axis.z);
            glScalef(batch.scale.x, batch.scale.y, batch.scale.z);
            
            glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBufferID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBufferID);
        
            HeightfieldPoint* point = 0;
            glVertexPointer(3, GL_FLOAT, sizeof(HeightfieldPoint), &point->vertex);
            glTexCoordPointer(2, GL_FLOAT, sizeof(HeightfieldPoint), &point->textureCoord);
            
            glBindTexture(GL_TEXTURE_2D, batch.heightTextureID);
            
            _baseHeightfieldProgram.setUniform(_baseHeightScaleLocation, batch.heightScale);
            _baseHeightfieldProgram.setUniform(_baseColorScaleLocation, batch.colorScale);
                
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, batch.colorTextureID);
            
            glDrawRangeElements(GL_TRIANGLES, 0, batch.vertexCount - 1, batch.indexCount, GL_UNSIGNED_INT, 0);
            
            glBindTexture(GL_TEXTURE_2D, 0);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
        
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
            glPopMatrix();
        }
        
        DependencyManager::get<TextureCache>()->setPrimaryDrawBuffers(true, false);
        
        _baseHeightfieldProgram.release();
        
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        
        if (!_heightfieldSplatBatches.isEmpty()) {
            glDepthFunc(GL_LEQUAL);
            glDepthMask(false);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);
            
            _splatHeightfieldProgram.bind();
            
            foreach (const HeightfieldSplatBatch& batch, _heightfieldSplatBatches) {
                glPushMatrix();
                glTranslatef(batch.translation.x, batch.translation.y, batch.translation.z);
                glm::vec3 axis = glm::axis(batch.rotation);
                glRotatef(glm::degrees(glm::angle(batch.rotation)), axis.x, axis.y, axis.z);
                glScalef(batch.scale.x, batch.scale.y, batch.scale.z);
                
                glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBufferID);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBufferID);
            
                HeightfieldPoint* point = 0;
                glVertexPointer(3, GL_FLOAT, sizeof(HeightfieldPoint), &point->vertex);
                glTexCoordPointer(2, GL_FLOAT, sizeof(HeightfieldPoint), &point->textureCoord);
                
                glBindTexture(GL_TEXTURE_2D, batch.heightTextureID);
                
                _splatHeightfieldProgram.setUniformValue(_splatHeightfieldLocations.heightScale,
                    batch.heightScale.x, batch.heightScale.y);
                _splatHeightfieldProgram.setUniform(_splatHeightfieldLocations.textureScale, batch.textureScale);
                _splatHeightfieldProgram.setUniform(_splatHeightfieldLocations.splatTextureOffset, batch.splatTextureOffset);
                
                const float QUARTER_STEP = 0.25f * EIGHT_BIT_MAXIMUM_RECIPROCAL;
                _splatHeightfieldProgram.setUniform(_splatHeightfieldLocations.splatTextureScalesS, batch.splatTextureScalesS);
                _splatHeightfieldProgram.setUniform(_splatHeightfieldLocations.splatTextureScalesT, batch.splatTextureScalesT);
                _splatHeightfieldProgram.setUniformValue(
                    _splatHeightfieldLocations.textureValueMinima,
                    (batch.materialIndex + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                    (batch.materialIndex + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                    (batch.materialIndex + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                    (batch.materialIndex + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP);
                _splatHeightfieldProgram.setUniformValue(
                    _splatHeightfieldLocations.textureValueMaxima,
                    (batch.materialIndex + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                    (batch.materialIndex + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                    (batch.materialIndex + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                    (batch.materialIndex + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP);
                    
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, batch.materialTextureID);
                
                for (int i = 0; i < SPLAT_COUNT; i++) {
                    glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[i]);
                    glBindTexture(GL_TEXTURE_2D, batch.splatTextureIDs[i]);
                }
                
                glDrawRangeElements(GL_TRIANGLES, 0, batch.vertexCount - 1, batch.indexCount, GL_UNSIGNED_INT, 0);
             
                for (int i = 0; i < SPLAT_COUNT; i++) {
                    glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[i]);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, 0);
            
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, 0);
            
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                
                glPopMatrix();   
            }
            
            _splatHeightfieldProgram.release();
            
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDepthMask(true);
            glDepthFunc(GL_LESS);
        }
        
        glDisable(GL_CULL_FACE);
        
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);    
        glDisableClientState(GL_VERTEX_ARRAY);  
    }
    _heightfieldBaseBatches.clear();
    _heightfieldSplatBatches.clear();
    
    if (!_voxelBaseBatches.isEmpty() && Menu::getInstance()->isOptionChecked(MenuOption::RenderDualContourSurfaces)) {
        DependencyManager::get<TextureCache>()->setPrimaryDrawBuffers(true, true);
    
        glEnableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_EQUAL, 0.0f);
        
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
            
        _baseVoxelProgram.bind();
        
        foreach (const MetavoxelBatch& batch, _voxelBaseBatches) {
            glPushMatrix();
            glTranslatef(batch.translation.x, batch.translation.y, batch.translation.z);
            glm::vec3 axis = glm::axis(batch.rotation);
            glRotatef(glm::degrees(glm::angle(batch.rotation)), axis.x, axis.y, axis.z);
            glScalef(batch.scale.x, batch.scale.y, batch.scale.z);
                
            glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBufferID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBufferID);
            
            VoxelPoint* point = 0;
            glVertexPointer(3, GL_FLOAT, sizeof(VoxelPoint), &point->vertex);
            glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(VoxelPoint), &point->color);
            glNormalPointer(GL_BYTE, sizeof(VoxelPoint), &point->normal);
            
            glDrawRangeElements(GL_QUADS, 0, batch.vertexCount - 1, batch.indexCount, GL_UNSIGNED_INT, 0);
            
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            
            glPopMatrix();
        }
        
        _baseVoxelProgram.release();
    
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
            
        DependencyManager::get<TextureCache>()->setPrimaryDrawBuffers(true, false);
        
        if (!_voxelSplatBatches.isEmpty()) {
            glDepthFunc(GL_LEQUAL);
            glDepthMask(false);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);
            
            _splatVoxelProgram.bind();
            
            _splatVoxelProgram.enableAttributeArray(_splatVoxelLocations.materials);
            _splatVoxelProgram.enableAttributeArray(_splatVoxelLocations.materialWeights);
            
            foreach (const VoxelSplatBatch& batch, _voxelSplatBatches) {
                glPushMatrix();
                glTranslatef(batch.translation.x, batch.translation.y, batch.translation.z);
                glm::vec3 axis = glm::axis(batch.rotation);
                glRotatef(glm::degrees(glm::angle(batch.rotation)), axis.x, axis.y, axis.z);
                glScalef(batch.scale.x, batch.scale.y, batch.scale.z);
            
                glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBufferID);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBufferID);
                
                VoxelPoint* point = 0;
                glVertexPointer(3, GL_FLOAT, sizeof(VoxelPoint), &point->vertex);
                glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(VoxelPoint), &point->color);
                glNormalPointer(GL_BYTE, sizeof(VoxelPoint), &point->normal);
                
                _splatVoxelProgram.setAttributeBuffer(_splatVoxelLocations.materials,
                    GL_UNSIGNED_BYTE, (qint64)&point->materials, SPLAT_COUNT, sizeof(VoxelPoint));
                _splatVoxelProgram.setAttributeBuffer(_splatVoxelLocations.materialWeights,
                    GL_UNSIGNED_BYTE, (qint64)&point->materialWeights, SPLAT_COUNT, sizeof(VoxelPoint));
                    
                const float QUARTER_STEP = 0.25f * EIGHT_BIT_MAXIMUM_RECIPROCAL;
                _splatVoxelProgram.setUniform(_splatVoxelLocations.splatTextureOffset, batch.splatTextureOffset);
                _splatVoxelProgram.setUniform(_splatVoxelLocations.splatTextureScalesS, batch.splatTextureScalesS);
                _splatVoxelProgram.setUniform(_splatVoxelLocations.splatTextureScalesT, batch.splatTextureScalesT);
                _splatVoxelProgram.setUniformValue(
                    _splatVoxelLocations.textureValueMinima,
                    (batch.materialIndex + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                    (batch.materialIndex + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                    (batch.materialIndex + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                    (batch.materialIndex + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP);
                _splatVoxelProgram.setUniformValue(
                    _splatVoxelLocations.textureValueMaxima,
                    (batch.materialIndex + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                    (batch.materialIndex + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                    (batch.materialIndex + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                    (batch.materialIndex + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP);
                
                for (int i = 0; i < SPLAT_COUNT; i++) {
                    glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[i]);
                    glBindTexture(GL_TEXTURE_2D, batch.splatTextureIDs[i]);
                }
                
                glDrawRangeElements(GL_QUADS, 0, batch.vertexCount - 1, batch.indexCount, GL_UNSIGNED_INT, 0);
                
                for (int i = 0; i < SPLAT_COUNT; i++) {
                    glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[i]);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            
                glActiveTexture(GL_TEXTURE0);
        
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                
                glPopMatrix();
            }
            
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDepthMask(true);
            glDepthFunc(GL_LESS);
            
            _splatVoxelProgram.disableAttributeArray(_splatVoxelLocations.materials);
            _splatVoxelProgram.disableAttributeArray(_splatVoxelLocations.materialWeights);
        }
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisable(GL_CULL_FACE);
    }
    _voxelBaseBatches.clear();
    _voxelSplatBatches.clear();
    
    if (!_hermiteBatches.isEmpty() && Menu::getInstance()->isOptionChecked(MenuOption::DisplayHermiteData)) {
        DependencyManager::get<TextureCache>()->setPrimaryDrawBuffers(true, true);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glNormal3f(0.0f, 1.0f, 0.0f);
        
        DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram();
        
        foreach (const HermiteBatch& batch, _hermiteBatches) {
            glPushMatrix();
            glTranslatef(batch.translation.x, batch.translation.y, batch.translation.z);
            glm::vec3 axis = glm::axis(batch.rotation);
            glRotatef(glm::degrees(glm::angle(batch.rotation)), axis.x, axis.y, axis.z);
            glScalef(batch.scale.x, batch.scale.y, batch.scale.z);
                
            glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBufferID);
            
            glVertexPointer(3, GL_FLOAT, 0, 0);
            
            glDrawArrays(GL_LINES, 0, batch.vertexCount);
            
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            
            glPopMatrix();
        }
        
        DependencyManager::get<DeferredLightingEffect>()->releaseSimpleProgram();
        
        glDisableClientState(GL_VERTEX_ARRAY);
        
        DependencyManager::get<TextureCache>()->setPrimaryDrawBuffers(true, false);
    }
    _hermiteBatches.clear();
    
    // give external parties a chance to join in
    emit rendering();
}

void MetavoxelSystem::paintHeightfieldColor(const glm::vec3& position, float radius, const QColor& color) {
    Sphere* sphere = new Sphere();
    sphere->setTranslation(position);
    sphere->setScale(radius);
    setHeightfieldColor(SharedObjectPointer(sphere), color, true);
}

void MetavoxelSystem::paintHeightfieldMaterial(const glm::vec3& position, float radius, const SharedObjectPointer& material) {
    Sphere* sphere = new Sphere();
    sphere->setTranslation(position);
    sphere->setScale(radius);
    setHeightfieldMaterial(SharedObjectPointer(sphere), material, true);
}

void MetavoxelSystem::setHeightfieldColor(const SharedObjectPointer& spanner, const QColor& color, bool paint) {
    MetavoxelEditMessage edit = { QVariant::fromValue(HeightfieldMaterialSpannerEdit(spanner,
        SharedObjectPointer(), color, paint)) };
    applyEdit(edit, true);
}

void MetavoxelSystem::setHeightfieldMaterial(const SharedObjectPointer& spanner,
        const SharedObjectPointer& material, bool paint) {
    MetavoxelEditMessage edit = { QVariant::fromValue(HeightfieldMaterialSpannerEdit(spanner, material, QColor(), paint)) };
    applyMaterialEdit(edit, true);
}

void MetavoxelSystem::deleteTextures(int heightTextureID, int colorTextureID, int materialTextureID) const {
    glDeleteTextures(1, (const GLuint*)&heightTextureID);
    glDeleteTextures(1, (const GLuint*)&colorTextureID);
    glDeleteTextures(1, (const GLuint*)&materialTextureID);
}

void MetavoxelSystem::deleteBuffers(int vertexBufferID, int indexBufferID, int hermiteBufferID) const {
    glDeleteBuffers(1, (const GLuint*)&vertexBufferID);
    glDeleteBuffers(1, (const GLuint*)&indexBufferID);
    glDeleteBuffers(1, (const GLuint*)&hermiteBufferID);
}

class SpannerRenderVisitor : public SpannerVisitor {
public:
    
    SpannerRenderVisitor(const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);
    virtual bool visit(Spanner* spanner);

protected:
    
    int _containmentDepth;
};

SpannerRenderVisitor::SpannerRenderVisitor(const MetavoxelLOD& lod) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>(), QVector<AttributePointer>(), lod,
        encodeOrder(Application::getInstance()->getViewFrustum()->getDirection())),
    _containmentDepth(INT_MAX) {
}

int SpannerRenderVisitor::visit(MetavoxelInfo& info) {
    if (_containmentDepth >= _depth) {
        Frustum::IntersectionType intersection = Application::getInstance()->getMetavoxels()->getFrustum().getIntersectionType(
            info.getBounds());
        if (intersection == Frustum::NO_INTERSECTION) {
            return STOP_RECURSION;
        }
        _containmentDepth = (intersection == Frustum::CONTAINS_INTERSECTION) ? _depth : INT_MAX;
    }
    return SpannerVisitor::visit(info);
}

bool SpannerRenderVisitor::visit(Spanner* spanner) {
    spanner->getRenderer()->render(_lod, _containmentDepth <= _depth);
    return true;
}

class SpannerCursorRenderVisitor : public SpannerRenderVisitor {
public:
    
    SpannerCursorRenderVisitor(const MetavoxelLOD& lod, const Box& bounds);
    
    virtual bool visit(Spanner* spanner);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    Box _bounds;
};

SpannerCursorRenderVisitor::SpannerCursorRenderVisitor(const MetavoxelLOD& lod, const Box& bounds) :
    SpannerRenderVisitor(lod),
    _bounds(bounds) {
}

bool SpannerCursorRenderVisitor::visit(Spanner* spanner) {
    if (spanner->isHeightfield()) {
         spanner->getRenderer()->render(_lod, _containmentDepth <= _depth, true);
    }
    return true;
}

int SpannerCursorRenderVisitor::visit(MetavoxelInfo& info) {
    return info.getBounds().intersects(_bounds) ? SpannerRenderVisitor::visit(info) : STOP_RECURSION;
}

void MetavoxelSystem::renderHeightfieldCursor(const glm::vec3& position, float radius) {
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    glActiveTexture(GL_TEXTURE4);
    float scale = 1.0f / radius;
    glm::vec4 sCoefficients(scale, 0.0f, 0.0f, -scale * position.x);
    glm::vec4 tCoefficients(0.0f, scale, 0.0f, -scale * position.y);
    glm::vec4 rCoefficients(0.0f, 0.0f, scale, -scale * position.z);
    glTexGenfv(GL_S, GL_EYE_PLANE, (const GLfloat*)&sCoefficients);
    glTexGenfv(GL_T, GL_EYE_PLANE, (const GLfloat*)&tCoefficients);
    glTexGenfv(GL_R, GL_EYE_PLANE, (const GLfloat*)&rCoefficients);
    glActiveTexture(GL_TEXTURE0);
    
    glm::vec3 extents(radius, radius, radius);
    SpannerCursorRenderVisitor visitor(getLOD(), Box(position - extents, position + extents));
    guide(visitor);
    
    if (!_heightfieldBaseBatches.isEmpty()) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
        _heightfieldCursorProgram.bind();
    
        foreach (const HeightfieldBaseLayerBatch& batch, _heightfieldBaseBatches) {
            glPushMatrix();
            glTranslatef(batch.translation.x, batch.translation.y, batch.translation.z);
            glm::vec3 axis = glm::axis(batch.rotation);
            glRotatef(glm::degrees(glm::angle(batch.rotation)), axis.x, axis.y, axis.z);
            glScalef(batch.scale.x, batch.scale.y, batch.scale.z);
            
            glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBufferID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBufferID);
        
            HeightfieldPoint* point = 0;
            glVertexPointer(3, GL_FLOAT, sizeof(HeightfieldPoint), &point->vertex);
            glTexCoordPointer(2, GL_FLOAT, sizeof(HeightfieldPoint), &point->textureCoord);
            
            glBindTexture(GL_TEXTURE_2D, batch.heightTextureID);
            
            glDrawRangeElements(GL_TRIANGLES, 0, batch.vertexCount - 1, batch.indexCount, GL_UNSIGNED_INT, 0);
            
            glBindTexture(GL_TEXTURE_2D, 0);
        
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
            glPopMatrix();
        }
        
        _heightfieldCursorProgram.release();
    
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    _heightfieldBaseBatches.clear();
    
    if (!_voxelBaseBatches.isEmpty()) {
        glEnableClientState(GL_VERTEX_ARRAY);
    
        _voxelCursorProgram.bind();
    
        foreach (const MetavoxelBatch& batch, _voxelBaseBatches) {
            glPushMatrix();
            glTranslatef(batch.translation.x, batch.translation.y, batch.translation.z);
            glm::vec3 axis = glm::axis(batch.rotation);
            glRotatef(glm::degrees(glm::angle(batch.rotation)), axis.x, axis.y, axis.z);
            glScalef(batch.scale.x, batch.scale.y, batch.scale.z);
                
            glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBufferID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBufferID);
            
            VoxelPoint* point = 0;
            glVertexPointer(3, GL_FLOAT, sizeof(VoxelPoint), &point->vertex);
            
            glDrawRangeElements(GL_QUADS, 0, batch.vertexCount - 1, batch.indexCount, GL_UNSIGNED_INT, 0);
            
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            
            glPopMatrix();
        }
        
        _voxelCursorProgram.release();
        
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    _voxelBaseBatches.clear();
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

class MaterialEditApplier : public SignalHandler {
public:

    MaterialEditApplier(const MetavoxelEditMessage& message, const QSharedPointer<NetworkTexture> texture);
    
    virtual void handle();

protected:
    
    MetavoxelEditMessage _message;
    QSharedPointer<NetworkTexture> _texture;
};

MaterialEditApplier::MaterialEditApplier(const MetavoxelEditMessage& message, const QSharedPointer<NetworkTexture> texture) :
    _message(message),
    _texture(texture) {
}

void MaterialEditApplier::handle() {
    static_cast<MaterialEdit*>(_message.edit.data())->averageColor = _texture->getAverageColor();
    Application::getInstance()->getMetavoxels()->applyEdit(_message, true);
    deleteLater();
}

void MetavoxelSystem::applyMaterialEdit(const MetavoxelEditMessage& message, bool reliable) {
    const MaterialEdit* edit = static_cast<const MaterialEdit*>(message.edit.constData());
    MaterialObject* material = static_cast<MaterialObject*>(edit->material.data());
    if (material && material->getDiffuse().isValid()) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "applyMaterialEdit", Q_ARG(const MetavoxelEditMessage&, message),
                Q_ARG(bool, reliable));
            return;
        }
        auto texture = DependencyManager::get<TextureCache>()->getTexture(
            material->getDiffuse(), SPLAT_TEXTURE);
        if (texture->isLoaded()) {
            MetavoxelEditMessage newMessage = message;
            static_cast<MaterialEdit*>(newMessage.edit.data())->averageColor = texture->getAverageColor();
            applyEdit(newMessage, true);    
        
        } else {
            MaterialEditApplier* applier = new MaterialEditApplier(message, texture);
            connect(texture.data(), &Resource::loaded, applier, &SignalHandler::handle);
        }
    } else {
        applyEdit(message, true);
    }
}

MetavoxelClient* MetavoxelSystem::createClient(const SharedNodePointer& node) {
    return new MetavoxelSystemClient(node, _updater);
}

void MetavoxelSystem::guideToAugmented(MetavoxelVisitor& visitor, bool render) {
    DependencyManager::get<NodeList>()->eachNode([&visitor, &render](const SharedNodePointer& node){
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelSystemClient* client = static_cast<MetavoxelSystemClient*>(node->getLinkedData());
            if (client) {
                MetavoxelData data = client->getAugmentedData();
                data.guide(visitor);
                if (render) {
                    // save the rendered augmented data so that its cached texture references, etc., don't
                    // get collected when we replace it with more recent versions
                    client->setRenderedAugmentedData(data);
                }
            }
        }
    });
}

void MetavoxelSystem::loadSplatProgram(const char* type, ProgramObject& program, SplatLocations& locations) {
    program.addShaderFromSourceFile(QGLShader::Vertex, PathUtils::resourcesPath() +
        "shaders/metavoxel_" + type + "_splat.vert");
    program.addShaderFromSourceFile(QGLShader::Fragment, PathUtils::resourcesPath() +
        "shaders/metavoxel_" + type + "_splat.frag");
    program.link();
    
    program.bind();
    program.setUniformValue("heightMap", 0);
    program.setUniformValue("textureMap", 1);
    program.setUniformValueArray("diffuseMaps", SPLAT_TEXTURE_UNITS, SPLAT_COUNT);
    locations.heightScale = program.uniformLocation("heightScale");
    locations.textureScale = program.uniformLocation("textureScale");
    locations.splatTextureOffset = program.uniformLocation("splatTextureOffset");
    locations.splatTextureScalesS = program.uniformLocation("splatTextureScalesS");
    locations.splatTextureScalesT = program.uniformLocation("splatTextureScalesT");
    locations.textureValueMinima = program.uniformLocation("textureValueMinima");
    locations.textureValueMaxima = program.uniformLocation("textureValueMaxima");
    locations.materials = program.attributeLocation("materials");
    locations.materialWeights = program.attributeLocation("materialWeights");
    program.release();
}

Throttle::Throttle() :
    _limit(INT_MAX),
    _total(0) {
}

bool Throttle::shouldThrottle(int bytes) {
    // clear expired buckets
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    while (!_buckets.isEmpty() && now >= _buckets.first().first) {
        _total -= _buckets.takeFirst().second;
    }
    
    // if possible, add the new bucket
    if (_total + bytes > _limit) {
        return true;
    }
    const int BUCKET_DURATION = 1000;
    _buckets.append(Bucket(now + BUCKET_DURATION, bytes));
    _total += bytes;
    return false;
}

MetavoxelSystemClient::MetavoxelSystemClient(const SharedNodePointer& node, MetavoxelUpdater* updater) :
    MetavoxelClient(node, updater) {
}

void MetavoxelSystemClient::setAugmentedData(const MetavoxelData& data) {
    QWriteLocker locker(&_augmentedDataLock);
    _augmentedData = data;
}

MetavoxelData MetavoxelSystemClient::getAugmentedData() {
    QReadLocker locker(&_augmentedDataLock);
    return _augmentedData;
}

class ReceiveDelayer : public QObject {
public:
    
    ReceiveDelayer(const SharedNodePointer& node, const QByteArray& packet);

protected:

    virtual void timerEvent(QTimerEvent* event);

private:
    
    SharedNodePointer _node;
    QByteArray _packet;
};

ReceiveDelayer::ReceiveDelayer(const SharedNodePointer& node, const QByteArray& packet) :
    _node(node),
    _packet(packet) {
}

void ReceiveDelayer::timerEvent(QTimerEvent* event) {
    QMutexLocker locker(&_node->getMutex());
    MetavoxelClient* client = static_cast<MetavoxelClient*>(_node->getLinkedData());
    if (client) {
        QMetaObject::invokeMethod(&client->getSequencer(), "receivedDatagram", Q_ARG(const QByteArray&, _packet));
    }
    deleteLater();
}

int MetavoxelSystemClient::parseData(const QByteArray& packet) {
    // process through sequencer
    MetavoxelSystem::NetworkSimulation simulation = Application::getInstance()->getMetavoxels()->getNetworkSimulation();
    if (randFloat() < simulation.dropRate) {
        return packet.size();
    }
    int count = (randFloat() < simulation.repeatRate) ? 2 : 1;
    for (int i = 0; i < count; i++) {
        if (simulation.bandwidthLimit > 0) {
            _receiveThrottle.setLimit(simulation.bandwidthLimit);
            if (_receiveThrottle.shouldThrottle(packet.size())) {
                continue;
            }
        }
        int delay = randIntInRange(simulation.minimumDelay, simulation.maximumDelay);
        if (delay > 0) {
            ReceiveDelayer* delayer = new ReceiveDelayer(_node, packet);
            delayer->startTimer(delay);
            
        } else {
            QMetaObject::invokeMethod(&_sequencer, "receivedDatagram", Q_ARG(const QByteArray&, packet));
        }
        Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::METAVOXELS).updateValue(packet.size());
    }
    return packet.size();
}

class AugmentVisitor : public MetavoxelVisitor {
public:
    
    AugmentVisitor(const MetavoxelLOD& lod, const MetavoxelData& previousData);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    const MetavoxelData& _previousData;
};

AugmentVisitor::AugmentVisitor(const MetavoxelLOD& lod, const MetavoxelData& previousData) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getRendererAttribute(),
        QVector<AttributePointer>(), lod),
    _previousData(previousData) {
}

int AugmentVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    static_cast<MetavoxelRenderer*>(info.inputValues.at(0).getInlineValue<
        SharedObjectPointer>().data())->getImplementation()->augment(*_data, _previousData, info, _lod);
    return STOP_RECURSION;
}

class Augmenter : public QRunnable {
public:
    
    Augmenter(const SharedNodePointer& node, const MetavoxelData& data,
        const MetavoxelData& previousData, const MetavoxelLOD& lod);
    
    virtual void run();

private:
    
    QWeakPointer<Node> _node;
    MetavoxelData _data;
    MetavoxelData _previousData;
    MetavoxelLOD _lod;
};

Augmenter::Augmenter(const SharedNodePointer& node, const MetavoxelData& data,
        const MetavoxelData& previousData, const MetavoxelLOD& lod) :
    _node(node),
    _data(data),
    _previousData(previousData),
    _lod(lod) {
}

void Augmenter::run() {
    SharedNodePointer node = _node;
    if (!node) {
        return;
    }
    AugmentVisitor visitor(_lod, _previousData);
    _data.guide(visitor);
    QMutexLocker locker(&node->getMutex());
    QMetaObject::invokeMethod(node->getLinkedData(), "setAugmentedData", Q_ARG(const MetavoxelData&, _data));
}

void MetavoxelSystemClient::dataChanged(const MetavoxelData& oldData) {
    MetavoxelClient::dataChanged(oldData);
    QThreadPool::globalInstance()->start(new Augmenter(_node, _data, getAugmentedData(), _remoteDataLOD));
}

class SendDelayer : public QObject {
public:
    
    SendDelayer(const SharedNodePointer& node, const QByteArray& data);

    virtual void timerEvent(QTimerEvent* event);

private:
    
    SharedNodePointer _node;
    QByteArray _data;
};

SendDelayer::SendDelayer(const SharedNodePointer& node, const QByteArray& data) :
    _node(node),
    _data(data.constData(), data.size()) {
}

void SendDelayer::timerEvent(QTimerEvent* event) {
    DependencyManager::get<NodeList>()->writeDatagram(_data, _node);
    deleteLater();
}

void MetavoxelSystemClient::sendDatagram(const QByteArray& data) {
    MetavoxelSystem::NetworkSimulation simulation = Application::getInstance()->getMetavoxels()->getNetworkSimulation();
    if (randFloat() < simulation.dropRate) {
        return;
    }
    int count = (randFloat() < simulation.repeatRate) ? 2 : 1;
    for (int i = 0; i < count; i++) {
        if (simulation.bandwidthLimit > 0) {
            _sendThrottle.setLimit(simulation.bandwidthLimit);
            if (_sendThrottle.shouldThrottle(data.size())) {
                continue;
            }
        }
        int delay = randIntInRange(simulation.minimumDelay, simulation.maximumDelay);
        if (delay > 0) {
            SendDelayer* delayer = new SendDelayer(_node, data);
            delayer->startTimer(delay);
            
        } else {
            DependencyManager::get<NodeList>()->writeDatagram(data, _node);
        }
        Application::getInstance()->getBandwidthMeter()->outputStream(BandwidthMeter::METAVOXELS).updateValue(data.size());
    }
}

BufferData::~BufferData() {
}

void VoxelPoint::setNormal(const glm::vec3& normal) {
    this->normal[0] = (char)(normal.x * 127.0f);
    this->normal[1] = (char)(normal.y * 127.0f);
    this->normal[2] = (char)(normal.z * 127.0f);
}

VoxelBuffer::VoxelBuffer(const QVector<VoxelPoint>& vertices, const QVector<int>& indices, const QVector<glm::vec3>& hermite,
        const QMultiHash<VoxelCoord, int>& quadIndices, int size, const QVector<SharedObjectPointer>& materials) :
    _vertices(vertices),
    _indices(indices),
    _hermite(hermite),
    _hermiteEnabled(Menu::getInstance()->isOptionChecked(MenuOption::DisplayHermiteData)),
    _quadIndices(quadIndices),
    _size(size),
    _vertexCount(vertices.size()),
    _indexCount(indices.size()),
    _hermiteCount(hermite.size()),
    _vertexBufferID(0),
    _indexBufferID(0),
    _hermiteBufferID(0),
    _materials(materials) {
}

VoxelBuffer::~VoxelBuffer() {
    QMetaObject::invokeMethod(Application::getInstance()->getMetavoxels(), "deleteBuffers", Q_ARG(int, _vertexBufferID),
        Q_ARG(int, _indexBufferID), Q_ARG(int, _hermiteBufferID));
}

bool VoxelBuffer::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        float boundsDistance, float& distance) const {
    float highest = _size - 1.0f;
    glm::vec3 position = (origin + direction * boundsDistance) * highest;
    glm::vec3 floors = glm::floor(position);
    int max = _size - 2;
    int x = qMin((int)floors.x, max), y = qMin((int)floors.y, max), z = qMin((int)floors.z, max);
    forever {
        VoxelCoord key(qRgb(x, y, z));
        for (QMultiHash<VoxelCoord, int>::const_iterator it = _quadIndices.constFind(key);
                it != _quadIndices.constEnd() && it.key() == key; it++) {
            const int* indices = _indices.constData() + *it;
            if (findRayTriangleIntersection(origin, direction, _vertices.at(indices[0]).vertex,
                    _vertices.at(indices[1]).vertex, _vertices.at(indices[2]).vertex, distance) ||
                findRayTriangleIntersection(origin, direction, _vertices.at(indices[0]).vertex,
                    _vertices.at(indices[2]).vertex, _vertices.at(indices[3]).vertex, distance)) {
                return true;
            }
        }
        float xDistance = FLT_MAX, yDistance = FLT_MAX, zDistance = FLT_MAX;
        if (direction.x > 0.0f) {
            xDistance = (x + 1.0f - position.x) / direction.x;
        } else if (direction.x < 0.0f) {
            xDistance = (x - position.x) / direction.x;
        }
        if (direction.y > 0.0f) {
            yDistance = (y + 1.0f - position.y) / direction.y;
        } else if (direction.y < 0.0f) {
            yDistance = (y - position.y) / direction.y;
        }
        if (direction.z > 0.0f) {
            zDistance = (z + 1.0f - position.z) / direction.z;
        } else if (direction.z < 0.0f) {
            zDistance = (z - position.z) / direction.z;
        }
        float minimumDistance = qMin(xDistance, qMin(yDistance, zDistance));
        if (minimumDistance == xDistance) {
            if (direction.x > 0.0f) {
                if (x++ == max) {
                    return false;
                }
            } else if (x-- == 0) {
                return false;
            }
        }
        if (minimumDistance == yDistance) {
            if (direction.y > 0.0f) {
                if (y++ == max) {
                    return false;
                }
            } else if (y-- == 0) {
                return false;
            }
        }
        if (minimumDistance == zDistance) {
            if (direction.z > 0.0f) {
                if (z++ == max) {
                    return false;
                }
            } else if (z-- == 0) {
                return false;
            }
        }
        position += direction * minimumDistance;
    }
    return false;
}

void VoxelBuffer::render(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale, bool cursor) {
    if (_vertexBufferID == 0) {
        glGenBuffers(1, &_vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(VoxelPoint), _vertices.constData(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glGenBuffers(1, &_indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(int), _indices.constData(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
        if (!_materials.isEmpty()) {
            _networkTextures.resize(_materials.size());
            auto textureCache = DependencyManager::get<TextureCache>();
            for (int i = 0; i < _materials.size(); i++) {
                const SharedObjectPointer material = _materials.at(i);
                if (material) {
                    _networkTextures[i] = textureCache->getTexture(
                        static_cast<MaterialObject*>(material.data())->getDiffuse(), SPLAT_TEXTURE);
                }
            }
        }
    }
    
    MetavoxelBatch baseBatch;
    baseBatch.translation = translation;
    baseBatch.rotation = rotation;
    baseBatch.scale = scale;
    baseBatch.vertexBufferID = _vertexBufferID;
    baseBatch.indexBufferID = _indexBufferID;
    baseBatch.vertexCount = _vertexCount;
    baseBatch.indexCount = _indexCount;
    Application::getInstance()->getMetavoxels()->addVoxelBaseBatch(baseBatch);
    
    if (!(cursor || _materials.isEmpty())) {
        VoxelSplatBatch splatBatch;
        splatBatch.translation = translation;
        splatBatch.rotation = rotation;
        splatBatch.scale = scale;
        splatBatch.vertexBufferID = _vertexBufferID;
        splatBatch.indexBufferID = _indexBufferID;
        splatBatch.vertexCount = _vertexCount;
        splatBatch.indexCount = _indexCount;
        splatBatch.splatTextureOffset = glm::vec3(
            glm::dot(translation, rotation * glm::vec3(1.0f, 0.0f, 0.0f)) / scale.x,
            glm::dot(translation, rotation * glm::vec3(0.0f, 1.0f, 0.0f)) / scale.y,
            glm::dot(translation, rotation * glm::vec3(0.0f, 0.0f, 1.0f)) / scale.z);
            
        for (int i = 0; i < _materials.size(); i += SPLAT_COUNT) {
            for (int j = 0; j < SPLAT_COUNT; j++) {
                int index = i + j;
                if (index < _networkTextures.size()) {
                    const NetworkTexturePointer& texture = _networkTextures.at(index);
                    if (texture) {
                        MaterialObject* material = static_cast<MaterialObject*>(_materials.at(index).data());
                        splatBatch.splatTextureScalesS[j] = scale.x / material->getScaleS();
                        splatBatch.splatTextureScalesT[j] = scale.z / material->getScaleT();
                        splatBatch.splatTextureIDs[j] = texture->getID();
                           
                    } else {
                        splatBatch.splatTextureIDs[j] = 0;
                    }
                } else {
                    splatBatch.splatTextureIDs[j] = 0;
                }
            }
            splatBatch.materialIndex = i;
            Application::getInstance()->getMetavoxels()->addVoxelSplatBatch(splatBatch);
        }
    }
    
    if (_hermiteCount > 0) {
        if (_hermiteBufferID == 0) {
            glGenBuffers(1, &_hermiteBufferID);
            glBindBuffer(GL_ARRAY_BUFFER, _hermiteBufferID);
            glBufferData(GL_ARRAY_BUFFER, _hermite.size() * sizeof(glm::vec3), _hermite.constData(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            _hermite.clear();
        }
        HermiteBatch hermiteBatch;
        hermiteBatch.translation = translation;
        hermiteBatch.rotation = rotation;
        hermiteBatch.scale = scale;
        hermiteBatch.vertexBufferID = _hermiteBufferID;
        hermiteBatch.vertexCount = _hermiteCount;
        Application::getInstance()->getMetavoxels()->addHermiteBatch(hermiteBatch);
    }
}

DefaultMetavoxelRendererImplementation::DefaultMetavoxelRendererImplementation() {
}

class SpannerSimulateVisitor : public SpannerVisitor {
public:
    
    SpannerSimulateVisitor(float deltaTime, const MetavoxelLOD& lod);
    
    virtual bool visit(Spanner* spanner);

private:
    
    float _deltaTime;
};

SpannerSimulateVisitor::SpannerSimulateVisitor(float deltaTime, const MetavoxelLOD& lod) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>(), QVector<AttributePointer>(), lod),
    _deltaTime(deltaTime) {
}

bool SpannerSimulateVisitor::visit(Spanner* spanner) {
    spanner->getRenderer()->simulate(_deltaTime);
    return true;
}

void DefaultMetavoxelRendererImplementation::simulate(MetavoxelData& data, float deltaTime,
        MetavoxelInfo& info, const MetavoxelLOD& lod) {
    SpannerSimulateVisitor spannerSimulateVisitor(deltaTime, lod);
    data.guide(spannerSimulateVisitor);
}

void DefaultMetavoxelRendererImplementation::render(MetavoxelData& data, MetavoxelInfo& info, const MetavoxelLOD& lod) {
    SpannerRenderVisitor spannerRenderVisitor(lod);
    data.guide(spannerRenderVisitor);
}

SphereRenderer::SphereRenderer() {
}

void SphereRenderer::render(const MetavoxelLOD& lod, bool contained, bool cursor) {
    Sphere* sphere = static_cast<Sphere*>(_spanner);
    const QColor& color = sphere->getColor();
    
    glPushMatrix();
    const glm::vec3& translation = sphere->getTranslation();
    glTranslatef(translation.x, translation.y, translation.z);
    glm::quat rotation = sphere->getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
    
    DependencyManager::get<DeferredLightingEffect>()->renderSolidSphere(sphere->getScale(), 32, 32,
                                                            glm::vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
    
    glPopMatrix();
}

CuboidRenderer::CuboidRenderer() {
}

void CuboidRenderer::render(const MetavoxelLOD& lod, bool contained, bool cursor) {
    Cuboid* cuboid = static_cast<Cuboid*>(_spanner);
    const QColor& color = cuboid->getColor();
    
    glPushMatrix();
    const glm::vec3& translation = cuboid->getTranslation();
    glTranslatef(translation.x, translation.y, translation.z);
    glm::quat rotation = cuboid->getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
    glScalef(1.0f, cuboid->getAspectY(), cuboid->getAspectZ());
    
    DependencyManager::get<DeferredLightingEffect>()->renderSolidCube(cuboid->getScale() * 2.0f,
                                                            glm::vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
    
    glPopMatrix();
}

StaticModelRenderer::StaticModelRenderer() :
    _model(new Model(this)) {
}

void StaticModelRenderer::init(Spanner* spanner) {
    SpannerRenderer::init(spanner);

    _model->init();
    
    StaticModel* staticModel = static_cast<StaticModel*>(spanner);
    applyTranslation(staticModel->getTranslation());
    applyRotation(staticModel->getRotation());
    applyScale(staticModel->getScale());
    applyURL(staticModel->getURL());
    
    connect(spanner, SIGNAL(translationChanged(const glm::vec3&)), SLOT(applyTranslation(const glm::vec3&)));
    connect(spanner, SIGNAL(rotationChanged(const glm::quat&)), SLOT(applyRotation(const glm::quat&)));
    connect(spanner, SIGNAL(scaleChanged(float)), SLOT(applyScale(float)));
    connect(spanner, SIGNAL(urlChanged(const QUrl&)), SLOT(applyURL(const QUrl&)));
}

void StaticModelRenderer::simulate(float deltaTime) {
    // update the bounds
    Box bounds;
    if (_model->isActive()) {
        const Extents& extents = _model->getGeometry()->getFBXGeometry().meshExtents;
        bounds = Box(extents.minimum, extents.maximum);
    }
    static_cast<StaticModel*>(_spanner)->setBounds(glm::translate(_model->getTranslation()) *
        glm::mat4_cast(_model->getRotation()) * glm::scale(_model->getScale()) * bounds);
    _model->simulate(deltaTime);
}

void StaticModelRenderer::render(const MetavoxelLOD& lod, bool contained, bool cursor) {
    _model->render();
}

bool StaticModelRenderer::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    RayIntersectionInfo info;
    info._rayStart = origin;
    info._rayDirection = direction;
    if (!_model->findRayIntersection(info)) {
        return false;
    }
    distance = info._hitDistance;
    return true;
}

void StaticModelRenderer::applyTranslation(const glm::vec3& translation) {
    _model->setTranslation(translation);
}

void StaticModelRenderer::applyRotation(const glm::quat& rotation) {
    _model->setRotation(rotation);
}

void StaticModelRenderer::applyScale(float scale) {
    _model->setScale(glm::vec3(scale, scale, scale));
}

void StaticModelRenderer::applyURL(const QUrl& url) {
    _model->setURL(url);
}

HeightfieldRenderer::HeightfieldRenderer() {
}

const int X_MAXIMUM_FLAG = 1;
const int Y_MAXIMUM_FLAG = 2;

static void renderNode(const HeightfieldNodePointer& node, Heightfield* heightfield, const MetavoxelLOD& lod,
        const glm::vec2& minimum, float size, bool contained, bool cursor) {
    const glm::quat& rotation = heightfield->getRotation();
    glm::vec3 scale(heightfield->getScale() * size, heightfield->getScale() * heightfield->getAspectY(),
        heightfield->getScale() * heightfield->getAspectZ() * size);
    glm::vec3 translation = heightfield->getTranslation() + rotation * glm::vec3(minimum.x * heightfield->getScale(),
        0.0f, minimum.y * heightfield->getScale() * heightfield->getAspectZ());
    if (!contained) {
        Frustum::IntersectionType type = Application::getInstance()->getMetavoxels()->getFrustum().getIntersectionType(
            glm::translate(translation) * glm::mat4_cast(rotation) * Box(glm::vec3(), scale));
        if (type == Frustum::NO_INTERSECTION) {
            return;
        }
        if (type == Frustum::CONTAINS_INTERSECTION) {
            contained = true;
        }
    }
    if (!node->isLeaf() && lod.shouldSubdivide(minimum, size)) {
        float nextSize = size * 0.5f;
        for (int i = 0; i < HeightfieldNode::CHILD_COUNT; i++) {
            renderNode(node->getChild(i), heightfield, lod, minimum + glm::vec2(i & X_MAXIMUM_FLAG ? nextSize : 0.0f,
                i & Y_MAXIMUM_FLAG ? nextSize : 0.0f), nextSize, contained, cursor);
        }
        return;
    }
    HeightfieldNodeRenderer* renderer = static_cast<HeightfieldNodeRenderer*>(node->getRenderer());
    if (!renderer) {
        node->setRenderer(renderer = new HeightfieldNodeRenderer());
    }
    renderer->render(node, translation, rotation, scale, cursor);
}

void HeightfieldRenderer::render(const MetavoxelLOD& lod, bool contained, bool cursor) {
    Heightfield* heightfield = static_cast<Heightfield*>(_spanner);
    renderNode(heightfield->getRoot(), heightfield, heightfield->transformLOD(lod), glm::vec2(), 1.0f, contained, cursor);
}

HeightfieldNodeRenderer::HeightfieldNodeRenderer() :
    _heightTextureID(0),
    _colorTextureID(0),
    _materialTextureID(0) {
}

HeightfieldNodeRenderer::~HeightfieldNodeRenderer() {
    QMetaObject::invokeMethod(Application::getInstance()->getMetavoxels(), "deleteTextures", Q_ARG(int, _heightTextureID),
        Q_ARG(int, _colorTextureID), Q_ARG(int, _materialTextureID));
}

bool HeightfieldNodeRenderer::findRayIntersection(const glm::vec3& translation, const glm::quat& rotation,
        const glm::vec3& scale, const glm::vec3& origin, const glm::vec3& direction,
        float boundsDistance, float& distance) const {
    if (!_voxels) {
        return false;
    }
    glm::quat inverseRotation = glm::inverse(rotation);
    float inverseScale = 1.0f / scale.x;
    return static_cast<const VoxelBuffer*>(_voxels.data())->findRayIntersection(
        inverseRotation * (origin - translation) * inverseScale, inverseRotation * direction * inverseScale,
        boundsDistance, distance);
}

class EdgeCrossing {
public:
    glm::vec3 point;
    glm::vec3 normal;
    QRgb color;
    char material;
    
    void setColorMaterial(const StackArray::Entry& entry) { color = entry.color; material = entry.material; }
    
    void mix(const EdgeCrossing& first, const EdgeCrossing& second, float t);
    
    VoxelPoint createPoint(int clampedX, int clampedZ, float step) const;
};

void EdgeCrossing::mix(const EdgeCrossing& first, const EdgeCrossing& second, float t) {
    point = glm::mix(first.point, second.point, t);
    normal = glm::normalize(glm::mix(first.normal, second.normal, t));
    color = qRgb(glm::mix(qRed(first.color), qRed(second.color), t), glm::mix(qGreen(first.color), qGreen(second.color), t),
        glm::mix(qBlue(first.color), qBlue(second.color), t));
    material = (t < 0.5f) ? first.material : second.material;
}

VoxelPoint EdgeCrossing::createPoint(int clampedX, int clampedZ, float step) const {
    VoxelPoint voxelPoint = { glm::vec3(clampedX + point.x, point.y, clampedZ + point.z) * step,
        { (quint8)qRed(color), (quint8)qGreen(color), (quint8)qBlue(color) },
        { (char)(normal.x * numeric_limits<qint8>::max()), (char)(normal.y * numeric_limits<qint8>::max()),
            (char)(normal.z * numeric_limits<qint8>::max()) },
        { (quint8)material, 0, 0, 0 },
        { numeric_limits<quint8>::max(), 0, 0, 0 } };
    return voxelPoint;
}

const int MAX_NORMALS_PER_VERTEX = 4;

class NormalIndex {
public:
    int indices[MAX_NORMALS_PER_VERTEX];
    
    bool isValid() const;
    
    int getClosestIndex(const glm::vec3& normal, QVector<VoxelPoint>& vertices) const;
};

bool NormalIndex::isValid() const {
    for (int i = 0; i < MAX_NORMALS_PER_VERTEX; i++) {
        if (indices[i] >= 0) {
            return true;
        }
    }
    return false;
}

int NormalIndex::getClosestIndex(const glm::vec3& normal, QVector<VoxelPoint>& vertices) const {
    int firstIndex = indices[0];
    int closestIndex = firstIndex;
    const VoxelPoint& firstVertex = vertices.at(firstIndex);
    float closest = normal.x * firstVertex.normal[0] + normal.y * firstVertex.normal[1] + normal.z * firstVertex.normal[2];
    for (int i = 1; i < MAX_NORMALS_PER_VERTEX; i++) {
        int index = indices[i];
        if (index == firstIndex) {
            break;
        }
        const VoxelPoint& vertex = vertices.at(index);
        float product = normal.x * vertex.normal[0] + normal.y * vertex.normal[1] + normal.z * vertex.normal[2];
        if (product > closest) {
            closest = product;
            closestIndex = index;
        }
    }
    return closestIndex;
}

static glm::vec3 safeNormalize(const glm::vec3& vector) {
    float length = glm::length(vector);
    return (length > 0.0f) ? (vector / length) : vector;
}

class IndexVector : public QVector<NormalIndex> {
public:
    
    int position;
    
    void swap(IndexVector& other) { QVector<NormalIndex>::swap(other); qSwap(position, other.position); }
    
    const NormalIndex& get(int y) const;
};

const NormalIndex& IndexVector::get(int y) const {
    static NormalIndex invalidIndex = { { -1, -1, -1, -1 } };
    int relative = y - position;
    return (relative >= 0 && relative < size()) ? at(relative) : invalidIndex;
}

static inline glm::vec3 getNormal(const QVector<VoxelPoint>& vertices, const NormalIndex& i0,
        const NormalIndex& i1, const NormalIndex& i2, const NormalIndex& i3) {
    // check both triangles in case one is degenerate
    const glm::vec3& v0 = vertices.at(i0.indices[0]).vertex;
    glm::vec3 normal = glm::cross(vertices.at(i1.indices[0]).vertex - v0, vertices.at(i2.indices[0]).vertex - v0);
    if (glm::length(normal) > EPSILON) {
        return normal;
    }
    return glm::cross(vertices.at(i2.indices[0]).vertex - v0, vertices.at(i3.indices[0]).vertex - v0);
}

static inline void appendTriangle(const EdgeCrossing& e0, const EdgeCrossing& e1, const EdgeCrossing& e2,
        int clampedX, int clampedZ, float step, QVector<VoxelPoint>& vertices, QVector<int>& indices,
        QMultiHash<VoxelCoord, int>& quadIndices) {
    int firstIndex = vertices.size();
    vertices.append(e0.createPoint(clampedX, clampedZ, step));
    vertices.append(e1.createPoint(clampedX, clampedZ, step));
    vertices.append(e2.createPoint(clampedX, clampedZ, step));
    indices.append(firstIndex);
    indices.append(firstIndex + 1);
    indices.append(firstIndex + 2);
    indices.append(firstIndex + 2);
    
    int minimumY = qMin((int)e0.point.y, qMin((int)e1.point.y, (int)e2.point.y));
    int maximumY = qMax((int)e0.point.y, qMax((int)e1.point.y, (int)e2.point.y));
    for (int y = minimumY; y <= maximumY; y++) {
        quadIndices.insert(qRgb(clampedX, y, clampedZ), firstIndex);
    }
}

const int CORNER_COUNT = 4;
                    
static StackArray::Entry getEntry(const StackArray* lineSrc, int stackWidth, int y, float heightfieldHeight,
        EdgeCrossing cornerCrossings[CORNER_COUNT], int cornerIndex) {
    int offsetX = (cornerIndex & X_MAXIMUM_FLAG) ? 1 : 0;
    int offsetZ = (cornerIndex & Y_MAXIMUM_FLAG) ? 1 : 0;
    const StackArray& src = lineSrc[offsetZ * stackWidth + offsetX];
    int count = src.getEntryCount();
    if (count > 0) {
        int relative = y - src.getPosition();
        if (relative < count && (relative >= 0 || heightfieldHeight == 0.0f)) {
            return src.getEntry(y, heightfieldHeight);
        }
    }
    const EdgeCrossing& cornerCrossing = cornerCrossings[cornerIndex];
    if (cornerCrossing.point.y == 0.0f) {
        return src.getEntry(y, heightfieldHeight);
    }
    StackArray::Entry entry;
    bool set = false;
    if (cornerCrossing.point.y >= y) {
        entry.color = cornerCrossing.color;
        entry.material = cornerCrossing.material;
        set = true;
        entry.setHermiteY(cornerCrossing.normal, glm::clamp(cornerCrossing.point.y - y, 0.0f, 1.0f));
        
    } else {
        entry.material = entry.color = 0;
    }
    if (!(cornerIndex & X_MAXIMUM_FLAG)) {
        const EdgeCrossing& nextCornerCrossingX = cornerCrossings[cornerIndex | X_MAXIMUM_FLAG];
        if (nextCornerCrossingX.point.y != 0.0f && (nextCornerCrossingX.point.y >= y) != set) {
            float t = glm::clamp((y - cornerCrossing.point.y) /
                (nextCornerCrossingX.point.y - cornerCrossing.point.y), 0.0f, 1.0f);
            entry.setHermiteX(glm::normalize(glm::mix(cornerCrossing.normal, nextCornerCrossingX.normal, t)), t);
        }
    }
    if (!(cornerIndex & Y_MAXIMUM_FLAG)) {
        const EdgeCrossing& nextCornerCrossingZ = cornerCrossings[cornerIndex | Y_MAXIMUM_FLAG];
        if (nextCornerCrossingZ.point.y != 0.0f && (nextCornerCrossingZ.point.y >= y) != set) {
            float t = glm::clamp((y - cornerCrossing.point.y) /
                (nextCornerCrossingZ.point.y - cornerCrossing.point.y), 0.0f, 1.0f);
            entry.setHermiteZ(glm::normalize(glm::mix(cornerCrossing.normal, nextCornerCrossingZ.normal, t)), t);
        }
    }
    return entry;
}

void HeightfieldNodeRenderer::render(const HeightfieldNodePointer& node, const glm::vec3& translation,
        const glm::quat& rotation, const glm::vec3& scale, bool cursor) {
    if (!node->getHeight()) {
        return;
    }
    int width = node->getHeight()->getWidth();
    int height = node->getHeight()->getContents().size() / width;
    int innerWidth = width - 2 * HeightfieldHeight::HEIGHT_BORDER;
    int innerHeight = height - 2 * HeightfieldHeight::HEIGHT_BORDER;
    int vertexCount = width * height;
    int rows = height - 1;
    int columns = width - 1;
    int indexCount = rows * columns * 3 * 2;
    BufferPair& bufferPair = _bufferPairs[IntPair(width, height)];
    if (!bufferPair.first.isCreated()) {
        QVector<HeightfieldPoint> vertices(vertexCount);
        HeightfieldPoint* point = vertices.data();
        
        float xStep = 1.0f / (innerWidth - 1);
        float zStep = 1.0f / (innerHeight - 1);
        float z = -zStep;
        float sStep = 1.0f / width;
        float tStep = 1.0f / height;
        float t = tStep / 2.0f;
        for (int i = 0; i < height; i++, z += zStep, t += tStep) {
            float x = -xStep;
            float s = sStep / 2.0f;
            const float SKIRT_LENGTH = 0.25f;
            float baseY = (i == 0 || i == height - 1) ? -SKIRT_LENGTH : 0.0f;
            for (int j = 0; j < width; j++, point++, x += xStep, s += sStep) {
                point->vertex = glm::vec3(x, (j == 0 || j == width - 1) ? -SKIRT_LENGTH : baseY, z);
                point->textureCoord = glm::vec2(s, t);
            }
        }
        
        bufferPair.first.setUsagePattern(QOpenGLBuffer::StaticDraw);
        bufferPair.first.create();
        bufferPair.first.bind();
        bufferPair.first.allocate(vertices.constData(), vertexCount * sizeof(HeightfieldPoint));
        bufferPair.first.release();
        
        QVector<int> indices(indexCount);
        int* index = indices.data();
        for (int i = 0; i < rows; i++) {
            int lineIndex = i * width;
            int nextLineIndex = (i + 1) * width;
            for (int j = 0; j < columns; j++) {
                *index++ = lineIndex + j;
                *index++ = nextLineIndex + j;
                *index++ = nextLineIndex + j + 1;
                
                *index++ = nextLineIndex + j + 1;
                *index++ = lineIndex + j + 1;
                *index++ = lineIndex + j;
            }
        }
        
        bufferPair.second = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        bufferPair.second.create();
        bufferPair.second.bind();
        bufferPair.second.allocate(indices.constData(), indexCount * sizeof(int));
        bufferPair.second.release();
    }
    if (_heightTextureID == 0) {
        // we use non-aligned data for the various layers
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
        glGenTextures(1, &_heightTextureID);
        glBindTexture(GL_TEXTURE_2D, _heightTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, width, height, 0,
            GL_RED, GL_UNSIGNED_SHORT, node->getHeight()->getContents().constData());
    
        glGenTextures(1, &_colorTextureID);
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (node->getColor()) {
            const QByteArray& contents = node->getColor()->getContents();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, node->getColor()->getWidth(),
                contents.size() / (node->getColor()->getWidth() * DataBlock::COLOR_BYTES),
                0, GL_RGB, GL_UNSIGNED_BYTE, contents.constData());
            glGenerateMipmap(GL_TEXTURE_2D);
            
        } else {
            const quint8 WHITE_COLOR[] = { 255, 255, 255 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, WHITE_COLOR);
        }
        
        glGenTextures(1, &_materialTextureID);
        glBindTexture(GL_TEXTURE_2D, _materialTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (node->getMaterial()) {
            const QByteArray& contents = node->getMaterial()->getContents();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, node->getMaterial()->getWidth(),
                contents.size() / node->getMaterial()->getWidth(),
                0, GL_RED, GL_UNSIGNED_BYTE, contents.constData());
                
            const QVector<SharedObjectPointer>& materials = node->getMaterial()->getMaterials();
            _networkTextures.resize(materials.size());
            auto textureCache = DependencyManager::get<TextureCache>();
            for (int i = 0; i < materials.size(); i++) {
                const SharedObjectPointer& material = materials.at(i);
                if (material) {
                    _networkTextures[i] = textureCache->getTexture(
                        static_cast<MaterialObject*>(material.data())->getDiffuse(), SPLAT_TEXTURE);
                }
            }
        } else {
            const quint8 ZERO_VALUE = 0;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &ZERO_VALUE);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        
        // restore the default alignment; it's what Qt uses for image storage
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
    bool displayHermite = Menu::getInstance()->isOptionChecked(MenuOption::DisplayHermiteData);
    if ((!_voxels || (displayHermite && !static_cast<VoxelBuffer*>(_voxels.data())->isHermiteEnabled())) && node->getStack()) {
        // see http://www.frankpetterson.com/publications/dualcontour/dualcontour.pdf for a description of the
        // dual contour algorithm for generating meshes from voxel data using Hermite-tagged edges
        
        QVector<VoxelPoint> vertices;
        QVector<int> indices;
        QVector<glm::vec3> hermiteSegments;
        QMultiHash<VoxelCoord, int> quadIndices;
        
        int stackWidth = node->getStack()->getWidth();
        int stackHeight = node->getStack()->getContents().size() / stackWidth;
        int innerStackWidth = stackWidth - HeightfieldData::SHARED_EDGE;
        int innerStackHeight = stackHeight - HeightfieldData::SHARED_EDGE;
        const StackArray* src = node->getStack()->getContents().constData();
        const quint16* heightSrc = node->getHeight()->getContents().constData() +
            (width + 1) * HeightfieldHeight::HEIGHT_BORDER;
        QVector<SharedObjectPointer> stackMaterials = node->getStack()->getMaterials();
        QHash<int, int> materialMap;
        
        int colorWidth;
        const uchar* colorSrc = NULL;
        float colorStepX, colorStepZ;
        if (node->getColor()) {
            colorWidth = node->getColor()->getWidth();
            int colorHeight = node->getColor()->getContents().size() / (colorWidth * DataBlock::COLOR_BYTES);
            colorSrc = (const uchar*)node->getColor()->getContents().constData();
            colorStepX = (colorWidth - HeightfieldData::SHARED_EDGE) / (float)innerStackWidth;
            colorStepZ = (colorHeight - HeightfieldData::SHARED_EDGE) / (float)innerStackHeight;
        }
        
        int materialWidth;
        const uchar* materialSrc = NULL;
        float materialStepX, materialStepZ;
        if (node->getMaterial()) {
            materialWidth = node->getMaterial()->getWidth();
            int materialHeight = node->getMaterial()->getContents().size() / materialWidth;
            materialSrc = (const uchar*)node->getMaterial()->getContents().constData();
            materialStepX = (materialWidth - HeightfieldData::SHARED_EDGE) / (float)innerStackWidth;
            materialStepZ = (materialHeight - HeightfieldData::SHARED_EDGE) / (float)innerStackHeight;
        }
        
        const int EDGES_PER_CUBE = 12;
        EdgeCrossing crossings[EDGES_PER_CUBE * 2];
        
        // as we scan down the cube generating vertices between grid points, we remember the indices of the last
        // (element, line, section--x, y, z) so that we can connect generated vertices as quads
        IndexVector indicesX;
        IndexVector lastIndicesX;
        QVector<IndexVector> indicesZ(stackWidth + 1);
        QVector<IndexVector> lastIndicesZ(stackWidth + 1);
        float step = 1.0f / innerStackWidth;
        float voxelScale = scale.y / (numeric_limits<quint16>::max() * scale.x * step);
        
        for (int z = 0; z <= stackHeight; z++) {
            bool middleZ = (z != 0 && z != stackHeight);
            const StackArray* lineSrc = src;
            const quint16* heightLineSrc = heightSrc;
            for (int x = 0; x <= stackWidth; x++) {
                bool middleX = (x != 0 && x != stackWidth);
                
                // find the y extents of this and the neighboring columns
                int minimumY = INT_MAX, maximumY = -1;
                lineSrc->getExtents(minimumY, maximumY);
                if (middleX) {
                    lineSrc[1].getExtents(minimumY, maximumY);
                    if (middleZ) {
                        lineSrc[stackWidth + 1].getExtents(minimumY, maximumY);
                    }
                }
                if (middleZ) {
                    lineSrc[stackWidth].getExtents(minimumY, maximumY);
                }
                if (maximumY >= minimumY) {
                    float heightfieldHeight = *heightLineSrc * voxelScale;
                    float nextHeightfieldHeightX = heightLineSrc[1] * voxelScale;
                    float nextHeightfieldHeightZ = heightLineSrc[width] * voxelScale;
                    float nextHeightfieldHeightXZ = heightLineSrc[width + 1] * voxelScale;
                    const int UPPER_LEFT_CORNER = 1;
                    const int UPPER_RIGHT_CORNER = 2;
                    const int LOWER_LEFT_CORNER = 4;
                    const int LOWER_RIGHT_CORNER = 8;
                    const int NO_CORNERS = 0;
                    const int ALL_CORNERS = UPPER_LEFT_CORNER | UPPER_RIGHT_CORNER | LOWER_LEFT_CORNER | LOWER_RIGHT_CORNER;
                    const int NEXT_CORNERS[] = { 1, 3, 0, 2 };
                    int corners = NO_CORNERS;
                    if (heightfieldHeight != 0.0f) {
                        corners |= UPPER_LEFT_CORNER;
                    }
                    if (nextHeightfieldHeightX != 0.0f && x != stackWidth) {
                        corners |= UPPER_RIGHT_CORNER;
                    }
                    if (nextHeightfieldHeightZ != 0.0f && z != stackHeight) {
                        corners |= LOWER_LEFT_CORNER;
                    }
                    if (nextHeightfieldHeightXZ != 0.0f && x != stackWidth && z != stackHeight) {
                        corners |= LOWER_RIGHT_CORNER;
                    }
                    bool stitchable = x != 0 && z != 0 && !(corners == NO_CORNERS || corners == ALL_CORNERS);
                    EdgeCrossing cornerCrossings[CORNER_COUNT];
                    int clampedX = qMax(x - 1, 0), clampedZ = qMax(z - 1, 0);
                    int cornerMinimumY = INT_MAX, cornerMaximumY = -1;
                    if (stitchable) {
                        for (int i = 0; i < CORNER_COUNT; i++) {
                            if (!(corners & (1 << i))) {
                                continue;
                            }
                            int offsetX = (i & X_MAXIMUM_FLAG) ? 1 : 0;
                            int offsetZ = (i & Y_MAXIMUM_FLAG) ? 1 : 0;
                            const quint16* height = heightLineSrc + offsetZ * width + offsetX;
                            float heightValue = *height * voxelScale;
                            int y = (int)heightValue;
                            cornerMinimumY = qMin(cornerMinimumY, y);
                            cornerMaximumY = qMax(cornerMaximumY, y);
                            EdgeCrossing& crossing = cornerCrossings[i];
                            crossing.point = glm::vec3(offsetX, heightValue, offsetZ);
                            int left = height[-1];
                            int right = height[1];
                            int down = height[-width];
                            int up = height[width];
                            crossing.normal = glm::normalize(glm::vec3((left == 0 || right == 0) ? 0.0f : left - right,
                                2.0f / voxelScale, (up == 0 || down == 0) ? 0.0f : down - up));
                            int clampedOffsetX = clampedX + offsetX, clampedOffsetZ = clampedZ + offsetZ;
                            if (colorSrc) {
                                const uchar* color = colorSrc + ((int)(clampedOffsetZ * colorStepZ) * colorWidth +
                                    (int)(clampedOffsetX * colorStepX)) * DataBlock::COLOR_BYTES;
                                crossing.color = qRgb(color[0], color[1], color[2]);
                             
                            } else {
                                crossing.color = qRgb(numeric_limits<quint8>::max(), numeric_limits<quint8>::max(),
                                    numeric_limits<quint8>::max());
                            }
                            int material = 0;
                            if (materialSrc) {
                                material = materialSrc[(int)(clampedOffsetZ * materialStepZ) * materialWidth +
                                    (int)(clampedOffsetX * materialStepX)];
                                if (material != 0) {
                                    int& mapping = materialMap[material];
                                    if (mapping == 0) {
                                        mapping = getMaterialIndex(node->getMaterial()->getMaterials().at(material - 1),
                                            stackMaterials);
                                    }
                                    material = mapping;
                                }
                            }
                            crossing.material = material;
                        }
                        minimumY = qMin(minimumY, cornerMinimumY);
                        maximumY = qMax(maximumY, cornerMaximumY);
                        
                        if (corners == (LOWER_LEFT_CORNER | UPPER_LEFT_CORNER | UPPER_RIGHT_CORNER)) {
                            appendTriangle(cornerCrossings[1], cornerCrossings[0], cornerCrossings[2],
                                clampedX, clampedZ, step, vertices, indices, quadIndices);
                        
                        } else if (corners == (UPPER_RIGHT_CORNER | LOWER_RIGHT_CORNER | LOWER_LEFT_CORNER)) {
                            appendTriangle(cornerCrossings[2], cornerCrossings[3], cornerCrossings[1],
                                clampedX, clampedZ, step, vertices, indices, quadIndices);
                        }
                    }
                    int position = minimumY;
                    int count = maximumY - minimumY + 1;
                    NormalIndex lastIndexY = { { -1, -1, -1, -1 } };
                    indicesX.position = position;
                    indicesX.resize(count);
                    indicesZ[x].position = position;
                    indicesZ[x].resize(count);
                    for (int y = position, end = position + count; y < end; y++) {
                        StackArray::Entry entry = getEntry(lineSrc, stackWidth, y, heightfieldHeight, cornerCrossings, 0);
                        if (displayHermite && x != 0 && z != 0 && !lineSrc->isEmpty() && y >= lineSrc->getPosition()) {
                            glm::vec3 normal;
                            if (entry.hermiteX != 0) {
                                glm::vec3 start = glm::vec3(clampedX + entry.getHermiteX(normal), y, clampedZ) * step;
                                hermiteSegments.append(start);
                                hermiteSegments.append(start + normal * step);
                            }
                            if (entry.hermiteY != 0) {
                                glm::vec3 start = glm::vec3(clampedX, y + entry.getHermiteY(normal), clampedZ) * step;
                                hermiteSegments.append(start);
                                hermiteSegments.append(start + normal * step);
                            }
                            if (entry.hermiteZ != 0) {
                                glm::vec3 start = glm::vec3(clampedX, y, clampedZ + entry.getHermiteZ(normal)) * step;
                                hermiteSegments.append(start);
                                hermiteSegments.append(start + normal * step);
                            }
                        }
                        // number variables correspond to cube corners, where the x, y, and z components are represented as
                        // bits in the 0, 1, and 2 position, respectively; hence, alpha0 is the value at the minimum x, y, and
                        // z corner and alpha7 is the value at the maximum x, y, and z
                        int alpha0 = lineSrc->getEntryAlpha(y, heightfieldHeight);
                        int alpha2 = lineSrc->getEntryAlpha(y + 1, heightfieldHeight);
                        int alpha1 = alpha0, alpha3 = alpha2, alpha4 = alpha0, alpha6 = alpha2;
                        int alphaTotal = alpha0 + alpha2;
                        int possibleTotal = 2 * numeric_limits<uchar>::max();
                        
                        // cubes on the edge are two-dimensional: this ensures that their vertices will be shared between
                        // neighboring blocks, which share only one layer of points
                        if (middleZ) {
                            alphaTotal += (alpha4 = lineSrc[stackWidth].getEntryAlpha(y, nextHeightfieldHeightZ));
                            possibleTotal += numeric_limits<uchar>::max();
                            
                            alphaTotal += (alpha6 = lineSrc[stackWidth].getEntryAlpha(y + 1, nextHeightfieldHeightZ));
                            possibleTotal += numeric_limits<uchar>::max();
                        }
                        int alpha5 = alpha4, alpha7 = alpha6;
                        if (middleX) {
                            alphaTotal += (alpha1 = lineSrc[1].getEntryAlpha(y, nextHeightfieldHeightX));
                            possibleTotal += numeric_limits<uchar>::max();
                            
                            alphaTotal += (alpha3 = lineSrc[1].getEntryAlpha(y + 1, nextHeightfieldHeightX));
                            possibleTotal += numeric_limits<uchar>::max();
                                
                            if (middleZ) {
                                alphaTotal += (alpha5 = lineSrc[stackWidth + 1].getEntryAlpha(y, nextHeightfieldHeightXZ));
                                possibleTotal += numeric_limits<uchar>::max();
                                
                                alphaTotal += (alpha7 = lineSrc[stackWidth + 1].getEntryAlpha(y + 1, nextHeightfieldHeightXZ));
                                possibleTotal += numeric_limits<uchar>::max();
                            }
                        }
                        if (alphaTotal == 0 || alphaTotal == possibleTotal) {
                            continue; // no corners set/all corners set
                        }
                        // we first look for crossings with the heightfield corner vertices; these take priority
                        int crossingCount = 0;
                        if (y >= cornerMinimumY && y <= cornerMaximumY) {
                            // first look for set corners, which override any interpolated values
                            int crossedCorners = NO_CORNERS;
                            for (int i = 0; i < CORNER_COUNT; i++) {
                                if (!(corners & (1 << i))) {
                                    continue;
                                }
                                const EdgeCrossing& cornerCrossing = cornerCrossings[i];
                                if (cornerCrossing.point.y >= y && cornerCrossing.point.y < y + 1) {
                                    crossedCorners |= (1 << i);
                                }
                            }
                            switch (crossedCorners) {
                                case UPPER_LEFT_CORNER:
                                case LOWER_LEFT_CORNER | UPPER_LEFT_CORNER:
                                case LOWER_RIGHT_CORNER | LOWER_LEFT_CORNER | UPPER_LEFT_CORNER:
                                case UPPER_LEFT_CORNER | LOWER_RIGHT_CORNER:
                                    crossings[crossingCount++] = cornerCrossings[0];
                                    crossings[crossingCount - 1].point.y -= y;
                                    break;
                                
                                case UPPER_RIGHT_CORNER:
                                case UPPER_LEFT_CORNER | UPPER_RIGHT_CORNER:
                                case UPPER_RIGHT_CORNER | LOWER_LEFT_CORNER:
                                case LOWER_LEFT_CORNER | UPPER_LEFT_CORNER | UPPER_RIGHT_CORNER:
                                    crossings[crossingCount++] = cornerCrossings[1];
                                    crossings[crossingCount - 1].point.y -= y;
                                    break;
                                
                                case LOWER_LEFT_CORNER:
                                case LOWER_RIGHT_CORNER | LOWER_LEFT_CORNER:
                                case UPPER_RIGHT_CORNER | LOWER_RIGHT_CORNER | LOWER_LEFT_CORNER:
                                    crossings[crossingCount++] = cornerCrossings[2];
                                    crossings[crossingCount - 1].point.y -= y;
                                    break;
                                
                                case LOWER_RIGHT_CORNER:
                                case UPPER_RIGHT_CORNER | LOWER_RIGHT_CORNER:
                                case UPPER_LEFT_CORNER | UPPER_RIGHT_CORNER | LOWER_RIGHT_CORNER:
                                    crossings[crossingCount++] = cornerCrossings[3];
                                    crossings[crossingCount - 1].point.y -= y;
                                    break;
                                
                                case NO_CORNERS:
                                    for (int i = 0; i < CORNER_COUNT; i++) {
                                        if (!(corners & (1 << i))) {
                                            continue;
                                        }
                                        int nextIndex = NEXT_CORNERS[i];
                                        if (!(corners & (1 << nextIndex))) {
                                            continue;
                                        }
                                        const EdgeCrossing& cornerCrossing = cornerCrossings[i];
                                        const EdgeCrossing& nextCornerCrossing = cornerCrossings[nextIndex];
                                        float divisor = (nextCornerCrossing.point.y - cornerCrossing.point.y);
                                        if (divisor == 0.0f) {
                                            continue;
                                        }
                                        float t1 = (y - cornerCrossing.point.y) / divisor;
                                        float t2 = (y + 1 - cornerCrossing.point.y) / divisor;
                                        if (t1 >= 0.0f && t1 <= 1.0f) {
                                            crossings[crossingCount++].mix(cornerCrossing, nextCornerCrossing, t1);
                                            crossings[crossingCount - 1].point.y -= y;
                                        }
                                        if (t2 >= 0.0f && t2 <= 1.0f) {
                                            crossings[crossingCount++].mix(cornerCrossing, nextCornerCrossing, t2);
                                            crossings[crossingCount - 1].point.y -= y;
                                        }
                                    }
                                    break;
                            }
                        }
                        
                        // the terrifying conditional code that follows checks each cube edge for a crossing, gathering
                        // its properties (color, material, normal) if one is present; as before, boundary edges are excluded
                        if (crossingCount == 0) {
                            StackArray::Entry nextEntryY = getEntry(lineSrc, stackWidth, y + 1,
                                heightfieldHeight, cornerCrossings, 0);
                            if (middleX) {
                                StackArray::Entry nextEntryX = getEntry(lineSrc, stackWidth, y, nextHeightfieldHeightX,
                                    cornerCrossings, 1);
                                StackArray::Entry nextEntryXY = getEntry(lineSrc, stackWidth, y + 1, nextHeightfieldHeightX,
                                    cornerCrossings, 1);   
                                if (alpha0 != alpha1) {
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.point = glm::vec3(entry.getHermiteX(crossing.normal), 0.0f, 0.0f);
                                    crossing.setColorMaterial(alpha0 == 0 ? nextEntryX : entry);
                                }
                                if (alpha1 != alpha3) {
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.point = glm::vec3(1.0f, nextEntryX.getHermiteY(crossing.normal), 0.0f);
                                    crossing.setColorMaterial(alpha1 == 0 ? nextEntryXY : nextEntryX);
                                }
                                if (alpha2 != alpha3) {
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.point = glm::vec3(nextEntryY.getHermiteX(crossing.normal), 1.0f, 0.0f);
                                    crossing.setColorMaterial(alpha2 == 0 ? nextEntryXY : nextEntryY);
                                }
                                if (middleZ) {
                                    StackArray::Entry nextEntryZ = getEntry(lineSrc, stackWidth, y, nextHeightfieldHeightZ,
                                        cornerCrossings, 2);
                                    StackArray::Entry nextEntryXZ = getEntry(lineSrc, stackWidth, y, nextHeightfieldHeightXZ,
                                        cornerCrossings, 3);
                                    StackArray::Entry nextEntryXYZ = getEntry(lineSrc, stackWidth, y + 1,
                                        nextHeightfieldHeightXZ, cornerCrossings, 3);
                                    if (alpha1 != alpha5) {
                                        EdgeCrossing& crossing = crossings[crossingCount++];
                                        crossing.point = glm::vec3(1.0f, 0.0f, nextEntryX.getHermiteZ(crossing.normal));
                                        crossing.setColorMaterial(alpha1 == 0 ? nextEntryXZ : nextEntryX);
                                    }
                                    if (alpha3 != alpha7) {
                                        EdgeCrossing& crossing = crossings[crossingCount++];
                                        StackArray::Entry nextEntryXY = getEntry(lineSrc, stackWidth, y + 1,
                                            nextHeightfieldHeightX, cornerCrossings, 1);
                                        crossing.point = glm::vec3(1.0f, 1.0f, nextEntryXY.getHermiteZ(crossing.normal));
                                        crossing.setColorMaterial(alpha3 == 0 ? nextEntryXYZ : nextEntryXY);
                                    }
                                    if (alpha4 != alpha5) {
                                        EdgeCrossing& crossing = crossings[crossingCount++];
                                        crossing.point = glm::vec3(nextEntryZ.getHermiteX(crossing.normal), 0.0f, 1.0f);
                                        crossing.setColorMaterial(alpha4 == 0 ? nextEntryXZ : nextEntryZ);
                                    }
                                    if (alpha5 != alpha7) {
                                        EdgeCrossing& crossing = crossings[crossingCount++];
                                        StackArray::Entry nextEntryXZ = getEntry(lineSrc, stackWidth, y,
                                            nextHeightfieldHeightXZ, cornerCrossings, 3);
                                        crossing.point = glm::vec3(1.0f, nextEntryXZ.getHermiteY(crossing.normal), 1.0f);
                                        crossing.setColorMaterial(alpha5 == 0 ? nextEntryXYZ : nextEntryXZ);
                                    }
                                    if (alpha6 != alpha7) {
                                        EdgeCrossing& crossing = crossings[crossingCount++];
                                        StackArray::Entry nextEntryYZ = getEntry(lineSrc, stackWidth, y + 1,
                                            nextHeightfieldHeightZ, cornerCrossings, 2);
                                        crossing.point = glm::vec3(nextEntryYZ.getHermiteX(crossing.normal), 1.0f, 1.0f);
                                        crossing.setColorMaterial(alpha6 == 0 ? nextEntryXYZ : nextEntryYZ);
                                    }
                                }
                            }
                            if (alpha0 != alpha2) {
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.point = glm::vec3(0.0f, entry.getHermiteY(crossing.normal), 0.0f);
                                crossing.setColorMaterial(alpha0 == 0 ? nextEntryY : entry);
                            }
                            if (middleZ) {
                                StackArray::Entry nextEntryZ = getEntry(lineSrc, stackWidth, y,
                                    nextHeightfieldHeightZ, cornerCrossings, 2);
                                StackArray::Entry nextEntryYZ = getEntry(lineSrc, stackWidth, y + 1,
                                    nextHeightfieldHeightZ, cornerCrossings, 2);
                                if (alpha0 != alpha4) {
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.point = glm::vec3(0.0f, 0.0f, entry.getHermiteZ(crossing.normal));
                                    crossing.setColorMaterial(alpha0 == 0 ? nextEntryZ : entry);
                                }
                                if (alpha2 != alpha6) {
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.point = glm::vec3(0.0f, 1.0f, nextEntryY.getHermiteZ(crossing.normal));
                                    crossing.setColorMaterial(alpha2 == 0 ? nextEntryYZ : nextEntryY);
                                }
                                if (alpha4 != alpha6) {
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.point = glm::vec3(0.0f, nextEntryZ.getHermiteY(crossing.normal), 1.0f);
                                    crossing.setColorMaterial(alpha4 == 0 ? nextEntryYZ : nextEntryZ);
                                }
                            }
                        }
                        // make sure we have valid crossings to include
                        int validCrossings = 0;
                        for (int i = 0; i < crossingCount; i++) {
                            if (qAlpha(crossings[i].color) != 0) {
                                validCrossings++;
                            }
                        }
                        NormalIndex index = { { -1, -1, -1, -1 } };
                        if (validCrossings != 0) {
                            index.indices[0] = index.indices[1] = index.indices[2] = index.indices[3] = vertices.size();
                            glm::vec3 center;
                            glm::vec3 normals[MAX_NORMALS_PER_VERTEX];
                            int normalCount = 0;
                            const float CREASE_COS_NORMAL = glm::cos(glm::radians(45.0f));
                            const int MAX_MATERIALS_PER_VERTEX = 4;
                            quint8 materials[] = { 0, 0, 0, 0 };
                            glm::vec4 materialWeights;
                            float totalWeight = 0.0f;
                            int red = 0, green = 0, blue = 0;
                            for (int i = 0; i < crossingCount; i++) {
                                const EdgeCrossing& crossing = crossings[i];
                                if (qAlpha(crossing.color) == 0) {
                                    continue;
                                }
                                center += crossing.point;
                                
                                int j = 0;
                                for (; j < normalCount; j++) {
                                    if (glm::dot(normals[j], crossing.normal) > CREASE_COS_NORMAL) {
                                        normals[j] = safeNormalize(normals[j] + crossing.normal);
                                        break;
                                    }
                                }
                                if (j == normalCount) {
                                    normals[normalCount++] = crossing.normal;
                                }
                                
                                red += qRed(crossing.color);
                                green += qGreen(crossing.color);
                                blue += qBlue(crossing.color);
                                
                                // when assigning a material, search for its presence and, if not found,
                                // place it in the first empty slot
                                if (crossing.material != 0) {
                                    for (j = 0; j < MAX_MATERIALS_PER_VERTEX; j++) {
                                        if (materials[j] == crossing.material) {
                                            materialWeights[j] += 1.0f;
                                            totalWeight += 1.0f;
                                            break;
                                            
                                        } else if (materials[j] == 0) {
                                            materials[j] = crossing.material;
                                            materialWeights[j] = 1.0f;
                                            totalWeight += 1.0f;
                                            break;
                                        }
                                    }
                                }
                            }
                            center /= validCrossings;
                            
                            // use a sequence of Givens rotations to perform a QR decomposition
                            // see http://www.cs.rice.edu/~jwarren/papers/techreport02408.pdf
                            glm::mat4 r(0.0f);
                            glm::vec4 bottom;
                            for (int i = 0; i < crossingCount; i++) {
                                const EdgeCrossing& crossing = crossings[i];
                                if (qAlpha(crossing.color) == 0) {
                                    continue;
                                }
                                bottom = glm::vec4(crossing.normal, glm::dot(crossing.normal, crossing.point - center));
                                
                                for (int j = 0; j < 4; j++) {
                                    float angle = glm::atan(-bottom[j], r[j][j]);
                                    float sina = glm::sin(angle);
                                    float cosa = glm::cos(angle);
                                    
                                    for (int k = 0; k < 4; k++) {
                                        float tmp = bottom[k];
                                        bottom[k] = sina * r[k][j] + cosa * tmp;
                                        r[k][j] = cosa * r[k][j] - sina * tmp;
                                    }
                                }
                            }
                            
                            // extract the submatrices, form ata
                            glm::mat3 a(r);
                            glm::vec3 b(r[3]);
                            glm::mat3 atrans = glm::transpose(a);
                            glm::mat3 ata = atrans * a;
                            
                            // find the eigenvalues and eigenvectors of ata
                            // (see http://en.wikipedia.org/wiki/Jacobi_eigenvalue_algorithm)
                            glm::mat3 d = ata;
                            glm::quat combinedRotation;
                            const int MAX_ITERATIONS = 20;
                            for (int i = 0; i < MAX_ITERATIONS; i++) {
                                glm::vec3 offDiagonals = glm::abs(glm::vec3(d[1][0], d[2][0], d[2][1]));
                                int largestIndex = (offDiagonals[0] > offDiagonals[1]) ?
                                    (offDiagonals[0] > offDiagonals[2] ? 0 : 2) : (offDiagonals[1] > offDiagonals[2] ? 1 : 2);
                                const float DESIRED_PRECISION = 0.00001f;
                                if (offDiagonals[largestIndex] < DESIRED_PRECISION) {
                                    break;
                                }
                                int largestJ = (largestIndex == 2) ? 1 : 0;
                                int largestI = (largestIndex == 0) ? 1 : 2; 
                                float sjj = d[largestJ][largestJ];
                                float sii = d[largestI][largestI];
                                float angle = glm::atan(2.0f * d[largestJ][largestI], sjj - sii) / 2.0f;
                                glm::quat rotation = glm::angleAxis(angle, largestIndex == 0 ? glm::vec3(0.0f, 0.0f, -1.0f) :
                                    (largestIndex == 1 ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(-1.0f, 0.0f, 0.0f)));
                                combinedRotation = glm::normalize(rotation * combinedRotation);
                                glm::mat3 matrix = glm::mat3_cast(combinedRotation);
                                d = matrix * ata * glm::transpose(matrix);
                            }
                            
                            // form the singular matrix from the eigenvalues
                            const float MIN_SINGULAR_THRESHOLD = 0.1f;
                            d[0][0] = (d[0][0] < MIN_SINGULAR_THRESHOLD) ? 0.0f : 1.0f / d[0][0];
                            d[1][1] = (d[1][1] < MIN_SINGULAR_THRESHOLD) ? 0.0f : 1.0f / d[1][1];
                            d[2][2] = (d[2][2] < MIN_SINGULAR_THRESHOLD) ? 0.0f : 1.0f / d[2][2];
                            
                            // compute the pseudo-inverse, ataplus, and use to find the minimizing solution
                            glm::mat3 u = glm::mat3_cast(combinedRotation);
                            glm::mat3 ataplus = glm::transpose(u) * d * u; 
                            glm::vec3 solution = (ataplus * atrans * b) + center;
                            
                            // make sure it doesn't fall beyond the cell boundaries
                            center = glm::clamp(solution, 0.0f, 1.0f);
                        
                            if (totalWeight > 0.0f) {
                                materialWeights *= (numeric_limits<quint8>::max() / totalWeight);
                            }
                            VoxelPoint point = { (glm::vec3(clampedX, y, clampedZ) + center) * step,
                                { (quint8)(red / validCrossings), (quint8)(green / validCrossings),
                                    (quint8)(blue / validCrossings) },
                                { (char)(normals[0].x * 127.0f), (char)(normals[0].y * 127.0f),
                                    (char)(normals[0].z * 127.0f) },
                                { materials[0], materials[1], materials[2], materials[3] },
                                { (quint8)materialWeights[0], (quint8)materialWeights[1], (quint8)materialWeights[2],
                                    (quint8)materialWeights[3] } };
                            
                            vertices.append(point);
                            for (int i = 1; i < normalCount; i++) {
                                index.indices[i] = vertices.size();
                                point.setNormal(normals[i]);
                                vertices.append(point);
                            }
                        }
                        
                        // the first x, y, and z are repeated for the boundary edge; past that, we consider generating
                        // quads for each edge that includes a transition, using indices of previously generated vertices
                        int reclampedX = qMin(clampedX, stackWidth - 1);
                        int reclampedZ = qMin(clampedZ, stackHeight - 1);
                        if (alpha0 != alpha1 && y > position && z > 0) {
                            const NormalIndex& index1 = lastIndexY;
                            const NormalIndex& index2 = lastIndicesZ[x].get(y - 1);
                            const NormalIndex& index3 = lastIndicesZ[x].get(y);
                            if (index.isValid() && index1.isValid() && index2.isValid() && index3.isValid()) {
                                quadIndices.insert(qRgb(reclampedX, y, reclampedZ), indices.size());
                                quadIndices.insert(qRgb(reclampedX, y - 1, reclampedZ), indices.size());
                                if (reclampedZ > 0) {
                                    quadIndices.insert(qRgb(reclampedX, y - 1, reclampedZ - 1), indices.size());
                                    quadIndices.insert(qRgb(reclampedX, y, reclampedZ - 1), indices.size());
                                }
                                glm::vec3 normal = getNormal(vertices, index, index1, index2, index3);
                                if (alpha0 == 0) { // quad faces negative x
                                    indices.append(index3.getClosestIndex(normal = -normal, vertices));
                                    indices.append(index2.getClosestIndex(normal, vertices));
                                    indices.append(index1.getClosestIndex(normal, vertices));
                                } else { // quad faces positive x
                                    indices.append(index1.getClosestIndex(normal, vertices));
                                    indices.append(index2.getClosestIndex(normal, vertices));
                                    indices.append(index3.getClosestIndex(normal, vertices));
                                }
                                indices.append(index.getClosestIndex(normal, vertices));
                            }
                        }
                        
                        if (alpha0 != alpha2 && x > 0 && z > 0) {
                            const NormalIndex& index1 = lastIndicesZ[x].get(y);
                            const NormalIndex& index2 = lastIndicesZ[x - 1].get(y);
                            const NormalIndex& index3 = lastIndicesX.get(y);
                            if (index.isValid() && index1.isValid() && index2.isValid() && index3.isValid()) {
                                quadIndices.insert(qRgb(reclampedX, y, reclampedZ), indices.size());
                                if (reclampedX > 0) {
                                    quadIndices.insert(qRgb(reclampedX - 1, y, reclampedZ), indices.size());
                                    if (reclampedZ > 0) {
                                        quadIndices.insert(qRgb(reclampedX - 1, y, reclampedZ - 1), indices.size());
                                    }
                                }
                                if (reclampedZ > 0) {
                                    quadIndices.insert(qRgb(reclampedX, y, reclampedZ - 1), indices.size());
                                }
                                glm::vec3 normal = getNormal(vertices, index, index3, index2, index1);
                                if (alpha0 == 0) { // quad faces negative y
                                    indices.append(index3.getClosestIndex(normal, vertices));
                                    indices.append(index2.getClosestIndex(normal, vertices));
                                    indices.append(index1.getClosestIndex(normal, vertices));
                                } else { // quad faces positive y
                                    indices.append(index1.getClosestIndex(normal = -normal, vertices));
                                    indices.append(index2.getClosestIndex(normal, vertices));
                                    indices.append(index3.getClosestIndex(normal, vertices));
                                }
                                indices.append(index.getClosestIndex(normal, vertices));
                            }
                        }
                        
                        if (alpha0 != alpha4 && x > 0 && y > position) {
                            const NormalIndex& index1 = lastIndexY;
                            const NormalIndex& index2 = lastIndicesX.get(y - 1);
                            const NormalIndex& index3 = lastIndicesX.get(y);
                            if (index.isValid() && index1.isValid() && index2.isValid() && index3.isValid()) {
                                quadIndices.insert(qRgb(reclampedX, y, reclampedZ), indices.size());
                                if (reclampedX > 0) {
                                    quadIndices.insert(qRgb(reclampedX - 1, y, reclampedZ), indices.size());
                                    quadIndices.insert(qRgb(reclampedX - 1, y - 1, reclampedZ), indices.size());
                                }
                                quadIndices.insert(qRgb(reclampedX, y - 1, reclampedZ), indices.size());
                                
                                glm::vec3 normal = getNormal(vertices, index, index1, index2, index3);
                                if (alpha0 == 0) { // quad faces negative z
                                    indices.append(index1.getClosestIndex(normal, vertices));
                                    indices.append(index2.getClosestIndex(normal, vertices));
                                    indices.append(index3.getClosestIndex(normal, vertices));
                                } else { // quad faces positive z
                                    indices.append(index3.getClosestIndex(normal = -normal, vertices));
                                    indices.append(index2.getClosestIndex(normal, vertices));
                                    indices.append(index1.getClosestIndex(normal, vertices));
                                }
                                indices.append(index.getClosestIndex(normal, vertices));
                            }
                        }
                        lastIndexY = index;
                        indicesX[y - position] = index;
                        indicesZ[x][y - position] = index;
                    }
                } else {
                    indicesX.clear();
                    indicesZ[x].clear();
                }
                if (x != 0) {
                    lineSrc++;
                    heightLineSrc++;
                }
                indicesX.swap(lastIndicesX);
            }
            if (z != 0) {
                src += stackWidth;
                heightSrc += width;
            }
            indicesZ.swap(lastIndicesZ);
            lastIndicesX.clear();
        }
        _voxels = new VoxelBuffer(vertices, indices, hermiteSegments, quadIndices, stackWidth, stackMaterials);
    }
    
    if (_voxels) {
        _voxels->render(translation, rotation, glm::vec3(scale.x, scale.x, scale.x), cursor);
    }
    
    HeightfieldBaseLayerBatch baseBatch;
    baseBatch.vertexBufferID = bufferPair.first.bufferId();
    baseBatch.indexBufferID = bufferPair.second.bufferId();
    baseBatch.translation = translation;
    baseBatch.rotation = rotation;
    baseBatch.scale = scale;
    baseBatch.vertexCount = vertexCount;
    baseBatch.indexCount = indexCount;
    baseBatch.heightTextureID = _heightTextureID;
    baseBatch.heightScale = glm::vec4(1.0f / width, 1.0f / height, (innerWidth - 1) / -2.0f, (innerHeight - 1) / -2.0f);
    baseBatch.colorTextureID = _colorTextureID;
    float widthMultiplier = 1.0f / (0.5f - 1.5f / width);
    float heightMultiplier = 1.0f / (0.5f - 1.5f / height);
    if (node->getColor()) {
        int colorWidth = node->getColor()->getWidth();
        int colorHeight = node->getColor()->getContents().size() / (colorWidth * DataBlock::COLOR_BYTES);
        baseBatch.colorScale = glm::vec2((0.5f - 0.5f / colorWidth) * widthMultiplier,
            (0.5f - 0.5f / colorHeight) * heightMultiplier);
    }
    Application::getInstance()->getMetavoxels()->addHeightfieldBaseBatch(baseBatch);
    
    if (!(cursor || _networkTextures.isEmpty())) {
        HeightfieldSplatBatch splatBatch;
        splatBatch.vertexBufferID = bufferPair.first.bufferId();
        splatBatch.indexBufferID = bufferPair.second.bufferId();
        splatBatch.translation = translation;
        splatBatch.rotation = rotation;
        splatBatch.scale = scale;
        splatBatch.vertexCount = vertexCount;
        splatBatch.indexCount = indexCount;
        splatBatch.heightTextureID = _heightTextureID;
        splatBatch.heightScale = glm::vec4(1.0f / width, 1.0f / height, 0.0f, 0.0f);
        splatBatch.materialTextureID = _materialTextureID;
        if (node->getMaterial()) {
            int materialWidth = node->getMaterial()->getWidth();
            int materialHeight = node->getMaterial()->getContents().size() / materialWidth;
            splatBatch.textureScale = glm::vec2((0.5f - 0.5f / materialWidth) * widthMultiplier,
                (0.5f - 0.5f / materialHeight) * heightMultiplier);
        }
        splatBatch.splatTextureOffset = glm::vec2(
            glm::dot(translation, rotation * glm::vec3(1.0f, 0.0f, 0.0f)) / scale.x,
            glm::dot(translation, rotation * glm::vec3(0.0f, 0.0f, 1.0f)) / scale.z);
        
        const QVector<SharedObjectPointer>& materials = node->getMaterial()->getMaterials();
        for (int i = 0; i < materials.size(); i += SPLAT_COUNT) {
            for (int j = 0; j < SPLAT_COUNT; j++) {
                int index = i + j;
                if (index < _networkTextures.size()) {
                    const NetworkTexturePointer& texture = _networkTextures.at(index);
                    if (texture) {
                        MaterialObject* material = static_cast<MaterialObject*>(materials.at(index).data());
                        splatBatch.splatTextureScalesS[j] = scale.x / material->getScaleS();
                        splatBatch.splatTextureScalesT[j] = scale.z / material->getScaleT();
                        splatBatch.splatTextureIDs[j] = texture->getID();
                        
                    } else {
                        splatBatch.splatTextureIDs[j] = 0;
                    }
                } else {
                    splatBatch.splatTextureIDs[j] = 0;
                }
            }
            splatBatch.materialIndex = i;
            Application::getInstance()->getMetavoxels()->addHeightfieldSplatBatch(splatBatch);
        }
    }
}

QHash<HeightfieldNodeRenderer::IntPair, HeightfieldNodeRenderer::BufferPair> HeightfieldNodeRenderer::_bufferPairs;

