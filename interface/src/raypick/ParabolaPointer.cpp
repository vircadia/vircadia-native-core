//
//  Created by Sam Gondelman 7/17/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ParabolaPointer.h"

#include "Application.h"
#include "avatar/AvatarManager.h"
#include <StencilMaskPass.h>

#include <DependencyManager.h>
#include <shaders/Shaders.h>

#include "ParabolaPick.h"
const glm::vec4 ParabolaPointer::RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_COLOR { 1.0f };
const float ParabolaPointer::RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_WIDTH { 0.01f };
const bool ParabolaPointer::RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_ISVISIBLEINSECONDARYCAMERA { false };

gpu::PipelinePointer ParabolaPointer::RenderState::ParabolaRenderItem::_parabolaPipeline { nullptr };
gpu::PipelinePointer ParabolaPointer::RenderState::ParabolaRenderItem::_transparentParabolaPipeline { nullptr };

ParabolaPointer::ParabolaPointer(const QVariant& rayProps, const RenderStateMap& renderStates, const DefaultRenderStateMap& defaultRenderStates, bool hover,
                                 const PointerTriggers& triggers, bool faceAvatar, bool followNormal, float followNormalStrength, bool centerEndY, bool lockEnd, bool distanceScaleEnd,
                                 bool scaleWithAvatar, bool enabled) :
    PathPointer(PickQuery::Parabola, rayProps, renderStates, defaultRenderStates, hover, triggers, faceAvatar, followNormal, followNormalStrength,
                centerEndY, lockEnd, distanceScaleEnd, scaleWithAvatar, enabled)
{
}

void ParabolaPointer::editRenderStatePath(const std::string& state, const QVariant& pathProps) {
    auto renderState = std::static_pointer_cast<RenderState>(_renderStates[state]);
    if (renderState) {
        QVariantMap pathMap = pathProps.toMap();
        glm::vec3 color = glm::vec3(RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_COLOR);
        float alpha = RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_COLOR.a;
        float width = RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_WIDTH;
        bool isVisibleInSecondaryCamera = RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_ISVISIBLEINSECONDARYCAMERA;
        bool enabled = false;
        if (!pathMap.isEmpty()) {
            enabled = true;
            if (pathMap["color"].isValid()) {
                bool valid;
                color = toGlm(xColorFromVariant(pathMap["color"], valid));
            }
            if (pathMap["alpha"].isValid()) {
                alpha = pathMap["alpha"].toFloat();
            }
            if (pathMap["width"].isValid()) {
                width = pathMap["width"].toFloat();
                renderState->setPathWidth(width);
            }
            if (pathMap["isVisibleInSecondaryCamera"].isValid()) {
                isVisibleInSecondaryCamera = pathMap["isVisibleInSecondaryCamera"].toBool();
            }
        }
        renderState->editParabola(color, alpha, width, isVisibleInSecondaryCamera, enabled);
    }
}

glm::vec3 ParabolaPointer::getPickOrigin(const PickResultPointer& pickResult) const {
    auto parabolaPickResult = std::static_pointer_cast<ParabolaPickResult>(pickResult);
    return (parabolaPickResult ? vec3FromVariant(parabolaPickResult->pickVariant["origin"]) : glm::vec3(0.0f));
}

glm::vec3 ParabolaPointer::getPickEnd(const PickResultPointer& pickResult, float distance) const {
    auto parabolaPickResult = std::static_pointer_cast<ParabolaPickResult>(pickResult);
    if (distance > 0.0f) {
        PickParabola pick = PickParabola(parabolaPickResult->pickVariant);
        return pick.origin + pick.velocity * distance + 0.5f * pick.acceleration * distance * distance;
    } else {
        return parabolaPickResult->intersection;
    }
}

glm::vec3 ParabolaPointer::getPickedObjectNormal(const PickResultPointer& pickResult) const {
    auto parabolaPickResult = std::static_pointer_cast<ParabolaPickResult>(pickResult);
    return (parabolaPickResult ? parabolaPickResult->surfaceNormal : glm::vec3(0.0f));
}

