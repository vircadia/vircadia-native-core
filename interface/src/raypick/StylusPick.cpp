//
//  Created by Bradley Austin Davis on 2017/10/24
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "StylusPick.h"

#include "RayPick.h"

#include <glm/glm.hpp>

#include "ui/overlays/Base3DOverlay.h"

#include "Application.h"
#include <DependencyManager.h>
#include "avatar/AvatarManager.h"

#include <controllers/StandardControls.h>
#include <controllers/UserInputMapper.h>

using namespace bilateral;
float StylusPick::WEB_STYLUS_LENGTH = 0.2f;

struct SideData {
    QString avatarJoint;
    QString cameraJoint;
    controller::StandardPoseChannel channel;
    controller::Hand hand;
    glm::vec3 grabPointSphereOffset;

    int getJointIndex(bool finger) {
        const auto& jointName = finger ? avatarJoint : cameraJoint;
        return DependencyManager::get<AvatarManager>()->getMyAvatar()->getJointIndex(jointName);
    }
};

static const std::array<SideData, 2> SIDES{ { { "LeftHandIndex4",
                                                "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
                                                controller::StandardPoseChannel::LEFT_HAND,
                                                controller::Hand::LEFT,
                                                { -0.04f, 0.13f, 0.039f } },
                                              { "RightHandIndex4",
                                                "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND",
                                                controller::StandardPoseChannel::RIGHT_HAND,
                                                controller::Hand::RIGHT,
                                                { 0.04f, 0.13f, 0.039f } } } };

std::shared_ptr<PickResult> StylusPickResult::compareAndProcessNewResult(const std::shared_ptr<PickResult>& newRes) {
    auto newStylusResult = std::static_pointer_cast<StylusPickResult>(newRes);
    if (newStylusResult && newStylusResult->distance < distance) {
        return std::make_shared<StylusPickResult>(*newStylusResult);
    } else {
        return std::make_shared<StylusPickResult>(*this);
    }
}

bool StylusPickResult::checkOrFilterAgainstMaxDistance(float maxDistance) {
    return distance < maxDistance;
}

StylusPick::StylusPick(Side side, const PickFilter& filter, float maxDistance, bool enabled, const glm::vec3& tipOffset) :
    Pick(StylusTip(side, tipOffset), filter, maxDistance, enabled)
{
}

static StylusTip getFingerWorldLocation(Side side) {
    const auto& sideData = SIDES[index(side)];
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    auto fingerJointIndex = myAvatar->getJointIndex(sideData.avatarJoint);

    if (fingerJointIndex == -1) {
        return StylusTip();
    }

    auto fingerPosition = myAvatar->getAbsoluteJointTranslationInObjectFrame(fingerJointIndex);
    auto fingerRotation = myAvatar->getAbsoluteJointRotationInObjectFrame(fingerJointIndex);
    auto avatarOrientation = myAvatar->getWorldOrientation();
    auto avatarPosition = myAvatar->getWorldPosition();
    StylusTip result;
    result.side = side;
    result.orientation = avatarOrientation * fingerRotation;
    result.position = avatarPosition + (avatarOrientation * fingerPosition);
    return result;
}

// controllerWorldLocation is where the controller would be, in-world, with an added offset
static StylusTip getControllerWorldLocation(Side side, const glm::vec3& tipOffset) {
    static const std::array<controller::Input, 2> INPUTS{ { UserInputMapper::makeStandardInput(SIDES[0].channel),
                                                           UserInputMapper::makeStandardInput(SIDES[1].channel) } };
    const auto sideIndex = index(side);
    const auto& input = INPUTS[sideIndex];

    const auto pose = DependencyManager::get<UserInputMapper>()->getPose(input);
    const auto& valid = pose.valid;
    StylusTip result;
    if (valid) {
        result.side = side;
        const auto& sideData = SIDES[sideIndex];
        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        float sensorScaleFactor = myAvatar->getSensorToWorldScale();
        auto controllerJointIndex = myAvatar->getJointIndex(sideData.cameraJoint);

        const auto avatarOrientation = myAvatar->getWorldOrientation();
        const auto avatarPosition = myAvatar->getWorldPosition();
        result.orientation = avatarOrientation * myAvatar->getAbsoluteJointRotationInObjectFrame(controllerJointIndex);

        result.position = avatarPosition + (avatarOrientation * myAvatar->getAbsoluteJointTranslationInObjectFrame(controllerJointIndex));
        // add to the real position so the grab-point is out in front of the hand, a bit
        result.position += result.orientation * (sideData.grabPointSphereOffset * sensorScaleFactor);
        // move the stylus forward a bit
        result.position += result.orientation * (tipOffset * sensorScaleFactor);

        auto worldControllerPos = avatarPosition + avatarOrientation * pose.translation;
        // compute tip velocity from hand controller motion, it is more accurate than computing it from previous positions.
        auto worldControllerLinearVel = avatarOrientation * pose.velocity;
        auto worldControllerAngularVel = avatarOrientation * pose.angularVelocity;
        result.velocity = worldControllerLinearVel + glm::cross(worldControllerAngularVel, result.position - worldControllerPos);
    }

    return result;
}

