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

#include "ModelOverlay.h"
#include <Rig.h>

#include "Application.h"


QString const ModelOverlay::TYPE = "model";

ModelOverlay::ModelOverlay()
	: _smoothPositionTime(SMOOTH_TIME_POSITION),
	 _smoothPositionTimer(std::numeric_limits<float>::max()),
	 _smoothOrientationTime(SMOOTH_TIME_ORIENTATION),
	 _smoothOrientationTimer(std::numeric_limits<float>::max()),
	 _smoothPositionInitial(),
	 _smoothPositionTarget(),
	 _smoothOrientationInitial(),
	 _smoothOrientationTarget(), 
	 _model(std::make_shared<Model>(std::make_shared<Rig>(), nullptr, this)),
	 _modelTextures(QVariantMap())
{
    _model->init();
    _isLoaded = false;
}

ModelOverlay::ModelOverlay(const ModelOverlay* modelOverlay) :
    Volume3DOverlay(modelOverlay),
    _model(std::make_shared<Model>(std::make_shared<Rig>(), nullptr, this)),
    _modelTextures(QVariantMap()),
    _url(modelOverlay->_url),
    _updateModel(false)
{
    _model->init();
    if (_url.isValid()) {
        _updateModel = true;
        _isLoaded = false;
    }
}

void ModelOverlay::update(float deltatime) {
	if (_model && _model->isActive()) {
		_model->setRotation(getRotation());
		_model->setTranslation(getPosition());

        if (_smoothPositionTimer < _smoothPositionTime) {
            // Smooth the remote avatar movement.
            _smoothPositionTimer+= deltatime;
            if (_smoothPositionTimer < _smoothPositionTime) {
				_model->setTranslation(
                    lerp(_smoothPositionInitial, 
                        _smoothPositionTarget,
                        easeInOutQuad(glm::clamp(_smoothPositionTimer / _smoothPositionTime, 0.0f, 1.0f)))
                );
            }
		}

        if (_smoothOrientationTimer < _smoothOrientationTime) {
            // Smooth the remote avatar movement.
            _smoothOrientationTimer+= deltatime;
            if (_smoothOrientationTimer < _smoothOrientationTime) {
				_model->setRotation(
                    slerp(_smoothOrientationInitial, 
                        _smoothOrientationTarget,
                        easeInOutQuad(glm::clamp(_smoothOrientationTimer / _smoothOrientationTime, 0.0f, 1.0f)))
                );
            }
        }
	}

    if (_updateModel) {
        _updateModel = false;
        _model->setSnapModelToCenter(true);
        if (_scaleToFit) {
            _model->setScaleToFit(true, getScale() * getDimensions());
        } else {
            _model->setScale(getScale());
        }
        _model->setRotation(getRotation());
        _model->setTranslation(getPosition());
        _model->setURL(_url);
        _model->simulate(deltatime, true);
    } else {
        _model->simulate(deltatime);
    }
    _isLoaded = _model->isActive();
}

bool ModelOverlay::addToScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) {
    Volume3DOverlay::addToScene(overlay, scene, transaction);
    _model->addToScene(scene, transaction);
    return true;
}

void ModelOverlay::removeFromScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) {
    Volume3DOverlay::removeFromScene(overlay, scene, transaction);
    _model->removeFromScene(scene, transaction);
}

void ModelOverlay::render(RenderArgs* args) {

    // check to see if when we added our model to the scene they were ready, if they were not ready, then
    // fix them up in the scene
    render::ScenePointer scene = qApp->getMain3DScene();
    render::Transaction transaction;
    if (_model->needsFixupInScene()) {
        _model->removeFromScene(scene, transaction);
        _model->addToScene(scene, transaction);
    }

    _model->setVisibleInScene(_visible, scene);
    _model->setLayeredInFront(getDrawInFront(), scene);

    scene->enqueueTransaction(transaction);
}

void ModelOverlay::setProperties(const QVariantMap& properties) {
    auto origPosition = getPosition();
    auto origRotation = getRotation();
    auto origDimensions = getDimensions();
    auto origScale = getScale();

    Base3DOverlay::setProperties(properties);

    auto scale = properties["scale"];
    if (scale.isValid()) {
        setScale(vec3FromVariant(scale));
    }

    auto dimensions = properties["dimensions"];
    if (dimensions.isValid()) {
        _scaleToFit = true;
        setDimensions(vec3FromVariant(dimensions));
    } else if (scale.isValid()) {
        // if "scale" property is set but "dimentions" is not.
        // do NOT scale to fit.
        _scaleToFit = false;
    }

    if (origPosition != getPosition() || origRotation != getRotation() || origDimensions != getDimensions() || origScale != getScale()) {
        _updateModel = true;
    }

    auto urlValue = properties["url"];
    if (urlValue.isValid() && urlValue.canConvert<QString>()) {
        _url = urlValue.toString();
        _updateModel = true;
        _isLoaded = false;
    }

    auto texturesValue = properties["textures"];
    if (texturesValue.isValid() && texturesValue.canConvert(QVariant::Map)) {
        QVariantMap textureMap = texturesValue.toMap();
        QMetaObject::invokeMethod(_model.get(), "setTextures", Qt::AutoConnection,
                                  Q_ARG(const QVariantMap&, textureMap));
    }
}

QVariant ModelOverlay::getProperty(const QString& property) {
    if (property == "url") {
        return _url.toString();
    }
    if (property == "dimensions" || property == "size") {
        return vec3toVariant(getDimensions());
    }
    if (property == "scale") {
        return vec3toVariant(getScale());
    }
    if (property == "textures") {
        if (_modelTextures.size() > 0) {
            QVariantMap textures;
            foreach(const QString& key, _modelTextures.keys()) {
                textures[key] = _modelTextures[key].toString();
            }
            return textures;
        } else {
            return QVariant();
        }
    }

    return Volume3DOverlay::getProperty(property);
}

bool ModelOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                        float& distance, BoxFace& face, glm::vec3& surfaceNormal) {

    QString subMeshNameTemp;
    return _model->findRayIntersectionAgainstSubMeshes(origin, direction, distance, face, surfaceNormal, subMeshNameTemp);
}

bool ModelOverlay::findRayIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& direction,
                                        float& distance, BoxFace& face, glm::vec3& surfaceNormal, QString& extraInfo) {

    return _model->findRayIntersectionAgainstSubMeshes(origin, direction, distance, face, surfaceNormal, extraInfo);
}

ModelOverlay* ModelOverlay::createClone() const {
    return new ModelOverlay(this);
}

void ModelOverlay::locationChanged(bool tellPhysics) {
    Base3DOverlay::locationChanged(tellPhysics);

	if (!_model || !_model->isActive()) {
		// If it's not active, don't care about it.
		return;
	}

	// Whether or not there is an existing smoothing going on, just reset the smoothing timer and set the starting position as the avatar's current position, then smooth to the new position.
	_smoothPositionInitial = _model->getTranslation();
	_smoothPositionTarget = getPosition();
	_smoothPositionTimer = 0.0f;

	// Whether or not there is an existing smoothing going on, just reset the smoothing timer and set the starting position as the avatar's current position, then smooth to the new position.
	_smoothOrientationInitial = _model->getRotation();
	_smoothOrientationTarget = getRotation();
	_smoothOrientationTimer = 0.0f;
}