IntersectionType ParabolaPointer::getPickedObjectType(const PickResultPointer& pickResult) const {
    auto parabolaPickResult = std::static_pointer_cast<ParabolaPickResult>(pickResult);
    return (parabolaPickResult ? parabolaPickResult->type : IntersectionType::NONE);
}

QUuid ParabolaPointer::getPickedObjectID(const PickResultPointer& pickResult) const {
    auto parabolaPickResult = std::static_pointer_cast<ParabolaPickResult>(pickResult);
    return (parabolaPickResult ? parabolaPickResult->objectID : QUuid());
}

void ParabolaPointer::setVisualPickResultInternal(PickResultPointer pickResult, IntersectionType type, const QUuid& id,
                                               const glm::vec3& intersection, float distance, const glm::vec3& surfaceNormal) {
    auto parabolaPickResult = std::static_pointer_cast<ParabolaPickResult>(pickResult);
    if (parabolaPickResult) {
        parabolaPickResult->type = type;
        parabolaPickResult->objectID = id;
        parabolaPickResult->intersection = intersection;
        parabolaPickResult->distance = distance;
        parabolaPickResult->surfaceNormal = surfaceNormal;
        PickParabola parabola = PickParabola(parabolaPickResult->pickVariant);
        parabolaPickResult->pickVariant["velocity"] = vec3toVariant((intersection - parabola.origin -
            0.5f * parabola.acceleration * parabolaPickResult->parabolicDistance * parabolaPickResult->parabolicDistance) / parabolaPickResult->parabolicDistance);
    }
}

ParabolaPointer::RenderState::RenderState(const OverlayID& startID, const OverlayID& endID, const glm::vec3& pathColor, float pathAlpha, float pathWidth,
                                          bool isVisibleInSecondaryCamera, bool pathEnabled) :
    StartEndRenderState(startID, endID)
{
    render::Transaction transaction;
    auto scene = qApp->getMain3DScene();
    _pathID = scene->allocateID();
    _pathWidth = pathWidth;
    if (render::Item::isValidID(_pathID)) {
        auto renderItem = std::make_shared<ParabolaRenderItem>(pathColor, pathAlpha, pathWidth, isVisibleInSecondaryCamera, pathEnabled);
        transaction.resetItem(_pathID, std::make_shared<ParabolaRenderItem::Payload>(renderItem));
        scene->enqueueTransaction(transaction);
    }
}

void ParabolaPointer::RenderState::cleanup() {
    StartEndRenderState::cleanup();
    if (render::Item::isValidID(_pathID)) {
        render::Transaction transaction;
        auto scene = qApp->getMain3DScene();
        transaction.removeItem(_pathID);
        scene->enqueueTransaction(transaction);
    }
}

void ParabolaPointer::RenderState::disable() {
    StartEndRenderState::disable();
    if (render::Item::isValidID(_pathID)) {
        render::Transaction transaction;
        auto scene = qApp->getMain3DScene();
        transaction.updateItem<ParabolaRenderItem>(_pathID, [](ParabolaRenderItem& item) {
            item.setVisible(false);
        });
        scene->enqueueTransaction(transaction);
    }
}

void ParabolaPointer::RenderState::editParabola(const glm::vec3& color, float alpha, float width, bool isVisibleInSecondaryCamera, bool enabled) {
    if (render::Item::isValidID(_pathID)) {
        render::Transaction transaction;
        auto scene = qApp->getMain3DScene();
        transaction.updateItem<ParabolaRenderItem>(_pathID, [color, alpha, width, isVisibleInSecondaryCamera, enabled](ParabolaRenderItem& item) {
            item.setColor(color);
            item.setAlpha(alpha);
            item.setWidth(width);
            item.setIsVisibleInSecondaryCamera(isVisibleInSecondaryCamera);
            item.setEnabled(enabled);
            item.updateKey();
        });
        scene->enqueueTransaction(transaction);
    }
}

