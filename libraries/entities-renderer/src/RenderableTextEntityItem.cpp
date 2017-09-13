//
//  RenderableTextEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableTextEntityItem.h"

#include <TextEntityItem.h>
#include <GeometryCache.h>
#include <PerfStat.h>
#include <Transform.h>
#include <TextEntityItem.h>
#include <TextRenderer3D.h>

#include "GLMHelpers.h"

using namespace render::entities;

static const int FIXED_FONT_POINT_SIZE = 40;

TextEntityRenderer::TextEntityRenderer(const EntityItemPointer& entity) :
    Parent(entity),
    _textRenderer(TextRenderer3D::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE / 2.0f)) {

}

TextEntityRenderer::~TextEntityRenderer() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_geometryID && geometryCache) {
        geometryCache->releaseID(_geometryID);
    }
}

bool TextEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (_text != entity->getText()) {
        return true;
    }

    if (_lineHeight != entity->getLineHeight()) {
        return true;
    }

    if (_textColor != toGlm(entity->getTextColorX())) {
        return true;
    }

    if (_backgroundColor != toGlm(entity->getBackgroundColorX())) {
        return true;
    }

    if (_dimensions != entity->getDimensions()) {
        return true;
    }

    if (_faceCamera != entity->getFaceCamera()) {
        return true;
    }
    return false;
}

void TextEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    _textColor = toGlm(entity->getTextColorX());
    _backgroundColor = toGlm(entity->getBackgroundColorX());
    _dimensions = entity->getDimensions();
    _faceCamera = entity->getFaceCamera();
    _lineHeight = entity->getLineHeight();
    _text = entity->getText();
}


void TextEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableTextEntityItem::render");
   
    static const float SLIGHTLY_BEHIND = -0.005f;
    float fadeRatio = _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f;
    bool transparent = fadeRatio < 1.0f;
    glm::vec4 textColor = glm::vec4(_textColor, fadeRatio);
    glm::vec4 backgroundColor = glm::vec4(_backgroundColor, fadeRatio);
    const glm::vec3& dimensions = _dimensions;
    
    // Render background
    glm::vec3 minCorner = glm::vec3(0.0f, -dimensions.y, SLIGHTLY_BEHIND);
    glm::vec3 maxCorner = glm::vec3(dimensions.x, 0.0f, SLIGHTLY_BEHIND);
    
    
    // Batch render calls
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;

    auto transformToTopLeft = _modelTransform;
    if (_faceCamera) {
        //rotate about vertical to face the camera
        glm::vec3 dPosition = args->getViewFrustum().getPosition() - _modelTransform.getTranslation();
        // If x and z are 0, atan(x, z) is undefined, so default to 0 degrees
        float yawRotation = dPosition.x == 0.0f && dPosition.z == 0.0f ? 0.0f : glm::atan(dPosition.x, dPosition.z);
        glm::quat orientation = glm::quat(glm::vec3(0.0f, yawRotation, 0.0f));
        transformToTopLeft.setRotation(orientation);
    }
    transformToTopLeft.postTranslate(glm::vec3(-0.5f, 0.5f, 0.0f)); // Go to the top left
    transformToTopLeft.setScale(1.0f); // Use a scale of one so that the text is not deformed
    
    batch.setModelTransform(transformToTopLeft);
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (!_geometryID) {
        _geometryID = geometryCache->allocateID();
    }
    geometryCache->bindSimpleProgram(batch, false, transparent, false, false, true);
    geometryCache->renderQuad(batch, minCorner, maxCorner, backgroundColor, _geometryID);
    
    float scale = _lineHeight / _textRenderer->getFontSize();
    transformToTopLeft.setScale(scale); // Scale to have the correct line height
    batch.setModelTransform(transformToTopLeft);
    
    float leftMargin = 0.1f * _lineHeight, topMargin = 0.1f * _lineHeight;
    glm::vec2 bounds = glm::vec2(dimensions.x - 2.0f * leftMargin,
                                 dimensions.y - 2.0f * topMargin);
    _textRenderer->draw(batch, leftMargin / scale, -topMargin / scale, _text, textColor, bounds / scale);
}
