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

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

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
    _scaleToFit(modelOverlay->_scaleToFit),
    _loadPriority(modelOverlay->_loadPriority)
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
        Transform transform = evalRenderTransform();
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

    if (isAnimatingSomething()) {
        if (!jointsMapped()) {
            mapAnimationJoints(_model->getJointNames());
        }
        animate();
    }

    // check to see if when we added our model to the scene they were ready, if they were not ready, then
    // fix them up in the scene
    render::ScenePointer scene = qApp->getMain3DScene();
    render::Transaction transaction;
    if (_model->needsFixupInScene()) {
        _model->removeFromScene(scene, transaction);
        _model->addToScene(scene, transaction);

        auto newRenderItemIDs{ _model->fetchRenderItemIDs() };
        transaction.updateItem<Overlay>(getRenderItemID(), [newRenderItemIDs](Overlay& data) {
            auto modelOverlay = static_cast<ModelOverlay*>(&data);
            modelOverlay->setSubRenderItemIDs(newRenderItemIDs);
        });
    }
    if (_visibleDirty) {
        _visibleDirty = false;
        _model->setVisibleInScene(getVisible(), scene);
    }
    if (_drawInFrontDirty) {
        _drawInFrontDirty = false;
        _model->setLayeredInFront(getDrawInFront(), scene);
    }
    if (_drawInHUDDirty) {
        _drawInHUDDirty = false;
        _model->setLayeredInHUD(getDrawHUDLayer(), scene);
    }
    scene->enqueueTransaction(transaction);
}

bool ModelOverlay::addToScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) {
    Volume3DOverlay::addToScene(overlay, scene, transaction);
    _model->addToScene(scene, transaction);
    return true;
}

void ModelOverlay::removeFromScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) {
    Volume3DOverlay::removeFromScene(overlay, scene, transaction);
    _model->removeFromScene(scene, transaction);
    transaction.updateItem<Overlay>(getRenderItemID(), [](Overlay& data) {
        auto modelOverlay = static_cast<ModelOverlay*>(&data);
        modelOverlay->clearSubRenderItemIDs();
    });
}

void ModelOverlay::setVisible(bool visible) {
    Overlay::setVisible(visible);
    _visibleDirty = true;
}

void ModelOverlay::setDrawInFront(bool drawInFront) {
    Base3DOverlay::setDrawInFront(drawInFront);
    _drawInFrontDirty = true;
}

void ModelOverlay::setDrawHUDLayer(bool drawHUDLayer) {
    Base3DOverlay::setDrawHUDLayer(drawHUDLayer);
    _drawInHUDDirty = true;
}