void ParabolaPointer::RenderState::update(const glm::vec3& origin, const glm::vec3& end, const glm::vec3& surfaceNormal, bool scaleWithAvatar, bool distanceScaleEnd, bool centerEndY,
                                          bool faceAvatar, bool followNormal, float followNormalStrength, float distance, const PickResultPointer& pickResult) {
    StartEndRenderState::update(origin, end, surfaceNormal, scaleWithAvatar, distanceScaleEnd, centerEndY, faceAvatar, followNormal, followNormalStrength, distance, pickResult);
    auto parabolaPickResult = std::static_pointer_cast<ParabolaPickResult>(pickResult);
    if (parabolaPickResult && render::Item::isValidID(_pathID)) {
        render::Transaction transaction;
        auto scene = qApp->getMain3DScene();

        PickParabola parabola = PickParabola(parabolaPickResult->pickVariant);
        glm::vec3 velocity = parabola.velocity;
        glm::vec3 acceleration = parabola.acceleration;
        float parabolicDistance = distance > 0.0f ? distance : parabolaPickResult->parabolicDistance;
        float width = scaleWithAvatar ? getPathWidth() * DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale() : getPathWidth();
        transaction.updateItem<ParabolaRenderItem>(_pathID, [origin, velocity, acceleration, parabolicDistance, width](ParabolaRenderItem& item) {
            item.setVisible(true);
            item.setOrigin(origin);
            item.setVelocity(velocity);
            item.setAcceleration(acceleration);
            item.setParabolicDistance(parabolicDistance);
            item.setWidth(width);
            item.updateBounds();
        });
        scene->enqueueTransaction(transaction);
    }
}

std::shared_ptr<StartEndRenderState> ParabolaPointer::buildRenderState(const QVariantMap& propMap) {
    QUuid startID;
    if (propMap["start"].isValid()) {
        QVariantMap startMap = propMap["start"].toMap();
        if (startMap["type"].isValid()) {
            startMap.remove("visible");
            startID = qApp->getOverlays().addOverlay(startMap["type"].toString(), startMap);
        }
    }

    glm::vec3 color = glm::vec3(RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_COLOR);
    float alpha = RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_COLOR.a;
    float width = RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_WIDTH;
    bool isVisibleInSecondaryCamera = RenderState::ParabolaRenderItem::DEFAULT_PARABOLA_ISVISIBLEINSECONDARYCAMERA;
    bool enabled = false;
    if (propMap["path"].isValid()) {
        enabled = true;
        QVariantMap pathMap = propMap["path"].toMap();
        if (pathMap["color"].isValid()) {
            bool valid;
            color = toGlm(xColorFromVariant(pathMap["color"], valid));
        }

        if (pathMap["alpha"].isValid()) {
            alpha = pathMap["alpha"].toFloat();
        }

        if (pathMap["width"].isValid()) {
            width = pathMap["width"].toFloat();
        }

        if (pathMap["isVisibleInSecondaryCamera"].isValid()) {
            isVisibleInSecondaryCamera = pathMap["isVisibleInSecondaryCamera"].toBool();
        }
    }

    QUuid endID;
    if (propMap["end"].isValid()) {
        QVariantMap endMap = propMap["end"].toMap();
        if (endMap["type"].isValid()) {
            endMap.remove("visible");
            endID = qApp->getOverlays().addOverlay(endMap["type"].toString(), endMap);
        }
    }

    return std::make_shared<RenderState>(startID, endID, color, alpha, width, isVisibleInSecondaryCamera, enabled);
}

