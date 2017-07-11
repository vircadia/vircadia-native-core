#include "FadeEffect.h"
#include "TextureCache.h"
#include "render/Logging.h"
#include "render/TransitionStage.h"

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

void FadeEditJob::run(const render::RenderContextPointer& renderContext, const FadeEditJob::Input& inputs) {
    auto jobConfig = static_cast<const FadeEditConfig*>(renderContext->jobConfig.get());
    auto& itemBounds = inputs.get0();

    if (jobConfig->editFade) {
        float minIsectDistance = std::numeric_limits<float>::max();
        auto itemId = findNearestItem(renderContext, itemBounds, minIsectDistance);

        if (itemId != render::Item::INVALID_ITEM_ID) {
            const auto& item = renderContext->_scene->getItem(itemId);

            if (item.getTransitionId() == render::TransitionStage::INVALID_INDEX) {
                static const render::Transition::Type categoryToTransition[FadeConfig::CATEGORY_COUNT] = {
                    render::Transition::ELEMENT_ENTER_DOMAIN,
                    render::Transition::BUBBLE_ISECT_OWNER,
                    render::Transition::BUBBLE_ISECT_TRESPASSER,
                    render::Transition::USER_ENTER_DOMAIN,
                    render::Transition::AVATAR_CHANGE
                };

                // Relaunch transition
                render::Transaction transaction;
                transaction.transitionItem(itemId, categoryToTransition[inputs.get1()]);
                renderContext->_scene->enqueueTransaction(transaction);
            }
        }
    }
}

render::ItemID FadeEditJob::findNearestItem(const render::RenderContextPointer& renderContext, const render::ItemBounds& inputs, float& minIsectDistance) const {
    const glm::vec3 rayOrigin = renderContext->args->getViewFrustum().getPosition();
    const glm::vec3 rayDirection = renderContext->args->getViewFrustum().getDirection();
    auto& scene = renderContext->_scene;
    BoxFace face;
    glm::vec3 normal;
    float isectDistance;
    render::ItemID nearestItem = render::Item::INVALID_ITEM_ID;
    const float minDistance = 2.f;

    for (const auto& itemBound : inputs) {
        if (!itemBound.bound.contains(rayOrigin) && itemBound.bound.findRayIntersection(rayOrigin, rayDirection, isectDistance, face, normal)) {
            if (isectDistance>minDistance && isectDistance < minIsectDistance) {
                auto& item = scene->getItem(itemBound.id);

                if (item.getKey().isShape() && !item.getKey().isMeta()) {
                    nearestItem = itemBound.id;
                    minIsectDistance = isectDistance;
                }
            }
        }
    }
    return nearestItem;
}