void ModelOverlay::setProperties(const QVariantMap& properties) {
    auto origPosition = getWorldPosition();
    auto origRotation = getWorldOrientation();
    auto origDimensions = getDimensions();
    auto origScale = getSNScale();

    Base3DOverlay::setProperties(properties);

    auto scale = properties["scale"];
    if (scale.isValid()) {
        setSNScale(vec3FromVariant(scale));
    }

    auto dimensions = properties["dimensions"];
    if (!dimensions.isValid()) {
        dimensions = properties["size"];
    }
    if (dimensions.isValid()) {
        _scaleToFit = true;
        setDimensions(vec3FromVariant(dimensions));
    } else if (scale.isValid()) {
        // if "scale" property is set but "dimentions" is not.
        // do NOT scale to fit.
        _scaleToFit = false;
    }

    if (origPosition != getWorldPosition() || origRotation != getWorldOrientation() || origDimensions != getDimensions() || origScale != getSNScale()) {
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

    auto animationSettings = properties["animationSettings"];
    if (animationSettings.canConvert(QVariant::Map)) {
        QVariantMap animationSettingsMap = animationSettings.toMap();

        auto animationURL = animationSettingsMap["url"];
        auto animationFPS = animationSettingsMap["fps"];
        auto animationCurrentFrame = animationSettingsMap["currentFrame"];
        auto animationFirstFrame = animationSettingsMap["firstFrame"];
        auto animationLastFrame = animationSettingsMap["lastFrame"];
        auto animationRunning = animationSettingsMap["running"];
        auto animationLoop = animationSettingsMap["loop"];
        auto animationHold = animationSettingsMap["hold"];
        auto animationAllowTranslation = animationSettingsMap["allowTranslation"];

        if (animationURL.canConvert(QVariant::Url)) {
            _animationURL = animationURL.toUrl();
        }
        if (animationFPS.isValid()) {
            _animationFPS = animationFPS.toFloat();
        }
        if (animationCurrentFrame.isValid()) {
            _animationCurrentFrame = animationCurrentFrame.toFloat();
        }
        if (animationFirstFrame.isValid()) {
            _animationFirstFrame = animationFirstFrame.toFloat();
        }
        if (animationLastFrame.isValid()) {
            _animationLastFrame = animationLastFrame.toFloat();
        }

        if (animationRunning.canConvert(QVariant::Bool)) {
            _animationRunning = animationRunning.toBool();
        }
        if (animationLoop.canConvert(QVariant::Bool)) {
            _animationLoop = animationLoop.toBool();
        }
        if (animationHold.canConvert(QVariant::Bool)) {
            _animationHold = animationHold.toBool();
        }
        if (animationAllowTranslation.canConvert(QVariant::Bool)) {
            _animationAllowTranslation = animationAllowTranslation.toBool();
        }

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

    // animation properties
    if (property == "animationSettings") {
        QVariantMap animationSettingsMap;

        animationSettingsMap["url"] = _animationURL;
        animationSettingsMap["fps"] = _animationFPS;
        animationSettingsMap["currentFrame"] = _animationCurrentFrame;
        animationSettingsMap["firstFrame"] = _animationFirstFrame;
        animationSettingsMap["lastFrame"] = _animationLastFrame;
        animationSettingsMap["running"] = _animationRunning;
        animationSettingsMap["loop"] = _animationLoop;
        animationSettingsMap["hold"]= _animationHold;
        animationSettingsMap["allowTranslation"] = _animationAllowTranslation;

        return animationSettingsMap;
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

Transform ModelOverlay::evalRenderTransform() {
    Transform transform = getTransform();
    transform.setScale(1.0f); // disable inherited scale
    return transform;
}

void ModelOverlay::locationChanged(bool tellPhysics) {
    Base3DOverlay::locationChanged(tellPhysics);

    // FIXME Start using the _renderTransform instead of calling for Transform and Dimensions from here, do the custom things needed in evalRenderTransform()
    if (_model && _model->isActive()) {
        _model->setRotation(getWorldOrientation());
        _model->setTranslation(getWorldPosition());
    }
}

QString ModelOverlay::getName() const {
    if (_name != "") {
        return QString("Overlay:") + getType() + ":" + _name;
    }
    return QString("Overlay:") + getType() + ":" + _url.toString();
}


void ModelOverlay::animate() {

    if (!_animation || !_animation->isLoaded() || !_model || !_model->isLoaded()) {
        return;
    }


    QVector<JointData> jointsData;

    const QVector<FBXAnimationFrame>&  frames = _animation->getFramesReference(); // NOTE: getFrames() is too heavy
    int frameCount = frames.size();
    if (frameCount <= 0) {
        return;
    }

    if (!_lastAnimated) {
        _lastAnimated = usecTimestampNow();
        return;
    }

    auto now = usecTimestampNow();
    auto interval = now - _lastAnimated;
    _lastAnimated = now;
    float deltaTime = (float)interval / (float)USECS_PER_SECOND;
    _animationCurrentFrame += (deltaTime * _animationFPS);

    int animationCurrentFrame = (int)(glm::floor(_animationCurrentFrame)) % frameCount;
    if (animationCurrentFrame < 0 || animationCurrentFrame > frameCount) {
        animationCurrentFrame = 0;
    }

    if (animationCurrentFrame == _lastKnownCurrentFrame) {
        return;
    }
    _lastKnownCurrentFrame = animationCurrentFrame;

    if (_jointMapping.size() != _model->getJointStateCount()) {
        return;
    }

    QStringList animationJointNames = _animation->getGeometry().getJointNames();
    auto& fbxJoints = _animation->getGeometry().joints;

    auto& originalFbxJoints = _model->getFBXGeometry().joints;
    auto& originalFbxIndices = _model->getFBXGeometry().jointIndices;

    const QVector<glm::quat>& rotations = frames[_lastKnownCurrentFrame].rotations;
    const QVector<glm::vec3>& translations = frames[_lastKnownCurrentFrame].translations;

    jointsData.resize(_jointMapping.size());
    for (int j = 0; j < _jointMapping.size(); j++) {
        int index = _jointMapping[j];

        if (index >= 0) {
            glm::mat4 translationMat;

            if (_animationAllowTranslation) {
                if (index < translations.size()) {
                    translationMat = glm::translate(translations[index]);
                }
            } else if (index < animationJointNames.size()) {
                QString jointName = fbxJoints[index].name;

                if (originalFbxIndices.contains(jointName)) {
                    // Making sure the joint names exist in the original model the animation is trying to apply onto. If they do, then remap and get its translation.
                    int remappedIndex = originalFbxIndices[jointName] - 1; // JointIndeces seem to always start from 1 and the found index is always 1 higher than actual.
                    translationMat = glm::translate(originalFbxJoints[remappedIndex].translation);
                }
            }
            glm::mat4 rotationMat;
            if (index < rotations.size()) {
                rotationMat = glm::mat4_cast(fbxJoints[index].preRotation * rotations[index] * fbxJoints[index].postRotation);
            } else {
                rotationMat = glm::mat4_cast(fbxJoints[index].preRotation * fbxJoints[index].postRotation);
            }

            glm::mat4 finalMat = (translationMat * fbxJoints[index].preTransform *
                rotationMat * fbxJoints[index].postTransform);
            auto& jointData = jointsData[j];
            jointData.translation = extractTranslation(finalMat);
            jointData.translationSet = true;
            jointData.rotation = glmExtractRotation(finalMat);
            jointData.rotationSet = true;
        }
    }
    // Set the data in the model
    copyAnimationJointDataToModel(jointsData);
}


void ModelOverlay::mapAnimationJoints(const QStringList& modelJointNames) {

    // if we don't have animation, or we're already joint mapped then bail early
    if (!hasAnimation() || jointsMapped()) {
        return;
    }

    if (!_animation || _animation->getURL() != _animationURL) {
        _animation = DependencyManager::get<AnimationCache>()->getAnimation(_animationURL);
    }

    if (_animation && _animation->isLoaded()) {
        QStringList animationJointNames = _animation->getJointNames();

        if (modelJointNames.size() > 0 && animationJointNames.size() > 0) {
            _jointMapping.resize(modelJointNames.size());
            for (int i = 0; i < modelJointNames.size(); i++) {
                _jointMapping[i] = animationJointNames.indexOf(modelJointNames[i]);
            }
            _jointMappingCompleted = true;
            _jointMappingURL = _animationURL;
        }
    }
}

void ModelOverlay::copyAnimationJointDataToModel(QVector<JointData> jointsData) {
    if (!_model || !_model->isLoaded()) {
        return;
    }

    // relay any inbound joint changes from scripts/animation/network to the model/rig
    for (int index = 0; index < jointsData.size(); ++index) {
        auto& jointData = jointsData[index];
        _model->setJointRotation(index, true, jointData.rotation, 1.0f);
        _model->setJointTranslation(index, true, jointData.translation, 1.0f);
    }
    _updateModel = true;
}

void ModelOverlay::clearSubRenderItemIDs() {
    _subRenderItemIDs.clear();
}

void ModelOverlay::setSubRenderItemIDs(const render::ItemIDs& ids) {
    _subRenderItemIDs = ids;
}

uint32_t ModelOverlay::fetchMetaSubItems(render::ItemIDs& subItems) const {
    if (_model) {
        auto metaSubItems = _subRenderItemIDs;
        subItems.insert(subItems.end(), metaSubItems.begin(), metaSubItems.end());
        return (uint32_t)metaSubItems.size();
    }
    return 0;
}