PointerEvent ParabolaPointer::buildPointerEvent(const PickedObject& target, const PickResultPointer& pickResult, const std::string& button, bool hover) {
    QUuid pickedID;
    glm::vec3 intersection, surfaceNormal, origin, velocity, acceleration;
    auto parabolaPickResult = std::static_pointer_cast<ParabolaPickResult>(pickResult);
    if (parabolaPickResult) {
        intersection = parabolaPickResult->intersection;
        surfaceNormal = parabolaPickResult->surfaceNormal;
        const QVariantMap& parabola = parabolaPickResult->pickVariant;
        origin = vec3FromVariant(parabola["origin"]);
        velocity = vec3FromVariant(parabola["velocity"]);
        acceleration = vec3FromVariant(parabola["acceleration"]);
        pickedID = parabolaPickResult->objectID;
    }

    if (pickedID != target.objectID) {
        intersection = findIntersection(target, origin, velocity, acceleration);
    }
    glm::vec2 pos2D = findPos2D(target, intersection);

    // If we just started triggering and we haven't moved too much, don't update intersection and pos2D
    TriggerState& state = hover ? _latestState : _states[button];
    float sensorToWorldScale = DependencyManager::get<AvatarManager>()->getMyAvatar()->getSensorToWorldScale();
    float deadspotSquared = TOUCH_PRESS_TO_MOVE_DEADSPOT_SQUARED * sensorToWorldScale * sensorToWorldScale;
    bool withinDeadspot = usecTimestampNow() - state.triggerStartTime < POINTER_MOVE_DELAY && glm::distance2(pos2D, state.triggerPos2D) < deadspotSquared;
    if ((state.triggering || state.wasTriggering) && !state.deadspotExpired && withinDeadspot) {
        pos2D = state.triggerPos2D;
        intersection = state.intersection;
        surfaceNormal = state.surfaceNormal;
    }
    if (!withinDeadspot) {
        state.deadspotExpired = true;
    }

    return PointerEvent(pos2D, intersection, surfaceNormal, velocity);
}

glm::vec3 ParabolaPointer::findIntersection(const PickedObject& pickedObject, const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration) {
    // TODO: implement
    switch (pickedObject.type) {
        case ENTITY:
            //return ParabolaPick::intersectParabolaWithEntityXYPlane(pickedObject.objectID, origin, velocity, acceleration);
        case OVERLAY:
            //return ParabolaPick::intersectParabolaWithOverlayXYPlane(pickedObject.objectID, origin, velocity, acceleration);
        default:
            return glm::vec3(NAN);
    }
}

ParabolaPointer::RenderState::ParabolaRenderItem::ParabolaRenderItem(const glm::vec3& color, float alpha, float width,
                                                                     bool isVisibleInSecondaryCamera, bool enabled) :
    _isVisibleInSecondaryCamera(isVisibleInSecondaryCamera), _enabled(enabled)
{
    _uniformBuffer->resize(sizeof(ParabolaData));
    setColor(color);
    setAlpha(alpha);
    setWidth(width);
    updateKey();
}

void ParabolaPointer::RenderState::ParabolaRenderItem::setVisible(bool visible) {
    if (visible && _enabled) {
        _key = render::ItemKey::Builder(_key).withVisible();
    } else {
        _key = render::ItemKey::Builder(_key).withInvisible();
    }
    _visible = visible;
}

void ParabolaPointer::RenderState::ParabolaRenderItem::updateKey() {
    auto builder = _parabolaData.color.a < 1.0f ? render::ItemKey::Builder::transparentShape() : render::ItemKey::Builder::opaqueShape();

    // TODO: parabolas should cast shadows, but they're so thin that the cascaded shadow maps make them look pretty bad
    //builder.withShadowCaster();

    if (_enabled && _visible) {
        builder.withVisible();
    } else {
        builder.withInvisible();
    }

    if (_isVisibleInSecondaryCamera) {
        builder.withTagBits(render::hifi::TAG_ALL_VIEWS);
    } else {
        builder.withTagBits(render::hifi::TAG_MAIN_VIEW);
    }

    _key = builder.build();
}

