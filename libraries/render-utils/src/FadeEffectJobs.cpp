//
//  FadeEffectJobs.cpp

//  Created by Olivier Prat on 07/07/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "FadeEffectJobs.h"
#include "render/Logging.h"
#include "render/TransitionStage.h"

#include <NumericalConstants.h>
#include <Interpolate.h>
#include <gpu/Context.h>

#include <QJsonArray>

#include <PathUtils.h>

#define FADE_MIN_SCALE  0.001
#define FADE_MAX_SCALE  10000.0
#define FADE_MAX_SPEED  50.f

inline float parameterToValuePow(float parameter, const double minValue, const double maxOverMinValue) {
    return (float)(minValue * pow(maxOverMinValue, double(parameter)));
}

inline float valueToParameterPow(float value, const double minValue, const double maxOverMinValue) {
    return (float)(log(double(value) / minValue) / log(maxOverMinValue));
}

void FadeEditJob::configure(const Config& config) {
    _isEditEnabled = config.editFade;
}

void FadeEditJob::run(const render::RenderContextPointer& renderContext, const FadeEditJob::Input& inputs) {
    auto scene = renderContext->_scene;

    if (_isEditEnabled) {
        static const std::string selectionName("TransitionEdit");
        auto scene = renderContext->_scene;
        if (!scene->isSelectionEmpty(selectionName)) {
            auto selection = scene->getSelection(selectionName);
            auto editedItem = selection.getItems().front();
            render::Transaction transaction;
            bool hasTransaction{ false };

            if (editedItem != _editedItem && render::Item::isValidID(_editedItem)) {
                // Remove transition from previously edited item as we've changed edited item
                hasTransaction = true;
                transaction.removeTransitionFromItem(_editedItem);
            }
            _editedItem = editedItem;

            if (render::Item::isValidID(_editedItem)) {
                static const render::Transition::Type categoryToTransition[FADE_CATEGORY_COUNT] = {
                    render::Transition::ELEMENT_ENTER_DOMAIN,
                    render::Transition::BUBBLE_ISECT_OWNER,
                    render::Transition::BUBBLE_ISECT_TRESPASSER,
                    render::Transition::USER_ENTER_DOMAIN,
                    render::Transition::AVATAR_CHANGE
                };

                auto transitionType = categoryToTransition[inputs.get1()];

                transaction.queryTransitionOnItem(_editedItem, [transitionType, scene](render::ItemID id, const render::Transition* transition) {
                    if (transition == nullptr || transition->isFinished || transition->eventType != transitionType) {
                        // Relaunch transition
                        render::Transaction transaction;
                        transaction.resetTransitionOnItem(id, transitionType);
                        scene->enqueueTransaction(transaction);
                    }
                });
                hasTransaction = true;
            }

            if (hasTransaction) {
                scene->enqueueTransaction(transaction);
            }
        } else if (render::Item::isValidID(_editedItem)) {
            // Remove transition from previously edited item as we've disabled fade edition
            render::Transaction transaction;
            transaction.removeTransitionFromItem(_editedItem);
            scene->enqueueTransaction(transaction);
            _editedItem = render::Item::INVALID_ITEM_ID;
        }
    }
    else if (render::Item::isValidID(_editedItem)) {
        // Remove transition from previously edited item as we've disabled fade edition
        render::Transaction transaction;
        transaction.removeTransitionFromItem(_editedItem);
        scene->enqueueTransaction(transaction);
        _editedItem = render::Item::INVALID_ITEM_ID;
    }
}

