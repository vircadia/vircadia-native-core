#include "FadeEffect.h"
#include "TextureCache.h"

#include <PathUtils.h>
#include <NumericalConstants.h>
#include <Interpolate.h>
#include <gpu/Context.h>

#include <QJsonArray>

#define FADE_MIN_SCALE  0.001
#define FADE_MAX_SCALE  10000.0
#define FADE_MAX_SPEED  50.f

inline float parameterToValuePow(float parameter, const double minValue, const double maxOverMinValue) {
    return (float)(minValue * pow(maxOverMinValue, double(parameter)));
}

inline float valueToParameterPow(float value, const double minValue, const double maxOverMinValue) {
    return (float)(log(double(value) / minValue) / log(maxOverMinValue));
}

void FadeSwitchJob::configure(const Config& config) {
    _parameters->_isEditEnabled = config.editFade;
}

void FadeSwitchJob::run(const render::RenderContextPointer& renderContext, const Input& input, Output& output) {
    auto& normalOutputs = output.edit0().edit0();
    auto& fadeOutputs = output.edit1();

    // Only shapes are affected by fade at this time.
    normalOutputs[RenderFetchCullSortTask::LIGHT].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::LIGHT].get<render::ItemBounds>();
    normalOutputs[RenderFetchCullSortTask::META].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::META].get<render::ItemBounds>();
    normalOutputs[RenderFetchCullSortTask::OVERLAY_OPAQUE_SHAPE].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::OVERLAY_OPAQUE_SHAPE].get<render::ItemBounds>();
    normalOutputs[RenderFetchCullSortTask::OVERLAY_TRANSPARENT_SHAPE].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::OVERLAY_TRANSPARENT_SHAPE].get<render::ItemBounds>();
    normalOutputs[RenderFetchCullSortTask::BACKGROUND].edit<render::ItemBounds>() = input.get0()[RenderFetchCullSortTask::BACKGROUND].get<render::ItemBounds>();
    output.edit0().edit1() = input.get1();

    // Find the nearest item that intersects the view direction
    const render::Item* editedItem = nullptr;
    if (_parameters->_isEditEnabled) {
        float nearestOpaqueDistance = std::numeric_limits<float>::max();
        float nearestTransparentDistance = std::numeric_limits<float>::max();
        const render::Item* nearestItem;

        editedItem = findNearestItem(renderContext, input.get0()[RenderFetchCullSortTask::OPAQUE_SHAPE], nearestOpaqueDistance);
        nearestItem = findNearestItem(renderContext, input.get0()[RenderFetchCullSortTask::TRANSPARENT_SHAPE], nearestTransparentDistance);
        if (nearestTransparentDistance < nearestOpaqueDistance) {
            editedItem = nearestItem;
        }

        if (editedItem) {
            output.edit2() = editedItem->getBound();
        }
    }

    // Now, distribute items that need to be faded accross both outputs
    distribute(renderContext, input.get0()[RenderFetchCullSortTask::OPAQUE_SHAPE], normalOutputs[RenderFetchCullSortTask::OPAQUE_SHAPE], fadeOutputs[OPAQUE_SHAPE], editedItem);
    distribute(renderContext, input.get0()[RenderFetchCullSortTask::TRANSPARENT_SHAPE], normalOutputs[RenderFetchCullSortTask::TRANSPARENT_SHAPE], fadeOutputs[TRANSPARENT_SHAPE], editedItem);
}

const render::Item* FadeSwitchJob::findNearestItem(const render::RenderContextPointer& renderContext, const render::Varying& input, float& minIsectDistance) const {
    const glm::vec3 rayOrigin = renderContext->args->getViewFrustum().getPosition();
    const glm::vec3 rayDirection = renderContext->args->getViewFrustum().getDirection();
    const auto& inputItems = input.get<render::ItemBounds>();
    auto& scene = renderContext->_scene;
    BoxFace face;
    glm::vec3 normal;
    float isectDistance;
    const render::Item* nearestItem = nullptr;
    const float minDistance = 2.f;

    for (const auto& itemBound : inputItems) {
        if (!itemBound.bound.contains(rayOrigin) && itemBound.bound.findRayIntersection(rayOrigin, rayDirection, isectDistance, face, normal)) {
            if (isectDistance>minDistance && isectDistance < minIsectDistance) {
                auto& item = scene->getItem(itemBound.id);

                if (item.getKey().isShape() && !item.getKey().isMeta()) {
                    nearestItem = &item;
                    minIsectDistance = isectDistance;
                }
            }
        }
    }
    return nearestItem;
}

