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
    : _model(std::make_shared<Model>(std::make_shared<Rig>())),
      _modelTextures(QVariantMap())
{
    _model->init();
    _isLoaded = false;
}

ModelOverlay::ModelOverlay(const ModelOverlay* modelOverlay) :
    Volume3DOverlay(modelOverlay),
    _model(std::make_shared<Model>(std::make_shared<Rig>())),
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

bool ModelOverlay::addToScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    Volume3DOverlay::addToScene(overlay, scene, pendingChanges);
    _model->addToScene(scene, pendingChanges);
    return true;
}

void ModelOverlay::removeFromScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    Volume3DOverlay::removeFromScene(overlay, scene, pendingChanges);
    _model->removeFromScene(scene, pendingChanges);
}

void ModelOverlay::render(RenderArgs* args) {

    // check to see if when we added our model to the scene they were ready, if they were not ready, then
    // fix them up in the scene
    render::ScenePointer scene = qApp->getMain3DScene();
    render::PendingChanges pendingChanges;
    if (_model->needsFixupInScene()) {
        _model->removeFromScene(scene, pendingChanges);
        _model->addToScene(scene, pendingChanges);
    }
    scene->enqueuePendingChanges(pendingChanges);

    if (!_visible) {
        return;
    }
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
//        foreach(const QString& key, textureMap.keys()) {
//
//            QUrl newTextureURL = textureMap[key].toUrl();
//            qDebug() << "Updating texture named" << key << "to texture at URL" << newTextureURL;
//
//            QMetaObject::invokeMethod(_model.get(), "setTextureWithNameToURL", Qt::AutoConnection,
//                                      Q_ARG(const QString&, key),
//                                      Q_ARG(const QUrl&, newTextureURL));
//
//            _modelTextures[key] = newTextureURL;  // Keep local track of textures for getProperty()
//        }
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

    if (_model && _model->isActive()) {
        _model->setRotation(getRotation());
        _model->setTranslation(getPosition());
    }
}