FadeConfig::FadeConfig() 
{
    events[ELEMENT_ENTER_LEAVE_DOMAIN].noiseSize = glm::vec3{ 0.75f, 0.75f, 0.75f };
    events[ELEMENT_ENTER_LEAVE_DOMAIN].noiseLevel = 1.f;
    events[ELEMENT_ENTER_LEAVE_DOMAIN].noiseSpeed = glm::vec3{ 0.0f, 0.0f, 0.0f };
    events[ELEMENT_ENTER_LEAVE_DOMAIN].timing = FadeConfig::LINEAR;
    events[ELEMENT_ENTER_LEAVE_DOMAIN].baseSize = glm::vec3{ 1.0f, 1.0f, 1.0f };
    events[ELEMENT_ENTER_LEAVE_DOMAIN].baseLevel = 0.f;
    events[ELEMENT_ENTER_LEAVE_DOMAIN].isInverted = false;
    events[ELEMENT_ENTER_LEAVE_DOMAIN].duration = 4.f;
    events[ELEMENT_ENTER_LEAVE_DOMAIN].edgeWidth = 0.1f;
    events[ELEMENT_ENTER_LEAVE_DOMAIN].edgeInnerColor = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 0.0f };
    events[ELEMENT_ENTER_LEAVE_DOMAIN].edgeOuterColor = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 1.0f };

    events[BUBBLE_ISECT_OWNER].noiseSize = glm::vec3{ 1.5f, 1.0f/25.f, 0.5f };
    events[BUBBLE_ISECT_OWNER].noiseLevel = 0.37f;
    events[BUBBLE_ISECT_OWNER].noiseSpeed = glm::vec3{ 1.0f, 0.2f, 1.0f };
    events[BUBBLE_ISECT_OWNER].timing = FadeConfig::LINEAR;
    events[BUBBLE_ISECT_OWNER].baseSize = glm::vec3{ 2.0f, 2.0f, 2.0f };
    events[BUBBLE_ISECT_OWNER].baseLevel = 1.f;
    events[BUBBLE_ISECT_OWNER].isInverted = false;
    events[BUBBLE_ISECT_OWNER].duration = 4.f;
    events[BUBBLE_ISECT_OWNER].edgeWidth = 0.02f;
    events[BUBBLE_ISECT_OWNER].edgeInnerColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 1.0f };
    events[BUBBLE_ISECT_OWNER].edgeOuterColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 2.0f };

    events[BUBBLE_ISECT_TRESPASSER].noiseSize = glm::vec3{ 0.5f, 1.0f / 25.f, 0.5f };
    events[BUBBLE_ISECT_TRESPASSER].noiseLevel = 1.f;
    events[BUBBLE_ISECT_TRESPASSER].noiseSpeed = glm::vec3{ 1.0f, 0.2f, 1.0f };
    events[BUBBLE_ISECT_TRESPASSER].timing = FadeConfig::LINEAR;
    events[BUBBLE_ISECT_TRESPASSER].baseSize = glm::vec3{ 2.0f, 2.0f, 2.0f };
    events[BUBBLE_ISECT_TRESPASSER].baseLevel = 0.f;
    events[BUBBLE_ISECT_TRESPASSER].isInverted = false;
    events[BUBBLE_ISECT_TRESPASSER].duration = 4.f;
    events[BUBBLE_ISECT_TRESPASSER].edgeWidth = 0.025f;
    events[BUBBLE_ISECT_TRESPASSER].edgeInnerColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 1.0f };
    events[BUBBLE_ISECT_TRESPASSER].edgeOuterColor = glm::vec4{ 31.f / 255.f, 198.f / 255.f, 166.f / 255.f, 2.0f };

    events[USER_ENTER_LEAVE_DOMAIN].noiseSize = glm::vec3{ 10.f, 0.01f, 10.0f };
    events[USER_ENTER_LEAVE_DOMAIN].noiseLevel = 0.7f;
    events[USER_ENTER_LEAVE_DOMAIN].noiseSpeed = glm::vec3{ 0.0f, -0.5f, 0.0f };
    events[USER_ENTER_LEAVE_DOMAIN].timing = FadeConfig::LINEAR;
    events[USER_ENTER_LEAVE_DOMAIN].baseSize = glm::vec3{ 10000.f, 1.0f, 10000.0f };
    events[USER_ENTER_LEAVE_DOMAIN].baseLevel = 1.f;
    events[USER_ENTER_LEAVE_DOMAIN].isInverted = true;
    events[USER_ENTER_LEAVE_DOMAIN].duration = 5.f;
    events[USER_ENTER_LEAVE_DOMAIN].edgeWidth = 0.229f;
    events[USER_ENTER_LEAVE_DOMAIN].edgeInnerColor = glm::vec4{ 78.f / 255.f, 215.f / 255.f, 255.f / 255.f, 0.25f };
    events[USER_ENTER_LEAVE_DOMAIN].edgeOuterColor = glm::vec4{ 1.f, 1.f, 1.f, 1.0f };

    events[AVATAR_CHANGE].noiseSize = glm::vec3{ 0.4f, 0.4f, 0.4f };
    events[AVATAR_CHANGE].noiseLevel = 1.f;
    events[AVATAR_CHANGE].noiseSpeed = glm::vec3{ 0.0f, 0.0f, 0.0f };
    events[AVATAR_CHANGE].timing = FadeConfig::LINEAR;
    events[AVATAR_CHANGE].baseSize = glm::vec3{ 0.4f, 0.4f, 0.4f };
    events[AVATAR_CHANGE].baseLevel = 1.f;
    events[AVATAR_CHANGE].isInverted = false;
    events[AVATAR_CHANGE].duration = 3.f;
    events[AVATAR_CHANGE].edgeWidth = 0.05f;
    events[AVATAR_CHANGE].edgeInnerColor = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
    events[AVATAR_CHANGE].edgeOuterColor = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
}