void FadeSwitchJob::distribute(const render::RenderContextPointer& renderContext, const render::Varying& input, 
    render::Varying& normalOutput, render::Varying& fadeOutput, const render::Item* editedItem) const {
    auto& scene = renderContext->_scene;
    assert(_parameters);
    const auto& inputItems = input.get<render::ItemBounds>();

    // Clear previous values
    normalOutput.edit<render::ItemBounds>().clear();
    fadeOutput.edit<render::ItemBounds>().clear();

    for (const auto&  itemBound : inputItems) {
        auto& item = scene->getItem(itemBound.id);

        if (!item.mustFade() && &item!=editedItem) {
            // No need to fade
            normalOutput.edit<render::ItemBounds>().emplace_back(itemBound);
        }
        else {
            fadeOutput.edit<render::ItemBounds>().emplace_back(itemBound);
        }
    }
}

FadeCommonParameters::FadeCommonParameters()
{
    _durations[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN] = 0.f;
    _durations[FadeJobConfig::BUBBLE_ISECT_OWNER] = 0.f;
    _durations[FadeJobConfig::BUBBLE_ISECT_TRESPASSER] = 0.f;
    _durations[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN] = 0.f;
    _durations[FadeJobConfig::AVATAR_CHANGE] = 0.f;
}

FadeJobConfig::FadeJobConfig() 
{
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN].noiseSize = glm::vec3{ 0.75f, 0.75f, 0.75f };
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN].noiseLevel = 1.f;
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN].noiseSpeed = glm::vec3{ 0.0f, 0.0f, 0.0f };
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN].timing = FadeJobConfig::LINEAR;
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN].baseSize = glm::vec3{ 1.0f, 1.0f, 1.0f };
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN].baseLevel = 0.f;
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN]._isInverted = false;
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN]._duration = 4.f;
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN].edgeWidth = 0.1f;
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN].edgeInnerColor = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 0.0f };
    events[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN].edgeOuterColor = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 1.0f };

    events[FadeJobConfig::BUBBLE_ISECT_OWNER].noiseSize = glm::vec3{ 1.5f, 1.0f/25.f, 0.5f };
    events[FadeJobConfig::BUBBLE_ISECT_OWNER].noiseLevel = 0.37f;
    events[FadeJobConfig::BUBBLE_ISECT_OWNER].noiseSpeed = glm::vec3{ 1.0f, 0.2f, 1.0f };
    events[FadeJobConfig::BUBBLE_ISECT_OWNER].timing = FadeJobConfig::LINEAR;
    events[FadeJobConfig::BUBBLE_ISECT_OWNER].baseSize = glm::vec3{ 2.0f, 2.0f, 2.0f };
    events[FadeJobConfig::BUBBLE_ISECT_OWNER].baseLevel = 1.f;
    events[FadeJobConfig::BUBBLE_ISECT_OWNER]._isInverted = false;
    events[FadeJobConfig::BUBBLE_ISECT_OWNER]._duration = 4.f;
    events[FadeJobConfig::BUBBLE_ISECT_OWNER].edgeWidth = 0.02f;
    events[FadeJobConfig::BUBBLE_ISECT_OWNER].edgeInnerColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 1.0f };
    events[FadeJobConfig::BUBBLE_ISECT_OWNER].edgeOuterColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 2.0f };

    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER].noiseSize = glm::vec3{ 0.5f, 1.0f / 25.f, 0.5f };
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER].noiseLevel = 1.f;
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER].noiseSpeed = glm::vec3{ 1.0f, 0.2f, 1.0f };
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER].timing = FadeJobConfig::LINEAR;
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER].baseSize = glm::vec3{ 2.0f, 2.0f, 2.0f };
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER].baseLevel = 0.f;
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER]._isInverted = false;
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER]._duration = 4.f;
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER].edgeWidth = 0.025f;
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER].edgeInnerColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 1.0f };
    events[FadeJobConfig::BUBBLE_ISECT_TRESPASSER].edgeOuterColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 2.0f };

    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN].noiseSize = glm::vec3{ 10.f, 0.01f, 10.0f };
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN].noiseLevel = 0.7f;
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN].noiseSpeed = glm::vec3{ 0.0f, -0.5f, 0.0f };
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN].timing = FadeJobConfig::LINEAR;
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN].baseSize = glm::vec3{ 10000.f, 1.0f, 10000.0f };
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN].baseLevel = 1.f;
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN]._isInverted = true;
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN]._duration = 5.f;
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN].edgeWidth = 0.229f;
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN].edgeInnerColor = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 0.25f };
    events[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN].edgeOuterColor = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 1.0f };

    events[FadeJobConfig::AVATAR_CHANGE].noiseSize = glm::vec3{ 0.4f, 0.4f, 0.4f };
    events[FadeJobConfig::AVATAR_CHANGE].noiseLevel = 1.f;
    events[FadeJobConfig::AVATAR_CHANGE].noiseSpeed = glm::vec3{ 0.0f, 0.0f, 0.0f };
    events[FadeJobConfig::AVATAR_CHANGE].timing = FadeJobConfig::LINEAR;
    events[FadeJobConfig::AVATAR_CHANGE].baseSize = glm::vec3{ 0.4f, 0.4f, 0.4f };
    events[FadeJobConfig::AVATAR_CHANGE].baseLevel = 1.f;
    events[FadeJobConfig::AVATAR_CHANGE]._isInverted = false;
    events[FadeJobConfig::AVATAR_CHANGE]._duration = 3.f;
    events[FadeJobConfig::AVATAR_CHANGE].edgeWidth = 0.05f;
    events[FadeJobConfig::AVATAR_CHANGE].edgeInnerColor = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
    events[FadeJobConfig::AVATAR_CHANGE].edgeOuterColor = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
}