void ParabolaPointer::RenderState::ParabolaRenderItem::updateBounds() {
    glm::vec3 max = _origin;
    glm::vec3 min = _origin;

    glm::vec3 end = _origin + _parabolaData.velocity * _parabolaData.parabolicDistance +
        0.5f * _parabolaData.acceleration * _parabolaData.parabolicDistance * _parabolaData.parabolicDistance;
    max = glm::max(max, end);
    min = glm::min(min, end);

    for (int i = 0; i < 3; i++) {
        if (fabsf(_parabolaData.velocity[i]) > EPSILON && fabsf(_parabolaData.acceleration[i]) > EPSILON) {
            float maxT = -_parabolaData.velocity[i] / _parabolaData.acceleration[i];
            if (maxT > 0.0f && maxT < _parabolaData.parabolicDistance) {
                glm::vec3 maxPoint = _origin + _parabolaData.velocity * maxT + 0.5f * _parabolaData.acceleration * maxT * maxT;
                max = glm::max(max, maxPoint);
                min = glm::min(min, maxPoint);
            }
        }
    }

    glm::vec3 halfWidth = glm::vec3(0.5f * _parabolaData.width);
    max += halfWidth;
    min -= halfWidth;

    _bound = AABox(min, max - min);
}

const gpu::PipelinePointer ParabolaPointer::RenderState::ParabolaRenderItem::getParabolaPipeline() {
    if (!_parabolaPipeline || !_transparentParabolaPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render_utils::program::parabola);

        {
            auto state = std::make_shared<gpu::State>();
            state->setDepthTest(true, true, gpu::LESS_EQUAL);
            state->setBlendFunction(false,
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            PrepareStencil::testMaskDrawShape(*state);
            state->setCullMode(gpu::State::CULL_NONE);
            _parabolaPipeline = gpu::Pipeline::create(program, state);
        }

        {
            auto state = std::make_shared<gpu::State>();
            state->setDepthTest(true, true, gpu::LESS_EQUAL);
            state->setBlendFunction(true,
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            PrepareStencil::testMask(*state);
            state->setCullMode(gpu::State::CULL_NONE);
            _transparentParabolaPipeline = gpu::Pipeline::create(program, state);
        }
    }
    return (_parabolaData.color.a < 1.0f ? _transparentParabolaPipeline : _parabolaPipeline);
}

void ParabolaPointer::RenderState::ParabolaRenderItem::render(RenderArgs* args) {
    if (!_visible) {
        return;
    }

    gpu::Batch& batch = *(args->_batch);

    Transform transform;
    transform.setTranslation(_origin);
    batch.setModelTransform(transform);

    batch.setPipeline(getParabolaPipeline());

    const int MAX_SECTIONS = 100;
    if (glm::length2(_parabolaData.acceleration) < EPSILON) {
        _parabolaData.numSections = 1;
    } else {
        _parabolaData.numSections = glm::clamp((int)(_parabolaData.parabolicDistance + 1) * 10, 1, MAX_SECTIONS);
    }
    updateUniformBuffer();
    batch.setUniformBuffer(0, _uniformBuffer);

    // We draw 2 * n + 2 vertices for a triangle strip
    batch.draw(gpu::TRIANGLE_STRIP, 2 * _parabolaData.numSections + 2, 0);
}

namespace render {
    template <> const ItemKey payloadGetKey(const ParabolaPointer::RenderState::ParabolaRenderItem::Pointer& payload) {
        return payload->getKey();
    }
    template <> const Item::Bound payloadGetBound(const ParabolaPointer::RenderState::ParabolaRenderItem::Pointer& payload) {
        if (payload) {
            return payload->getBound();
        }
        return Item::Bound();
    }
    template <> void payloadRender(const ParabolaPointer::RenderState::ParabolaRenderItem::Pointer& payload, RenderArgs* args) {
        if (payload) {
            payload->render(args);
        }
    }
    template <> const ShapeKey shapeGetShapeKey(const ParabolaPointer::RenderState::ParabolaRenderItem::Pointer& payload) {
        return ShapeKey::Builder::ownPipeline();
    }
}