void FadeConfig::setEditedCategory(int value) {
    assert(value < CATEGORY_COUNT);
    editedCategory = std::min<int>(CATEGORY_COUNT, value);
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

void FadeConfig::setEdgeInnerColorR(float value) {
    events[editedCategory].edgeInnerColor.r = value;
    emit dirty();
}

void FadeConfig::setEdgeInnerColorG(float value) {
    events[editedCategory].edgeInnerColor.g = value;
    emit dirty();
}

void FadeConfig::setEdgeInnerColorB(float value) {
    events[editedCategory].edgeInnerColor.b = value;
    emit dirty();
}

void FadeConfig::setEdgeInnerIntensity(float value) {
    events[editedCategory].edgeInnerColor.a = value;
    emit dirty();
}

void FadeConfig::setEdgeOuterColorR(float value) {
    events[editedCategory].edgeOuterColor.r = value;
    emit dirty();
}

void FadeConfig::setEdgeOuterColorG(float value) {
    events[editedCategory].edgeOuterColor.g = value;
    emit dirty();
}

void FadeConfig::setEdgeOuterColorB(float value) {
    events[editedCategory].edgeOuterColor.b = value;
    emit dirty();
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

QString FadeConfig::eventNames[FadeConfig::CATEGORY_COUNT] = {
    "element_enter_leave_domain",
    "bubble_isect_owner",
    "bubble_isect_trespasser",
    "user_enter_leave_domain",
    "avatar_change",
};

void FadeConfig::save() const {
    assert(editedCategory < FadeConfig::CATEGORY_COUNT);
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
        lProperties["duration"] = event.duration;
        lProperties["edgeWidth"] = event.edgeWidth;
        lProperties["timing"] = event.timing;
        lProperties["isInverted"] = event.isInverted;

        file.write( QJsonDocument(lProperties).toJson() );
        file.close();
    }
}

void FadeConfig::load() {
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
                event.duration = (float)value.toDouble();
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
                event.isInverted = value.toBool();
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

FadeJob::FadeJob()
{
	auto texturePath = PathUtils::resourcesPath() + "images/fadeMask.png";
	_fadeMaskMap = DependencyManager::get<TextureCache>()->getImageTexture(texturePath, image::TextureUsage::STRICT_TEXTURE);
    _previousTime = usecTimestampNow();
}

void FadeJob::configure(const Config& config) {
    auto& configurations = _configurations.edit();

    for (auto i = 0; i < FadeConfig::CATEGORY_COUNT; i++) {
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
    bool hasTransactions = false;
    bool isFirstItem = true;
    
    output = (FadeConfig::Category) jobConfig->editedCategory;

    // And now update fade effect
    for (auto transitionId : *transitionStage) {
        auto& state = transitionStage->editTransition(transitionId);
        if (!update(*jobConfig, scene, state, deltaTime)) {
            // Remove transition for this item
            transaction.transitionItem(state.itemId, render::Transition::NONE);
            hasTransactions = true;
        }

        if (isFirstItem) {
            jobConfig->setProperty("threshold", state.threshold);
            isFirstItem = false;
        }
    }

    if (hasTransactions) {
        scene->enqueueTransaction(transaction);
    }
    _previousTime = now;
}

const FadeConfig::Category FadeJob::transitionToCategory[render::Transition::TYPE_COUNT] = {
    FadeConfig::ELEMENT_ENTER_LEAVE_DOMAIN,
    FadeConfig::ELEMENT_ENTER_LEAVE_DOMAIN,
    FadeConfig::BUBBLE_ISECT_OWNER,
    FadeConfig::BUBBLE_ISECT_TRESPASSER,
    FadeConfig::USER_ENTER_LEAVE_DOMAIN,
    FadeConfig::USER_ENTER_LEAVE_DOMAIN,
    FadeConfig::AVATAR_CHANGE
};

bool FadeJob::update(const Config& config, const render::ScenePointer& scene, render::Transition& transition, const double deltaTime) const {
    const auto fadeCategory = transitionToCategory[transition.eventType];
    auto& eventConfig = config.events[fadeCategory];
    auto& item = scene->getItem(transition.itemId);
    const double eventDuration = (double)eventConfig.duration;
    const FadeConfig::Timing timing = (FadeConfig::Timing) eventConfig.timing;
    bool continueTransition = true;

    if (item.exist()) {
        auto aabb = item.getBound();
        if (render::Item::isValidID(transition.boundItemId)) {
            auto& boundItem = scene->getItem(transition.boundItemId);
            if (boundItem.exist()) {
                aabb = boundItem.getBound();
            }
        }
        auto& dimensions = aabb.getDimensions();

        assert(timing < FadeConfig::TIMING_COUNT);

        switch (transition.eventType) {
        case render::Transition::ELEMENT_ENTER_DOMAIN:
        case render::Transition::ELEMENT_LEAVE_DOMAIN:
        {
            transition.threshold = computeElementEnterRatio(transition.time, eventConfig.duration, timing);
            transition.noiseOffset = aabb.calcCenter();
            transition.baseOffset = transition.noiseOffset;
            transition.baseInvSize.x = 1.f / dimensions.x;
            transition.baseInvSize.y = 1.f / dimensions.y;
            transition.baseInvSize.z = 1.f / dimensions.z;
            continueTransition = transition.threshold < 1.f;
            if (transition.eventType == render::Transition::ELEMENT_ENTER_DOMAIN) {
                transition.threshold = 1.f - transition.threshold;
            }
        }
        break;

        case render::Transition::BUBBLE_ISECT_OWNER:
        {
            /*       const glm::vec3 cameraPos = renderContext->args->getViewFrustum().getPosition();
            glm::vec3 delta = itemBounds.bound.calcCenter() - cameraPos;
            float distance = glm::length(delta);

            delta = glm::normalize(delta) * std::max(0.f, distance - 0.5f);

            _editBaseOffset = cameraPos + delta*_editThreshold;
            _editThreshold = 0.33f;*/
        }
        break;

        case render::Transition::BUBBLE_ISECT_TRESPASSER:
        {
            //    _editBaseOffset = glm::vec3{ 0.f, 0.f, 0.f };
        }
        break;

        case render::Transition::USER_ENTER_DOMAIN:
        case render::Transition::USER_LEAVE_DOMAIN:
        {
            transition.threshold = computeElementEnterRatio(transition.time, eventConfig.duration, timing);
            transition.noiseOffset = aabb.calcCenter();
            transition.baseOffset = transition.noiseOffset - dimensions.y / 2.f;
            transition.baseInvSize.x = 1.f / eventConfig.baseSize.x;
            transition.baseInvSize.y = 1.f / dimensions.y;
            transition.baseInvSize.z = 1.f / eventConfig.baseSize.z;
            continueTransition = transition.threshold < 1.f;
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

    transition.threshold = (transition.threshold - 0.5f)*_thresholdScale[fadeCategory] + 0.5f;
    transition.time += deltaTime;

    return continueTransition;
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

render::ShapePipeline::BatchSetter FadeJob::getBatchSetter() const {
    return [this](const render::ShapePipeline& shapePipeline, gpu::Batch& batch, render::Args*) {
        auto program = shapePipeline.pipeline->getProgram();
        auto maskMapLocation = program->getTextures().findLocation("fadeMaskMap");
        auto bufferLocation = program->getUniformBuffers().findLocation("fadeParametersBuffer");
        batch.setResourceTexture(maskMapLocation, _fadeMaskMap);
        batch.setUniformBuffer(bufferLocation, _configurations);
    };
}

render::ShapePipeline::ItemSetter FadeJob::getItemSetter() const {
    return [this](const render::ShapePipeline& shapePipeline, render::Args* args, const render::Item& item) {
        if (!render::TransitionStage::isIndexInvalid(item.getTransitionId())) {
            auto scene = args->_scene;
            auto batch = args->_batch;
            auto transitionStage = scene->getStage<render::TransitionStage>(render::TransitionStage::getName());
            auto& transitionState = transitionStage->getTransition(item.getTransitionId());
            render::ShapeKey shapeKey(args->_globalShapeKey);

            // TODO test various cases: polyvox... etc
            // This is the normal case where we need to push the parameters in uniforms
            {
                auto program = shapePipeline.pipeline->getProgram();
                auto& uniforms = program->getUniforms();
                auto fadeNoiseOffsetLocation = uniforms.findLocation("fadeNoiseOffset");
                auto fadeBaseOffsetLocation = uniforms.findLocation("fadeBaseOffset");
                auto fadeBaseInvSizeLocation = uniforms.findLocation("fadeBaseInvSize");
                auto fadeThresholdLocation = uniforms.findLocation("fadeThreshold");
                auto fadeCategoryLocation = uniforms.findLocation("fadeCategory");

                if (fadeNoiseOffsetLocation >= 0 || fadeBaseInvSizeLocation >= 0 || fadeBaseOffsetLocation >= 0 || fadeThresholdLocation >= 0 || fadeCategoryLocation >= 0) {
                    const auto fadeCategory = transitionToCategory[transitionState.eventType];

                    batch->_glUniform1i(fadeCategoryLocation, fadeCategory);
                    batch->_glUniform1f(fadeThresholdLocation, transitionState.threshold);
                    batch->_glUniform3f(fadeNoiseOffsetLocation, transitionState.noiseOffset.x, transitionState.noiseOffset.y, transitionState.noiseOffset.z);
                    batch->_glUniform3f(fadeBaseOffsetLocation, transitionState.baseOffset.x, transitionState.baseOffset.y, transitionState.baseOffset.z);
                    batch->_glUniform3f(fadeBaseInvSizeLocation, transitionState.baseInvSize.x, transitionState.baseInvSize.y, transitionState.baseInvSize.z);
                }
            }
        }
    };
}