StylusTip StylusPick::getMathematicalPick() const {
    StylusTip result;
    if (qApp->getPreferAvatarFingerOverStylus()) {
        result = getFingerWorldLocation(_mathPick.side);
    } else {
        result = getControllerWorldLocation(_mathPick.side, _mathPick.tipOffset);
    }
    return result;
}

PickResultPointer StylusPick::getDefaultResult(const QVariantMap& pickVariant) const {
    return std::make_shared<StylusPickResult>(pickVariant);
}

PickResultPointer StylusPick::getEntityIntersection(const StylusTip& pick) {
    std::vector<StylusPickResult> results;
    for (const auto& target : getIncludeItems()) {
        if (target.isNull()) {
            continue;
        }

        auto entity = qApp->getEntities()->getTree()->findEntityByEntityItemID(target);
        if (!entity) {
            continue;
        }

        if (!entity->getVisible() && !getFilter().doesPickInvisible()) {
            continue;
        }

        const auto entityRotation = entity->getWorldOrientation();
        const auto entityPosition = entity->getWorldPosition();

        glm::vec3 normal = entityRotation * Vectors::UNIT_Z;
        float distance = glm::dot(pick.position - entityPosition, normal);
        glm::vec3 intersection = pick.position - (normal * distance);

        glm::vec2 pos2D = RayPick::projectOntoEntityXYPlane(target, intersection, false);
        if (pos2D == glm::clamp(pos2D, glm::vec2(0), glm::vec2(1))) {
            results.push_back(StylusPickResult(IntersectionType::ENTITY, target, distance, intersection, pick, normal));
        }
    }

    StylusPickResult nearestTarget(pick.toVariantMap());
    for (const auto& result : results) {
        if (result.distance < nearestTarget.distance) {
            nearestTarget = result;
        }
    }
    return std::make_shared<StylusPickResult>(nearestTarget);
}

PickResultPointer StylusPick::getOverlayIntersection(const StylusTip& pick) {
    std::vector<StylusPickResult> results;
    for (const auto& target : getIncludeItems()) {
        if (target.isNull()) {
            continue;
        }

        auto overlay = qApp->getOverlays().getOverlay(target);
        // Don't interact with non-3D or invalid overlays
        if (!overlay || !overlay->is3D()) {
            continue;
        }

        if (!overlay->getVisible() && !getFilter().doesPickInvisible()) {
            continue;
        }

        auto overlay3D = std::static_pointer_cast<Base3DOverlay>(overlay);
        const auto overlayRotation = overlay3D->getWorldOrientation();
        const auto overlayPosition = overlay3D->getWorldPosition();

        glm::vec3 normal = overlayRotation * Vectors::UNIT_Z;
        float distance = glm::dot(pick.position - overlayPosition, normal);
        glm::vec3 intersection = pick.position - (normal * distance);

        glm::vec2 pos2D = RayPick::projectOntoOverlayXYPlane(target, intersection, false);
        if (pos2D == glm::clamp(pos2D, glm::vec2(0), glm::vec2(1))) {
            results.push_back(StylusPickResult(IntersectionType::OVERLAY, target, distance, intersection, pick, normal));
        }
    }

    StylusPickResult nearestTarget(pick.toVariantMap());
    for (const auto& result : results) {
        if (result.distance < nearestTarget.distance) {
            nearestTarget = result;
        }
    }
    return std::make_shared<StylusPickResult>(nearestTarget);
}

PickResultPointer StylusPick::getAvatarIntersection(const StylusTip& pick) {
    return std::make_shared<StylusPickResult>(pick.toVariantMap());
}

PickResultPointer StylusPick::getHUDIntersection(const StylusTip& pick) {
    return std::make_shared<StylusPickResult>(pick.toVariantMap());
}

Transform StylusPick::getResultTransform() const {
    PickResultPointer result = getPrevPickResult();
    if (!result) {
        return Transform();
    }

    auto stylusResult = std::static_pointer_cast<StylusPickResult>(result);
    Transform transform;
    transform.setTranslation(stylusResult->intersection);
    return transform;
}