FadeConfig::FadeConfig() 
{
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].noiseSize = glm::vec3{ 0.75f, 0.75f, 0.75f };
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].noiseLevel = 1.f;
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].noiseSpeed = glm::vec3{ 0.0f, 0.0f, 0.0f };
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].timing = FadeConfig::LINEAR;
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].baseSize = glm::vec3{ 1.0f, 1.0f, 1.0f };
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].baseLevel = 0.f;
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].isInverted = false;
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].duration = 4.f;
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].edgeWidth = 0.1f;
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].edgeInnerColor = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 0.0f };
    events[FADE_ELEMENT_ENTER_LEAVE_DOMAIN].edgeOuterColor = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 1.0f };

    events[FADE_BUBBLE_ISECT_OWNER].noiseSize = glm::vec3{ 1.5f, 1.0f / 25.f, 0.5f };
    events[FADE_BUBBLE_ISECT_OWNER].noiseLevel = 0.37f;
    events[FADE_BUBBLE_ISECT_OWNER].noiseSpeed = glm::vec3{ 1.0f, 0.2f, 1.0f };
    events[FADE_BUBBLE_ISECT_OWNER].timing = FadeConfig::LINEAR;
    events[FADE_BUBBLE_ISECT_OWNER].baseSize = glm::vec3{ 2.0f, 2.0f, 2.0f };
    events[FADE_BUBBLE_ISECT_OWNER].baseLevel = 1.f;
    events[FADE_BUBBLE_ISECT_OWNER].isInverted = false;
    events[FADE_BUBBLE_ISECT_OWNER].duration = 4.f;
    events[FADE_BUBBLE_ISECT_OWNER].edgeWidth = 0.02f;
    events[FADE_BUBBLE_ISECT_OWNER].edgeInnerColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 1.0f };
    events[FADE_BUBBLE_ISECT_OWNER].edgeOuterColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 2.0f };

    events[FADE_BUBBLE_ISECT_TRESPASSER].noiseSize = glm::vec3{ 0.5f, 1.0f / 25.f, 0.5f };
    events[FADE_BUBBLE_ISECT_TRESPASSER].noiseLevel = 1.f;
    events[FADE_BUBBLE_ISECT_TRESPASSER].noiseSpeed = glm::vec3{ 1.0f, -5.f, 1.0f };
    events[FADE_BUBBLE_ISECT_TRESPASSER].timing = FadeConfig::LINEAR;
    events[FADE_BUBBLE_ISECT_TRESPASSER].baseSize = glm::vec3{ 2.0f, 2.0f, 2.0f };
    events[FADE_BUBBLE_ISECT_TRESPASSER].baseLevel = 0.f;
    events[FADE_BUBBLE_ISECT_TRESPASSER].isInverted = false;
    events[FADE_BUBBLE_ISECT_TRESPASSER].duration = 4.f;
    events[FADE_BUBBLE_ISECT_TRESPASSER].edgeWidth = 0.025f;
    events[FADE_BUBBLE_ISECT_TRESPASSER].edgeInnerColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 1.0f };
    events[FADE_BUBBLE_ISECT_TRESPASSER].edgeOuterColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 2.0f };

    events[FADE_USER_ENTER_LEAVE_DOMAIN].noiseSize = glm::vec3{ 10.f, 0.01f, 10.0f };
    events[FADE_USER_ENTER_LEAVE_DOMAIN].noiseLevel = 0.3f;
    events[FADE_USER_ENTER_LEAVE_DOMAIN].noiseSpeed = glm::vec3{ 0.0f, -5.0f, 0.0f };
    events[FADE_USER_ENTER_LEAVE_DOMAIN].timing = FadeConfig::LINEAR;
    events[FADE_USER_ENTER_LEAVE_DOMAIN].baseSize = glm::vec3{ 10000.f, 1.0f, 10000.0f };
    events[FADE_USER_ENTER_LEAVE_DOMAIN].baseLevel = 1.f;
    events[FADE_USER_ENTER_LEAVE_DOMAIN].isInverted = true;
    events[FADE_USER_ENTER_LEAVE_DOMAIN].duration = 2.f;
    events[FADE_USER_ENTER_LEAVE_DOMAIN].edgeWidth = 0.229f;
    events[FADE_USER_ENTER_LEAVE_DOMAIN].edgeInnerColor = glm::vec4{ 1.f, 0.63f, 0.13f, 0.5f };
    events[FADE_USER_ENTER_LEAVE_DOMAIN].edgeOuterColor = glm::vec4{ 1.f, 1.f, 1.f, 1.0f };

    events[FADE_AVATAR_CHANGE].noiseSize = glm::vec3{ 0.4f, 0.4f, 0.4f };
    events[FADE_AVATAR_CHANGE].noiseLevel = 1.f;
    events[FADE_AVATAR_CHANGE].noiseSpeed = glm::vec3{ 0.0f, 0.0f, 0.0f };
    events[FADE_AVATAR_CHANGE].timing = FadeConfig::LINEAR;
    events[FADE_AVATAR_CHANGE].baseSize = glm::vec3{ 0.4f, 0.4f, 0.4f };
    events[FADE_AVATAR_CHANGE].baseLevel = 1.f;
    events[FADE_AVATAR_CHANGE].isInverted = false;
    events[FADE_AVATAR_CHANGE].duration = 3.f;
    events[FADE_AVATAR_CHANGE].edgeWidth = 0.05f;
    events[FADE_AVATAR_CHANGE].edgeInnerColor = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
    events[FADE_AVATAR_CHANGE].edgeOuterColor = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
}

