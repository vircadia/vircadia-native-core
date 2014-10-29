//
//  ModelOverlay.cpp
//
//
//  Created by Clement on 6/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "../../Menu.h"

#include "ModelOverlay.h"

ModelOverlay::ModelOverlay()
    : _model(),
      _scale(1.0f),
      _updateModel(false)
{
    _model.init();
    _isLoaded = false;
}

void ModelOverlay::update(float deltatime) {
    if (_updateModel) {
        _updateModel = false;
        
        _model.setScaleToFit(true, _scale);
qDebug() << "setScaleToFit() scale: " << _scale;

        _model.setSnapModelToCenter(true);
        _model.setRotation(_rotation);
        _model.setTranslation(_position);
        _model.setURL(_url);
        _model.simulate(deltatime, true);
    } else {
        _model.simulate(deltatime);
    }
    _isLoaded = _model.isActive();
}

void ModelOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return;
    }
    
    if (_model.isActive()) {
        if (_model.isRenderable()) {
            float glowLevel = getGlowLevel();
            Glower* glower = NULL;
            if (glowLevel > 0.0f) {
                glower = new Glower(glowLevel);
            }
            _model.render(getAlpha());
            if (glower) {
                delete glower;
            }
        }
    }
}

void ModelOverlay::setProperties(const QScriptValue &properties) {
    Base3DOverlay::setProperties(properties);
    
    QScriptValue urlValue = properties.property("url");
    if (urlValue.isValid()) {
        _url = urlValue.toVariant().toString();
        _updateModel = true;
        _isLoaded = false;
    }
    
    QScriptValue scaleValue = properties.property("scale");
    if (scaleValue.isValid()) {
        _scale = scaleValue.toVariant().toFloat();
qDebug() << "scale: " << _scale;
        _updateModel = true;
    }
    
    QScriptValue rotationValue = properties.property("rotation");
    if (rotationValue.isValid()) {
        QScriptValue x = rotationValue.property("x");
        QScriptValue y = rotationValue.property("y");
        QScriptValue z = rotationValue.property("z");
        QScriptValue w = rotationValue.property("w");
        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            _rotation.x = x.toVariant().toFloat();
            _rotation.y = y.toVariant().toFloat();
            _rotation.z = z.toVariant().toFloat();
            _rotation.w = w.toVariant().toFloat();
        }
        _updateModel = true;
    }

    QScriptValue dimensionsValue = properties.property("dimensions");
    if (dimensionsValue.isValid()) {
        QScriptValue x = dimensionsValue.property("x");
        QScriptValue y = dimensionsValue.property("y");
        QScriptValue z = dimensionsValue.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 dimensions;
            dimensions.x = x.toVariant().toFloat();
            dimensions.y = y.toVariant().toFloat();
            dimensions.z = z.toVariant().toFloat();
            _model.setScaleToFit(true, dimensions);
        }
        _updateModel = true;
    }
    
    QScriptValue texturesValue = properties.property("textures");
    if (texturesValue.isValid()) {
        QVariantMap textureMap = texturesValue.toVariant().toMap();
        foreach(const QString& key, textureMap.keys()) {
            
            QUrl newTextureURL = textureMap[key].toUrl();
            qDebug() << "Updating texture named" << key << "to texture at URL" << newTextureURL;
            
            QMetaObject::invokeMethod(&_model, "setTextureWithNameToURL", Qt::AutoConnection,
                                      Q_ARG(const QString&, key),
                                      Q_ARG(const QUrl&, newTextureURL));
        }
    }

    if (properties.property("position").isValid()) {
        _updateModel = true;
    }
}

bool ModelOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face) const {

    qDebug() << "ModelOverlay::findRayIntersection() calling _model.findRayIntersectionAgainstSubMeshes()...";
    return _model.findRayIntersectionAgainstSubMeshes(origin, direction, distance, face);    
    
    /*
    // if our model isn't active, we can't ray pick yet...
    if (!_model.isActive()) {
        return false;
    }

    // extents is the entity relative, scaled, centered extents of the entity
    glm::vec3 position = getPosition();
    glm::mat4 rotation = glm::mat4_cast(getRotation());
    glm::mat4 translation = glm::translate(position);
    glm::mat4 entityToWorldMatrix = translation * rotation;
    glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

    Extents modelExtents = _model.getMeshExtents(); // NOTE: unrotated
    
    glm::vec3 dimensions = modelExtents.maximum - modelExtents.minimum;
    glm::vec3 corner = dimensions * -0.5f; // since we're going to do the ray picking in the overlay frame of reference
    AABox overlayFrameBox(corner, dimensions);

    glm::vec3 overlayFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 overlayFrameDirection = glm::vec3(worldToEntityMatrix * glm::vec4(direction, 0.0f));

    // we can use the AABox's ray intersection by mapping our origin and direction into the overlays frame
    // and testing intersection there.
    if (overlayFrameBox.findRayIntersection(overlayFrameOrigin, overlayFrameDirection, distance, face)) {
        return true;
    }

    return false;
    */
}