void FadeJobConfig::setEditedCategory(int value) {
    assert(value < EVENT_CATEGORY_COUNT);
    editedCategory = std::min<int>(EVENT_CATEGORY_COUNT, value);
    emit dirtyCategory();
    emit dirty();
}

void FadeJobConfig::setDuration(float value) {
    events[editedCategory]._duration = value;
    emit dirty();
}

float FadeJobConfig::getDuration() const { 
    return events[editedCategory]._duration;
}

void FadeJobConfig::setBaseSizeX(float value) {
    events[editedCategory].baseSize.x = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE/ FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getBaseSizeX() const { 
    return valueToParameterPow(events[editedCategory].baseSize.x, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setBaseSizeY(float value) {
    events[editedCategory].baseSize.y = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getBaseSizeY() const {
    return valueToParameterPow(events[editedCategory].baseSize.y, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setBaseSizeZ(float value) {
    events[editedCategory].baseSize.z = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getBaseSizeZ() const {
    return valueToParameterPow(events[editedCategory].baseSize.z, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setBaseLevel(float value) {
    events[editedCategory].baseLevel = value;
    emit dirty();
}

void FadeJobConfig::setInverted(bool value) {
    events[editedCategory]._isInverted = value;
    emit dirty();
}

bool FadeJobConfig::isInverted() const { 
    return events[editedCategory]._isInverted;
}

void FadeJobConfig::setNoiseSizeX(float value) {
    events[editedCategory].noiseSize.x = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getNoiseSizeX() const {
    return valueToParameterPow(events[editedCategory].noiseSize.x, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setNoiseSizeY(float value) {
    events[editedCategory].noiseSize.y = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getNoiseSizeY() const {
    return valueToParameterPow(events[editedCategory].noiseSize.y, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setNoiseSizeZ(float value) {
    events[editedCategory].noiseSize.z = parameterToValuePow(value, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
    emit dirty();
}

float FadeJobConfig::getNoiseSizeZ() const {
    return valueToParameterPow(events[editedCategory].noiseSize.z, FADE_MIN_SCALE, FADE_MAX_SCALE / FADE_MIN_SCALE);
}

void FadeJobConfig::setNoiseLevel(float value) {
    events[editedCategory].noiseLevel = value;
    emit dirty();
}

void FadeJobConfig::setNoiseSpeedX(float value) {
    events[editedCategory].noiseSpeed.x = powf(value, 3.f)*FADE_MAX_SPEED;
    emit dirty();
}

float FadeJobConfig::getNoiseSpeedX() const {
    return powf(events[editedCategory].noiseSpeed.x / FADE_MAX_SPEED, 1.f / 3.f);
}

void FadeJobConfig::setNoiseSpeedY(float value) {
    events[editedCategory].noiseSpeed.y = powf(value, 3.f)*FADE_MAX_SPEED;
    emit dirty();
}

float FadeJobConfig::getNoiseSpeedY() const {
    return powf(events[editedCategory].noiseSpeed.y / FADE_MAX_SPEED, 1.f / 3.f);
}

void FadeJobConfig::setNoiseSpeedZ(float value) {
    events[editedCategory].noiseSpeed.z = powf(value, 3.f)*FADE_MAX_SPEED;
    emit dirty();
}

float FadeJobConfig::getNoiseSpeedZ() const {
    return powf(events[editedCategory].noiseSpeed.z / FADE_MAX_SPEED, 1.f / 3.f);
}

void FadeJobConfig::setEdgeWidth(float value) {
    events[editedCategory].edgeWidth = value * value;
    emit dirty();
}

float FadeJobConfig::getEdgeWidth() const { 
    return sqrtf(events[editedCategory].edgeWidth);
}

void FadeJobConfig::setEdgeInnerColorR(float value) {
    events[editedCategory].edgeInnerColor.r = value;
    emit dirty();
}

void FadeJobConfig::setEdgeInnerColorG(float value) {
    events[editedCategory].edgeInnerColor.g = value;
    emit dirty();
}

void FadeJobConfig::setEdgeInnerColorB(float value) {
    events[editedCategory].edgeInnerColor.b = value;
    emit dirty();
}

void FadeJobConfig::setEdgeInnerIntensity(float value) {
    events[editedCategory].edgeInnerColor.a = value;
    emit dirty();
}

void FadeJobConfig::setEdgeOuterColorR(float value) {
    events[editedCategory].edgeOuterColor.r = value;
    emit dirty();
}

void FadeJobConfig::setEdgeOuterColorG(float value) {
    events[editedCategory].edgeOuterColor.g = value;
    emit dirty();
}

void FadeJobConfig::setEdgeOuterColorB(float value) {
    events[editedCategory].edgeOuterColor.b = value;
    emit dirty();
}

void FadeJobConfig::setEdgeOuterIntensity(float value) {
    events[editedCategory].edgeOuterColor.a = value;
    emit dirty();
}

void FadeJobConfig::setTiming(int value) {
    assert(value < TIMING_COUNT);
    events[editedCategory].timing = value;
    emit dirty();
}

QString FadeJobConfig::eventNames[EVENT_CATEGORY_COUNT] = {
    "element_enter_leave_domain",
    "bubble_isect_owner",
    "bubble_isect_trespasser",
    "user_enter_leave_domain",
    "avatar_change",
};

void FadeJobConfig::save() const {
    assert(category < EVENT_CATEGORY_COUNT);
    QJsonObject lProperties;
    const QString configFile = "config/" + eventNames[editedCategory] + ".json";
    QUrl path(PathUtils::resourcesPath() + configFile);
    QFile file(path.toString());
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Fade event configuration file " << path << " cannot be opened";
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
        lProperties["duration"] = event._duration;
        lProperties["edgeWidth"] = event.edgeWidth;
        lProperties["timing"] = event.timing;
        lProperties["isInverted"] = event._isInverted;

        file.write( QJsonDocument(lProperties).toJson() );
        file.close();
    }
}

void FadeJobConfig::load() {
    const QString configFile = "config/" + eventNames[editedCategory] + ".json";

    QUrl path(PathUtils::resourcesPath() + configFile);
    QFile file(path.toString());
    if (!file.exists()) {
        qWarning() << "Fade event configuration file " << path << " does not exist";
    }
    else if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Fade event configuration file " << path << " cannot be opened";
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

            qCDebug(renderlogging) << "Fade event configuration file" << path << "loaded";

            value = jsonObject["edgeInnerColor"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 4) {
                    qWarning() << "Fade event configuration file " << path << " contains an invalid 'edgeInnerColor' field. Expected array of size 4";
                }
                else {
                    event.edgeInnerColor.r = (float)data.at(0).toDouble();
                    event.edgeInnerColor.g = (float)data.at(1).toDouble();
                    event.edgeInnerColor.b = (float)data.at(2).toDouble();
                    event.edgeInnerColor.a = (float)data.at(3).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'edgeInnerColor' field. Expected array of size 4";
            }

            value = jsonObject["edgeOuterColor"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 4) {
                    qWarning() << "Fade event configuration file " << path << " contains an invalid 'edgeOuterColor' field. Expected array of size 4";
                }
                else {
                    event.edgeOuterColor.r = (float)data.at(0).toDouble();
                    event.edgeOuterColor.g = (float)data.at(1).toDouble();
                    event.edgeOuterColor.b = (float)data.at(2).toDouble();
                    event.edgeOuterColor.a = (float)data.at(3).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'edgeOuterColor' field. Expected array of size 4";
            }

            value = jsonObject["noiseSize"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 3) {
                    qWarning() << "Fade event configuration file " << path << " contains an invalid 'noiseSize' field. Expected array of size 3";
                }
                else {
                    event.noiseSize.x = (float)data.at(0).toDouble();
                    event.noiseSize.y = (float)data.at(1).toDouble();
                    event.noiseSize.z = (float)data.at(2).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'noiseSize' field. Expected array of size 3";
            }

            value = jsonObject["noiseSpeed"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 3) {
                    qWarning() << "Fade event configuration file " << path << " contains an invalid 'noiseSpeed' field. Expected array of size 3";
                }
                else {
                    event.noiseSpeed.x = (float)data.at(0).toDouble();
                    event.noiseSpeed.y = (float)data.at(1).toDouble();
                    event.noiseSpeed.z = (float)data.at(2).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'noiseSpeed' field. Expected array of size 3";
            }

            value = jsonObject["baseSize"];
            if (value.isArray()) {
                QJsonArray data = value.toArray();

                if (data.size() < 3) {
                    qWarning() << "Fade event configuration file " << path << " contains an invalid 'baseSize' field. Expected array of size 3";
                }
                else {
                    event.baseSize.x = (float)data.at(0).toDouble();
                    event.baseSize.y = (float)data.at(1).toDouble();
                    event.baseSize.z = (float)data.at(2).toDouble();
                }
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'baseSize' field. Expected array of size 3";
            }

            value = jsonObject["noiseLevel"];
            if (value.isDouble()) {
                event.noiseLevel = (float)value.toDouble();
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'noiseLevel' field. Expected float value";
            }

            value = jsonObject["baseLevel"];
            if (value.isDouble()) {
                event.baseLevel = (float)value.toDouble();
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'baseLevel' field. Expected float value";
            }

            value = jsonObject["duration"];
            if (value.isDouble()) {
                event._duration = (float)value.toDouble();
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'duration' field. Expected float value";
            }

            value = jsonObject["edgeWidth"];
            if (value.isDouble()) {
                event.edgeWidth = std::min(1.f, std::max(0.f, (float)value.toDouble()));
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'edgeWidth' field. Expected float value";
            }

            value = jsonObject["timing"];
            if (value.isDouble()) {
                event.timing = std::max(0, std::min(TIMING_COUNT - 1, value.toInt()));
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'timing' field. Expected integer value";
            }

            value = jsonObject["isInverted"];
            if (value.isBool()) {
                event._isInverted = value.toBool();
            }
            else {
                qWarning() << "Fade event configuration file " << path << " contains an invalid 'isInverted' field. Expected boolean value";
            }

            emit dirty();
        }
        else {
            qWarning() << "Fade event configuration file" << path << "failed to load:" <<
                error.errorString() << "at offset" << error.offset;
        }
    }
}

FadeConfigureJob::FadeConfigureJob(FadeCommonParameters::Pointer commonParams) : 
    _parameters{ commonParams }
{
	auto texturePath = PathUtils::resourcesPath() + "images/fadeMask.png";
	_fadeMaskMap = DependencyManager::get<TextureCache>()->getImageTexture(texturePath, image::TextureUsage::STRICT_TEXTURE);
}

void FadeConfigureJob::configure(const Config& config) {
    assert(_parameters);
    _parameters->_editedCategory = config.editedCategory;
    _parameters->_isManualThresholdEnabled = config.manualFade;
    _parameters->_manualThreshold = config.manualThreshold;

    for (auto i = 0; i < FadeJobConfig::EVENT_CATEGORY_COUNT; i++) {
        auto& configuration = _configurations[i];
        const auto& eventConfig = config.events[i];

        _parameters->_durations[i] = eventConfig._duration;
        configuration._baseInvSizeAndLevel.x = 1.f / eventConfig.baseSize.x;
        configuration._baseInvSizeAndLevel.y = 1.f / eventConfig.baseSize.y;
        configuration._baseInvSizeAndLevel.z = 1.f / eventConfig.baseSize.z;
        configuration._baseInvSizeAndLevel.w = eventConfig.baseLevel;
        configuration._noiseInvSizeAndLevel.x = 1.f / eventConfig.noiseSize.x;
        configuration._noiseInvSizeAndLevel.y = 1.f / eventConfig.noiseSize.y;
        configuration._noiseInvSizeAndLevel.z = 1.f / eventConfig.noiseSize.z;
        configuration._noiseInvSizeAndLevel.w = eventConfig.noiseLevel;
        configuration._isInverted = eventConfig._isInverted & 1;
        configuration._edgeWidthInvWidth.x = eventConfig.edgeWidth;
        configuration._edgeWidthInvWidth.y = 1.f / configuration._edgeWidthInvWidth.x;
        configuration._innerEdgeColor = eventConfig.edgeInnerColor;
        configuration._outerEdgeColor = eventConfig.edgeOuterColor;
        _parameters->_thresholdScale[i] = 1.f + (configuration._edgeWidthInvWidth.x + std::max(0.f, (eventConfig.noiseLevel + eventConfig.baseLevel)*0.5f-0.5f));
        _parameters->_noiseSpeed[i] = eventConfig.noiseSpeed;
        _parameters->_timing[i] = (FadeJobConfig::Timing) eventConfig.timing;
    }
    _isBufferDirty = true;
}

void FadeConfigureJob::run(const render::RenderContextPointer& renderContext, const Input& input, Output& output) {
    if (_isBufferDirty || _parameters->_isEditEnabled) {
        auto& configurations = output.edit1().edit();
        std::copy(_configurations, _configurations + FadeJobConfig::EVENT_CATEGORY_COUNT, configurations.parameters);
        if (_parameters->_editedCategory == FadeJobConfig::USER_ENTER_LEAVE_DOMAIN) {
            configurations.parameters[FadeJobConfig::USER_ENTER_LEAVE_DOMAIN]._baseInvSizeAndLevel.y = 1.0f / input.getDimensions().y;
        }
        _isBufferDirty = false;
    }
    output.edit0() = _fadeMaskMap;
}

const FadeRenderJob* FadeRenderJob::_currentInstance{ nullptr };
gpu::TexturePointer FadeRenderJob::_currentFadeMaskMap;
const gpu::BufferView* FadeRenderJob::_currentFadeBuffer{ nullptr };

void FadeRenderJob::run(const render::RenderContextPointer& renderContext, const Input& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    const auto& inItems = inputs.get0();

    if (!inItems.empty()) {
        const auto& lightingModel = inputs.get1();
        const auto& configuration = inputs.get2();
        const auto& fadeMaskMap = configuration.get0();
        const auto& fadeParamBuffer = configuration.get1();

        RenderArgs* args = renderContext->args;
        render::ShapeKey::Builder   defaultKeyBuilder;

        defaultKeyBuilder.withFade();
        defaultKeyBuilder.withoutCullFace();

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;

            // Very, very ugly hack to keep track of the current fade render job
            _currentInstance = this;
            _currentFadeMaskMap = fadeMaskMap;
            _currentFadeBuffer = &fadeParamBuffer;

            // Update interactive edit effect
            if (_parameters->_isEditEnabled) {
                updateFadeEdit(renderContext, inItems.front());
            }
            else {
                _editPreviousTime = 0;
            }

            // Setup camera, projection and viewport for all items
            batch.setViewportTransform(args->_viewport);
            batch.setStateScissorRect(args->_viewport);

            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);

            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat);

            // Setup lighting model for all items;
            batch.setUniformBuffer(render::ShapePipeline::Slot::LIGHTING_MODEL, lightingModel->getParametersBuffer());

            // From the lighting model define a global shapKey ORED with individiual keys
            render::ShapeKey::Builder keyBuilder = defaultKeyBuilder;
            if (lightingModel->isWireframeEnabled()) {
                keyBuilder.withWireframe();
            }

            // Prepare fade effect
            bindPerBatch(batch, fadeMaskMap, render::ShapePipeline::Slot::MAP::FADE_MASK, &fadeParamBuffer, render::ShapePipeline::Slot::BUFFER::FADE_PARAMETERS);

            render::ShapeKey globalKey = keyBuilder.build();
            args->_globalShapeKey = globalKey._flags.to_ulong();
            args->_enableFade = true;

            renderShapes(renderContext, _shapePlumber, inItems, -1, globalKey);

            args->_enableFade = false;
            args->_batch = nullptr;
            args->_globalShapeKey = 0;

            // Very, very ugly hack to keep track of the current fade render job
            _currentInstance = nullptr;
            _currentFadeMaskMap.reset();
            _currentFadeBuffer = nullptr;
        });
    }
}

float FadeRenderJob::computeElementEnterThreshold(double time, const double period, FadeJobConfig::Timing timing) const {
    assert(period > 0.0);
    float fadeAlpha = 1.0f;
    const double INV_FADE_PERIOD = 1.0 / period;
    double fraction = time * INV_FADE_PERIOD;
    fraction = std::max(fraction, 0.0);
    if (fraction < 1.0) {
        switch (timing) {
        default:
            fadeAlpha = fraction;
            break;
        case FadeJobConfig::EASE_IN:
            fadeAlpha = fraction*fraction*fraction;
            break;
        case FadeJobConfig::EASE_OUT:
            fadeAlpha = 1.f - fraction;
            fadeAlpha = 1.f- fadeAlpha*fadeAlpha*fadeAlpha;
            break;
        case FadeJobConfig::EASE_IN_OUT:
            fadeAlpha = fraction*fraction*fraction*(fraction*(fraction * 6 - 15) + 10);
            break;
        }
    }
    return fadeAlpha;
}

float FadeRenderJob::computeFadePercent(quint64 startTime) {
    const double time = (double)(int64_t(usecTimestampNow()) - int64_t(startTime)) / (double)(USECS_PER_SECOND);
    assert(_currentInstance);
    return _currentInstance->computeElementEnterThreshold(time, 
        _currentInstance->_parameters->_durations[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN],
        _currentInstance->_parameters->_timing[FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN]);
}

void FadeRenderJob::updateFadeEdit(const render::RenderContextPointer& renderContext, const render::ItemBound& itemBounds) {
    if (_editPreviousTime == 0) {
        _editPreviousTime = usecTimestampNow();
        _editTime = 0.0;
    }

    uint64_t now = usecTimestampNow();
    const double deltaTime = (int64_t(now) - int64_t(_editPreviousTime)) / double(USECS_PER_SECOND);
    const int editedCategory = _parameters->_editedCategory;
    const double eventDuration = (double)_parameters->_durations[editedCategory];
    const FadeJobConfig::Timing timing = _parameters->_timing[editedCategory];
    const double waitTime = 0.5;    // Wait between fade in and out
    double  cycleTime = fmod(_editTime, (eventDuration + waitTime) * 2.0);
    bool inverseTime = false;

    _editTime += deltaTime;
    _editPreviousTime = now;

    if (_parameters->_isManualThresholdEnabled) {
        _editThreshold = _parameters->_manualThreshold;
    }
    else {
        if (cycleTime < eventDuration) {
            _editThreshold = 1.f - computeElementEnterThreshold(cycleTime, eventDuration, timing);
        }
        else if (cycleTime < (eventDuration + waitTime)) {
            _editThreshold = 0.f;
        }
        else if (cycleTime < (2 * eventDuration + waitTime)) {
            _editThreshold = computeElementEnterThreshold(cycleTime - (eventDuration + waitTime), eventDuration, timing);
            inverseTime = true;
        }
        else {
            _editThreshold = 1.f;
            inverseTime = true;
        }
    }

    float threshold = _editThreshold;
    if (editedCategory != FadeJobConfig::BUBBLE_ISECT_OWNER) {
        threshold = (threshold - 0.5f)*_parameters->_thresholdScale[editedCategory] + 0.5f;
    }
    renderContext->jobConfig->setProperty("threshold", threshold);

    _editNoiseOffset = _parameters->_noiseSpeed[editedCategory] * (float)_editTime;
    if (inverseTime) {
        _editNoiseOffset = -_editNoiseOffset;
    }

    switch (editedCategory) {
    case FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN:
        break;

    case FadeJobConfig::BUBBLE_ISECT_OWNER:
    {
        const glm::vec3 cameraPos = renderContext->args->getViewFrustum().getPosition();
        glm::vec3 delta = itemBounds.bound.calcCenter() - cameraPos;
        float distance = glm::length(delta);

        delta = glm::normalize(delta) * std::max(0.f, distance - 0.5f);

        _editBaseOffset = cameraPos + delta*_editThreshold;
        _editThreshold = 0.33f;
    }
    break;

    case FadeJobConfig::BUBBLE_ISECT_TRESPASSER:
    {
        _editBaseOffset = glm::vec3{ 0.f, 0.f, 0.f };
    }
        break;

    case FadeJobConfig::USER_ENTER_LEAVE_DOMAIN:
    {
        _editBaseOffset = itemBounds.bound.calcCenter();
        _editBaseOffset.y -= itemBounds.bound.getDimensions().y / 2.f;
    }
    break;

    case FadeJobConfig::AVATAR_CHANGE:
        break;

    default:
        assert(false);
    }
}

void FadeRenderJob::bindPerBatch(gpu::Batch& batch, int fadeMaskMapLocation, int fadeBufferLocation) {
    assert(_currentFadeMaskMap);
    assert(_currentFadeBuffer!=nullptr);
    bindPerBatch(batch, _currentFadeMaskMap, fadeMaskMapLocation, _currentFadeBuffer, fadeBufferLocation);
}

void FadeRenderJob::bindPerBatch(gpu::Batch& batch, gpu::TexturePointer texture, int fadeMaskMapLocation, const gpu::BufferView* buffer, int fadeBufferLocation) {
    batch.setResourceTexture(fadeMaskMapLocation, texture);
    batch.setUniformBuffer(fadeBufferLocation, *buffer);
}

void FadeRenderJob::bindPerBatch(gpu::Batch& batch, gpu::TexturePointer texture, const gpu::BufferView* buffer, const gpu::PipelinePointer& pipeline)  {
    auto program = pipeline->getProgram();
    auto maskMapLocation = program->getTextures().findLocation("fadeMaskMap");
    auto bufferLocation = program->getUniformBuffers().findLocation("fadeParametersBuffer");
    bindPerBatch(batch, texture, maskMapLocation, buffer, bufferLocation);
}

bool FadeRenderJob::bindPerItem(gpu::Batch& batch, RenderArgs* args, glm::vec3 offset, quint64 startTime) {
    return bindPerItem(batch, args->_pipeline->pipeline.get(), offset, startTime);
}

bool FadeRenderJob::bindPerItem(gpu::Batch& batch, const gpu::Pipeline* pipeline, glm::vec3 offset, quint64 startTime) {
    auto& uniforms = pipeline->getProgram()->getUniforms();
    auto fadeNoiseOffsetLocation = uniforms.findLocation("fadeNoiseOffset");
    auto fadeBaseOffsetLocation = uniforms.findLocation("fadeBaseOffset");
    auto fadeThresholdLocation = uniforms.findLocation("fadeThreshold");
    auto fadeCategoryLocation = uniforms.findLocation("fadeCategory");

    if (fadeNoiseOffsetLocation >= 0 || fadeBaseOffsetLocation>=0 || fadeThresholdLocation >= 0 || fadeCategoryLocation>=0) {
        float threshold;
        int eventCategory = FadeJobConfig::ELEMENT_ENTER_LEAVE_DOMAIN;
        glm::vec3 noiseOffset = offset;
        glm::vec3 baseOffset = offset;

        threshold = 1.f-computeFadePercent(startTime);

        // Manage interactive edition override
        assert(_currentInstance);
        if (_currentInstance->_parameters->_isEditEnabled) {
            eventCategory = _currentInstance->_parameters->_editedCategory;
            threshold = _currentInstance->_editThreshold;
            noiseOffset += _currentInstance->_editNoiseOffset;
            // This works supposing offset is the world position of the object that is fading.
            if (eventCategory != FadeJobConfig::BUBBLE_ISECT_TRESPASSER) {
                baseOffset = _currentInstance->_editBaseOffset - offset;
            }
        }

        if (eventCategory != FadeJobConfig::BUBBLE_ISECT_OWNER) {
            threshold = (threshold - 0.5f)*_currentInstance->_parameters->_thresholdScale[eventCategory] + 0.5f;
        }

        batch._glUniform1i(fadeCategoryLocation, eventCategory);
        batch._glUniform1f(fadeThresholdLocation, threshold);
        // This is really temporary
        batch._glUniform3f(fadeNoiseOffsetLocation, noiseOffset.x, noiseOffset.y, noiseOffset.z);
        // This is really temporary
        batch._glUniform3f(fadeBaseOffsetLocation, baseOffset.x, baseOffset.y, baseOffset.z);

        return threshold > 0.f;
    }
    return false;
}