void FadeConfig::setEditedCategory(int value) {
    assert(value < FADE_CATEGORY_COUNT);
    editedCategory = std::min<int>(FADE_CATEGORY_COUNT, value);
    emit dirtyCategory();
    emit dirty();
}

void FadeConfig::setDuration(float value) {
    events[editedCategory].duration = value;
    emit dirty();
}

float FadeConfig::getDuration() const { 
    return events[editedCategory].duration;
}

void FadeConfig::setBaseSizeX(float value) {
    events[editedCategory].baseSize.x = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE/ FADE_MIN_SCALE);
    emit dirty();
}

float FadeConfig::getBaseSizeX() const { 
    return valueToParameterPow(events[editedCategory].baseSize.x, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeConfig::setBaseSizeY(float value) {
    events[editedCategory].baseSize.y = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeConfig::getBaseSizeY() const {
    return valueToParameterPow(events[editedCategory].baseSize.y, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeConfig::setBaseSizeZ(float value) {
    events[editedCategory].baseSize.z = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeConfig::getBaseSizeZ() const {
    return valueToParameterPow(events[editedCategory].baseSize.z, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeConfig::setBaseLevel(float value) {
    events[editedCategory].baseLevel = value;
    emit dirty();
}

void FadeConfig::setInverted(bool value) {
    events[editedCategory].isInverted = value;
    emit dirty();
}

bool FadeConfig::isInverted() const { 
    return events[editedCategory].isInverted;
}

void FadeConfig::setNoiseSizeX(float value) {
    events[editedCategory].noiseSize.x = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeConfig::getNoiseSizeX() const {
    return valueToParameterPow(events[editedCategory].noiseSize.x, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeConfig::setNoiseSizeY(float value) {
    events[editedCategory].noiseSize.y = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeConfig::getNoiseSizeY() const {
    return valueToParameterPow(events[editedCategory].noiseSize.y, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeConfig::setNoiseSizeZ(float value) {
    events[editedCategory].noiseSize.z = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeConfig::getNoiseSizeZ() const {
    return valueToParameterPow(events[editedCategory].noiseSize.z, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeConfig::setNoiseLevel(float value) {
    events[editedCategory].noiseLevel = value;
    emit dirty();
}

void FadeConfig::setNoiseSpeedX(float value) {
    events[editedCategory].noiseSpeed.x = powf(value, 3.f)*FADE_MAX_SPEED;
    emit dirty();
}

float FadeConfig::getNoiseSpeedX() const {
    return powf(events[editedCategory].noiseSpeed.x / FADE_MAX_SPEED, 1.f / 3.f);
}

void FadeConfig::setNoiseSpeedY(float value) {
    events[editedCategory].noiseSpeed.y = powf(value, 3.f)*FADE_MAX_SPEED;
    emit dirty();
}

float FadeConfig::getNoiseSpeedY() const {
    return powf(events[editedCategory].noiseSpeed.y / FADE_MAX_SPEED, 1.f / 3.f);
}

void FadeConfig::setNoiseSpeedZ(float value) {
    events[editedCategory].noiseSpeed.z = powf(value, 3.f)*FADE_MAX_SPEED;
    emit dirty();
}

float FadeConfig::getNoiseSpeedZ() const {
    return powf(events[editedCategory].noiseSpeed.z / FADE_MAX_SPEED, 1.f / 3.f);
}

void FadeConfig::setEdgeWidth(float value) {
    events[editedCategory].edgeWidth = value * value;
    emit dirty();
}

float FadeConfig::getEdgeWidth() const { 
    return sqrtf(events[editedCategory].edgeWidth);
}

void FadeConfig::setEdgeInnerColor(const QColor& value) {
    events[editedCategory].edgeInnerColor.r = value.redF();
    events[editedCategory].edgeInnerColor.g = value.greenF();
    events[editedCategory].edgeInnerColor.b = value.blueF();
    emit dirty();
}

QColor FadeConfig::getEdgeInnerColor() const {
    QColor color;
    color.setRedF(events[editedCategory].edgeInnerColor.r);
    color.setGreenF(events[editedCategory].edgeInnerColor.g);
    color.setBlueF(events[editedCategory].edgeInnerColor.b);
    color.setAlphaF(1.0f);
    return color;
}

void FadeConfig::setEdgeInnerIntensity(float value) {
    events[editedCategory].edgeInnerColor.a = value;
    emit dirty();
}

void FadeConfig::setEdgeOuterColor(const QColor& value) {
    events[editedCategory].edgeOuterColor.r = value.redF();
    events[editedCategory].edgeOuterColor.g = value.greenF();
    events[editedCategory].edgeOuterColor.b = value.blueF();
    emit dirty();
}

QColor FadeConfig::getEdgeOuterColor() const {
    QColor color;
    color.setRedF(events[editedCategory].edgeOuterColor.r);
    color.setGreenF(events[editedCategory].edgeOuterColor.g);
    color.setBlueF(events[editedCategory].edgeOuterColor.b);
    color.setAlphaF(1.0f);
    return color;
}

void FadeConfig::setEdgeOuterIntensity(float value) {
    events[editedCategory].edgeOuterColor.a = value;
    emit dirty();
}

void FadeConfig::setTiming(int value) {
    assert(value < TIMING_COUNT);
    events[editedCategory].timing = value;
    emit dirty();
}

QString FadeConfig::eventNames[FADE_CATEGORY_COUNT] = {
    "element_enter_leave_domain",
    "bubble_isect_owner",
    "bubble_isect_trespasser",
    "user_enter_leave_domain",
    "avatar_change",
};

void FadeConfig::save(const QString& configFilePath) const {
    assert(editedCategory < FADE_CATEGORY_COUNT);
    QJsonObject lProperties;
    QFile file(configFilePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Fade event configuration file " << configFilePath << " cannot be opened";
    }
    else {
        const auto& event = events[editedCategory];

        lProperties["edgeInnerColor"] = QJsonArray{ event.edgeInnerColor.r, event.edgeInnerColor.g, event.edgeInnerColor.b, event.edgeInnerColor.a };
        lProperties["edgeOuterColor"] = QJsonArray{ event.edgeOuterColor.r, event.edgeOuterColor.g, event.edgeOuterColor.b, event.edgeOuterColor.a };
        lProperties["noiseSize"] = QJsonArray{ event.noiseSize.x, event.noiseSize.y, event.noiseSize.z };
        lProperties["noiseSpeed"] = QJsonArray{ event.noiseSpeed.x, event.noiseSpeed.y, event.noiseSpeed.z };
        lProperties["baseSize"] = QJsonArray{ event.baseSize.x, event.baseSize.y, event.baseSize.z };
        lProperties["noiseLevel"] = event.noiseLevel;
        lProperties["baseLevel"] = event.baseLevel;
        lProperties["duration"] = event.duration;
        lProperties["edgeWidth"] = event.edgeWidth;
        lProperties["timing"] = event.timing;
        lProperties["isInverted"] = event.isInverted;

        file.write( QJsonDocument(lProperties).toJson() );
        file.close();
    }
}

void FadeConfig::load(const QString& configFilePath) {
    QFile file(configFilePath);
    if (!file.exists()) {
        qWarning() << "Fade event configuration file " << configFilePath << " does not exist";
    }
    else if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Fade event configuration file " << configFilePath << " cannot be opened";
    }
    else {
        QString fileData = file.readAll();
        file.close();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(fileData.toUtf8(), &error);
        if (error.error == error.NoError) {
            QJsonObject jsonObject = doc.object();
            QJsonValue value;
            auto& event = events[editedCategory];

            qCDebug(renderlogging) << "Fade event configuration file" << configFilePath << "loaded";

            value = jsonObject["edgeInnerColor"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 4) {
                    qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'edgeInnerColor' field. Expected array of size 4";
                }
                else {
                    event.edgeInnerColor.r = (float)data.at(0).toDouble();
                    event.edgeInnerColor.g = (float)data.at(1).toDouble();
                    event.edgeInnerColor.b = (float)data.at(2).toDouble();
                    event.edgeInnerColor.a = (float)data.at(3).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'edgeInnerColor' field. Expected array of size 4";
            }

            value = jsonObject["edgeOuterColor"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 4) {
                    qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'edgeOuterColor' field. Expected array of size 4";
                }
                else {
                    event.edgeOuterColor.r = (float)data.at(0).toDouble();
                    event.edgeOuterColor.g = (float)data.at(1).toDouble();
                    event.edgeOuterColor.b = (float)data.at(2).toDouble();
                    event.edgeOuterColor.a = (float)data.at(3).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'edgeOuterColor' field. Expected array of size 4";
            }

            value = jsonObject["noiseSize"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 3) {
                    qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'noiseSize' field. Expected array of size 3";
                }
                else {
                    event.noiseSize.x = (float)data.at(0).toDouble();
                    event.noiseSize.y = (float)data.at(1).toDouble();
                    event.noiseSize.z = (float)data.at(2).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'noiseSize' field. Expected array of size 3";
            }

            value = jsonObject["noiseSpeed"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 3) {
                    qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'noiseSpeed' field. Expected array of size 3";
                }
                else {
                    event.noiseSpeed.x = (float)data.at(0).toDouble();
                    event.noiseSpeed.y = (float)data.at(1).toDouble();
                    event.noiseSpeed.z = (float)data.at(2).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'noiseSpeed' field. Expected array of size 3";
            }

            value = jsonObject["baseSize"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 3) {
                    qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'baseSize' field. Expected array of size 3";
                }
                else {
                    event.baseSize.x = (float)data.at(0).toDouble();
                    event.baseSize.y = (float)data.at(1).toDouble();
                    event.baseSize.z = (float)data.at(2).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'baseSize' field. Expected array of size 3";
            }

            value = jsonObject["noiseLevel"];
            if (value.isDouble()) {
                event.noiseLevel = (float)value.toDouble();
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'noiseLevel' field. Expected float value";
            }

            value = jsonObject["baseLevel"];
            if (value.isDouble()) {
                event.baseLevel = (float)value.toDouble();
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'baseLevel' field. Expected float value";
            }

            value = jsonObject["duration"];
            if (value.isDouble()) {
                event.duration = (float)value.toDouble();
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'duration' field. Expected float value";
            }

            value = jsonObject["edgeWidth"];
            if (value.isDouble()) {
                event.edgeWidth = std::min(1.f, std::max(0.f, (float)value.toDouble()));
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'edgeWidth' field. Expected float value";
            }

            value = jsonObject["timing"];
            if (value.isDouble()) {
                event.timing = std::max(0, std::min(TIMING_COUNT - 1, value.toInt()));
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'timing' field. Expected integer value";
            }

            value = jsonObject["isInverted"];
            if (value.isBool()) {
                event.isInverted = value.toBool();
            }
            else {
                qWarning() << "Fade event configuration file " << configFilePath << " contains an invalid 'isInverted' field. Expected boolean value";
            }

            emit dirty();
        }
        else {
            qWarning() << "Fade event configuration file" << configFilePath << "failed to load:" <<
                error.errorString() << "at offset" << error.offset;
        }
    }
}

FadeJob::FadeJob() {
    _previousTime = usecTimestampNow();
}

void FadeJob::configure(const Config& config) {
    auto& configurations = _configurations.edit();

    for (auto i = 0; i < FADE_CATEGORY_COUNT; i++) {
        auto& eventParameters = configurations.parameters[i];
        const auto& eventConfig = config.events[i];

        eventParameters._baseLevel = eventConfig.baseLevel;
        eventParameters._noiseInvSizeAndLevel.x = 1.f / eventConfig.noiseSize.x;
        eventParameters._noiseInvSizeAndLevel.y = 1.f / eventConfig.noiseSize.y;
        eventParameters._noiseInvSizeAndLevel.z = 1.f / eventConfig.noiseSize.z;
        eventParameters._noiseInvSizeAndLevel.w = eventConfig.noiseLevel;
        eventParameters._isInverted = eventConfig.isInverted & 1;
        eventParameters._edgeWidthInvWidth.x = eventConfig.edgeWidth;
        eventParameters._edgeWidthInvWidth.y = 1.f / eventParameters._edgeWidthInvWidth.x;
        eventParameters._innerEdgeColor = eventConfig.edgeInnerColor;
        eventParameters._outerEdgeColor = eventConfig.edgeOuterColor;
        _thresholdScale[i] = 1.f + (eventParameters._edgeWidthInvWidth.x + std::max(0.f, (eventConfig.noiseLevel + eventConfig.baseLevel)*0.5f - 0.5f));
    }
}

void FadeJob::run(const render::RenderContextPointer& renderContext, FadeJob::Output& output) {
    Config* jobConfig = static_cast<Config*>(renderContext->jobConfig.get());
    auto scene = renderContext->args->_scene;
    auto transitionStage = scene->getStage<render::TransitionStage>(render::TransitionStage::getName());
    uint64_t now = usecTimestampNow();
    const double deltaTime = (int64_t(now) - int64_t(_previousTime)) / double(USECS_PER_SECOND);
    render::Transaction transaction;
    bool isFirstItem = true;
    bool hasTransaction = false;
    
    output = (FadeCategory) jobConfig->editedCategory;

    // And now update fade effect
    for (auto transitionId : *transitionStage) {
        auto& state = transitionStage->editTransition(transitionId);
#ifdef DEBUG
        auto& item = scene->getItem(state.itemId);
        assert(item.getTransitionId() == transitionId);
#endif
        if (update(renderContext->args, *jobConfig, scene, transaction, state, deltaTime)) {
            hasTransaction = true;
        }
        if (isFirstItem && (state.threshold != jobConfig->threshold)) {
            jobConfig->setProperty("threshold", state.threshold);
            isFirstItem = false;
        }
    }
    _previousTime = now;
    if (hasTransaction) {
        scene->enqueueTransaction(transaction);
    }
}

const FadeCategory FadeJob::transitionToCategory[render::Transition::TYPE_COUNT] = {
    FADE_ELEMENT_ENTER_LEAVE_DOMAIN,
    FADE_ELEMENT_ENTER_LEAVE_DOMAIN,
    FADE_BUBBLE_ISECT_OWNER,
    FADE_BUBBLE_ISECT_TRESPASSER,
    FADE_USER_ENTER_LEAVE_DOMAIN,
    FADE_USER_ENTER_LEAVE_DOMAIN,
    FADE_AVATAR_CHANGE
};

bool FadeJob::update(RenderArgs* args, const Config& config, const render::ScenePointer& scene, render::Transaction& transaction, render::Transition& transition, const double deltaTime) const {
    const auto fadeCategory = transitionToCategory[transition.eventType];
    auto& eventConfig = config.events[fadeCategory];
    auto item = scene->getItemSafe(transition.itemId);
    const double eventDuration = (double)eventConfig.duration;
    const FadeConfig::Timing timing = (FadeConfig::Timing) eventConfig.timing;

    if (item.exist()) {
        auto aabb = item.getBound(args);
        if (render::Item::isValidID(transition.boundItemId)) {
            auto boundItem = scene->getItemSafe(transition.boundItemId);
            if (boundItem.exist()) {
                aabb = boundItem.getBound(args);
            }
        }
        auto& dimensions = aabb.getDimensions();

        assert(timing < FadeConfig::TIMING_COUNT);

        transition.noiseOffset = aabb.calcCenter();
        transition.baseInvSize.x = 1.f / eventConfig.baseSize.x;
        transition.baseInvSize.y = 1.f / eventConfig.baseSize.y;
        transition.baseInvSize.z = 1.f / eventConfig.baseSize.z;

        switch (transition.eventType) {
        case render::Transition::ELEMENT_ENTER_DOMAIN:
        case render::Transition::ELEMENT_LEAVE_DOMAIN:
        {
            transition.threshold = computeElementEnterRatio(transition.time, eventDuration, timing);
            transition.baseOffset = transition.noiseOffset;
            transition.isFinished += (transition.threshold >= 1.f) & 1;
            if (transition.eventType == render::Transition::ELEMENT_ENTER_DOMAIN) {
                transition.threshold = 1.f - transition.threshold;
            }
        }
        break;

        case render::Transition::BUBBLE_ISECT_OWNER:
        {
            transition.threshold = 0.5f;
            transition.baseOffset = transition.noiseOffset;
        }
        break;

        case render::Transition::BUBBLE_ISECT_TRESPASSER:
        {
            transition.threshold = 0.5f;
            transition.baseOffset = transition.noiseOffset;
        }
        break;

        case render::Transition::USER_ENTER_DOMAIN:
        case render::Transition::USER_LEAVE_DOMAIN:
        {
            transition.threshold = computeElementEnterRatio(transition.time, eventDuration, timing);
            transition.baseOffset = transition.noiseOffset - dimensions.y / 2.f;
            transition.baseInvSize.y = 1.f / dimensions.y;
            transition.isFinished += (transition.threshold >= 1.f) & 1;
            if (transition.eventType == render::Transition::USER_LEAVE_DOMAIN) {
                transition.threshold = 1.f - transition.threshold;
            }
        }
        break;

        case render::Transition::AVATAR_CHANGE:
            break;

        default:
            assert(false);
        }
    }

    transition.noiseOffset += eventConfig.noiseSpeed * (float)transition.time;
    if (config.manualFade) {
        transition.threshold = config.manualThreshold;
    }
    transition.threshold = std::max(0.f, std::min(1.f, transition.threshold));
    transition.threshold = (transition.threshold - 0.5f)*_thresholdScale[fadeCategory] + 0.5f;
    transition.time += deltaTime;

    // If the transition is finished for more than a number of frames (here 1), garbage collect it.
    if (transition.isFinished > 1) {
        transaction.removeTransitionFromItem(transition.itemId);
        return true;
    }
    return false;
}

float FadeJob::computeElementEnterRatio(double time, const double period, FadeConfig::Timing timing) {
    assert(period > 0.0);
    float fadeAlpha = 1.0f;
    const double INV_FADE_PERIOD = 1.0 / period;
    double fraction = time * INV_FADE_PERIOD;
    fraction = std::max(fraction, 0.0);
    if (fraction < 1.0) {
        switch (timing) {
        default:
            fadeAlpha = (float)fraction;
            break;
        case FadeConfig::EASE_IN:
            fadeAlpha = (float)(fraction*fraction*fraction);
            break;
        case FadeConfig::EASE_OUT:
            fadeAlpha = 1.f - (float)fraction;
            fadeAlpha = 1.f- fadeAlpha*fadeAlpha*fadeAlpha;
            break;
        case FadeConfig::EASE_IN_OUT:
            fadeAlpha = (float)(fraction*fraction*fraction*(fraction*(fraction * 6 - 15) + 10));
            break;
        }
    }
    return fadeAlpha;
}
