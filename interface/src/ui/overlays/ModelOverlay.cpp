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
    : _model(std::make_shared<Model>(nullptr, this)),
      _modelTextures(QVariantMap())
{
    _model->init();
    _model->setLoadingPriority(_loadPriority);
    _isLoaded = false;
}

ModelOverlay::ModelOverlay(const ModelOverlay* modelOverlay) :
    Volume3DOverlay(modelOverlay),
    _model(std::make_shared<Model>(nullptr, this)),
    _modelTextures(QVariantMap()),
    _url(modelOverlay->_url),
    _updateModel(false),
    _loadPriority(modelOverlay->getLoadPriority())
{
    _model->init();
    _model->setLoadingPriority(_loadPriority);
    if (_url.isValid()) {
        _updateModel = true;
        _isLoaded = false;
    }
}

void ModelOverlay::update(float deltatime) {
    if (_updateModel) {
        _updateModel = false;
        _model->setSnapModelToCenter(true);
        Transform transform = getTransform();
#ifndef USE_SN_SCALE
        transform.setScale(1.0f); // disable inherited scale
#endif
        if (_scaleToFit) {
            _model->setScaleToFit(true, transform.getScale() * getDimensions());
        } else {
            _model->setScale(transform.getScale());
        }
        _model->setRotation(transform.getRotation());
        _model->setTranslation(transform.getTranslation());
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
    auto origScale = getSNScale();

    Base3DOverlay::setProperties(properties);

    auto scale = properties["scale"];
    if (scale.isValid()) {
        setSNScale(vec3FromVariant(scale));
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

    if (origPosition != getPosition() || origRotation != getRotation() || origDimensions != getDimensions() || origScale != getSNScale()) {
        _updateModel = true;
    }

    auto loadPriorityProperty = properties["loadPriority"];
    if (loadPriorityProperty.isValid()) {
        _loadPriority = loadPriorityProperty.toFloat();
        _model->setLoadingPriority(_loadPriority);
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

    // relative
    auto jointTranslationsValue = properties["jointTranslations"];
    if (jointTranslationsValue.canConvert(QVariant::List)) {
        const QVariantList& jointTranslations = jointTranslationsValue.toList();
        int translationCount = jointTranslations.size();
        int jointCount = _model->getJointStateCount();
        if (translationCount < jointCount) {
            jointCount = translationCount;
        }
        for (int i=0; i < jointCount; i++) {
            const auto& translationValue = jointTranslations[i];
            if (translationValue.isValid()) {
                _model->setJointTranslation(i, true, vec3FromVariant(translationValue), 1.0f);
            }
        }
        _updateModel = true;
    }

    // relative
    auto jointRotationsValue = properties["jointRotations"];
    if (jointRotationsValue.canConvert(QVariant::List)) {
        const QVariantList& jointRotations = jointRotationsValue.toList();
        int rotationCount = jointRotations.size();
        int jointCount = _model->getJointStateCount();
        if (rotationCount < jointCount) {
            jointCount = rotationCount;
        }
        for (int i=0; i < jointCount; i++) {
            const auto& rotationValue = jointRotations[i];
            if (rotationValue.isValid()) {
                _model->setJointRotation(i, true, quatFromVariant(rotationValue), 1.0f);
            }
        }
        _updateModel = true;
    }
}

template <typename vectorType, typename itemType>
vectorType ModelOverlay::mapJoints(mapFunction<itemType> function) const {
    vectorType result;
    if (_model && _model->isActive()) {
        const int jointCount = _model->getJointStateCount();
        result.reserve(jointCount);
        for (int i = 0; i < jointCount; i++) {
            result << function(i);
        }
    }
    return result;
}

QVariant ModelOverlay::getProperty(const QString& property) {
    if (property == "url") {
        return _url.toString();
    }
    if (property == "dimensions" || property == "size") {
        return vec3toVariant(getDimensions());
    }
    if (property == "scale") {
        return vec3toVariant(getSNScale());
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

    if (property == "jointNames") {
        if (_model && _model->isActive()) {
            // note: going through Rig because Model::getJointNames() (which proxies to FBXGeometry) was always empty
            const Rig* rig = &(_model->getRig());
            return mapJoints<QStringList, QString>([rig](int jointIndex) -> QString {
                return rig->nameOfJoint(jointIndex);
            });
        }
    }

    // relative
    if (property == "jointRotations") {
        return mapJoints<QVariantList, QVariant>(
            [this](int jointIndex) -> QVariant {
                glm::quat rotation;
                _model->getJointRotation(jointIndex, rotation);
                return quatToVariant(rotation);
            });
    }

    // relative
    if (property == "jointTranslations") {
        return mapJoints<QVariantList, QVariant>(
            [this](int jointIndex) -> QVariant {
                glm::vec3 translation;
                _model->getJointTranslation(jointIndex, translation);
                return vec3toVariant(translation);
            });
    }

    // absolute
    if (property == "jointOrientations") {
        return mapJoints<QVariantList, QVariant>(
            [this](int jointIndex) -> QVariant {
                glm::quat orientation;
                _model->getJointRotationInWorldFrame(jointIndex, orientation);
                return quatToVariant(orientation);
            });
    }

    // absolute
    if (property == "jointPositions") {
        return mapJoints<QVariantList, QVariant>(
            [this](int jointIndex) -> QVariant {
                glm::vec3 position;
                _model->getJointPositionInWorldFrame(jointIndex, position);
                return vec3toVariant(position);
            });
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

    // FIXME Start using the _renderTransform instead of calling for Transform and Dimensions from here, do the custom things needed in evalRenderTransform()
    if (_model && _model->isActive()) {
        _model->setRotation(getRotation());
        _model->setTranslation(getPosition());
    }
}

QString ModelOverlay::getName() const {
    if (_name != "") {
        return QString("Overlay:") + getType() + ":" + _name;
    }
    return QString("Overlay:") + getType() + ":" + _url.toString();
}
