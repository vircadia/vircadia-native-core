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

void ModelOverlay::render() {
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
    
    if (properties.property("position").isValid()) {
        _updateModel = true;
    }
}