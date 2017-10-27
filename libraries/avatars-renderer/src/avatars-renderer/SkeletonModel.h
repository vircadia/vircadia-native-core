//
//  SkeletonModel.h
//  interface/src/avatar
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SkeletonModel_h
#define hifi_SkeletonModel_h

#include "CauterizedModel.h"

class Avatar;
class MuscleConstraint;

class SkeletonModel;
using SkeletonModelPointer = std::shared_ptr<SkeletonModel>;
using SkeletonModelWeakPointer = std::weak_ptr<SkeletonModel>;

/// A skeleton loaded from a model.
class SkeletonModel : public CauterizedModel {
    using Parent = CauterizedModel;
    Q_OBJECT

public:

    SkeletonModel(Avatar* owningAvatar, QObject* parent = nullptr);
    ~SkeletonModel();

    void initJointStates() override;

    void simulate(float deltaTime, bool fullUpdate = true) override;
    void updateRig(float deltaTime, glm::mat4 parentTransform) override;
    void updateAttitude(const glm::quat& orientation);

    /// Returns the index of the left hand joint, or -1 if not found.
    int getLeftHandJointIndex() const { return isActive() ? getFBXGeometry().leftHandJointIndex : -1; }

    /// Returns the index of the right hand joint, or -1 if not found.
    int getRightHandJointIndex() const { return isActive() ? getFBXGeometry().rightHandJointIndex : -1; }

    bool getLeftGrabPosition(glm::vec3& position) const;
    bool getRightGrabPosition(glm::vec3& position) const;

    /// Retrieve the position of the left hand
    /// \return true whether or not the position was found
    bool getLeftHandPosition(glm::vec3& position) const;

    /// Retrieve the position of the right hand
    /// \return true whether or not the position was found
    bool getRightHandPosition(glm::vec3& position) const;

    /// Restores some fraction of the default position of the left hand.
    /// \param fraction the fraction of the default position to restore
    /// \return whether or not the left hand joint was found
    bool restoreLeftHandPosition(float fraction = 1.0f, float priority = 1.0f);

    /// Gets the position of the left shoulder.
    /// \return whether or not the left shoulder joint was found
    bool getLeftShoulderPosition(glm::vec3& position) const;

    /// Returns the extended length from the left hand to its last free ancestor.
    float getLeftArmLength() const;

    /// Restores some fraction of the default position of the right hand.
    /// \param fraction the fraction of the default position to restore
    /// \return whether or not the right hand joint was found
    bool restoreRightHandPosition(float fraction = 1.0f, float priority = 1.0f);

    /// Gets the position of the right shoulder.
    /// \return whether or not the right shoulder joint was found
    bool getRightShoulderPosition(glm::vec3& position) const;

    /// Returns the extended length from the right hand to its first free ancestor.
    float getRightArmLength() const;

    /// Returns the position of the head joint.
    /// \return whether or not the head was found
    bool getHeadPosition(glm::vec3& headPosition) const;

    /// Returns the position of the neck joint.
    /// \return whether or not the neck was found
    bool getNeckPosition(glm::vec3& neckPosition) const;

    bool getLocalNeckPosition(glm::vec3& neckPosition) const;

    /// Retrieve the positions of up to two eye meshes.
    /// \return whether or not both eye meshes were found
    bool getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const;

    /// Gets the default position of the mid eye point in model frame coordinates.
    /// \return whether or not the head was found.
    glm::vec3 getDefaultEyeModelPosition() const;

    void renderBoundingCollisionShapes(RenderArgs* args, gpu::Batch& batch, float scale, float alpha);
    float getBoundingCapsuleRadius() const { return _boundingCapsuleRadius; }
    float getBoundingCapsuleHeight() const { return _boundingCapsuleHeight; }
    const glm::vec3 getBoundingCapsuleOffset() const { return _boundingCapsuleLocalOffset; }

    bool hasSkeleton();

    float getHeadClipDistance() const { return _headClipDistance; }

    void onInvalidate() override;

signals:

    void skeletonLoaded();

protected:

    void computeBoundingShape();

protected:

    bool getEyeModelPositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const;

    Avatar* _owningAvatar;

    glm::vec3 _boundingCapsuleLocalOffset;
    float _boundingCapsuleRadius;
    float _boundingCapsuleHeight;

    glm::vec3 _defaultEyeModelPosition;

    float _headClipDistance;  // Near clip distance to use if no separate head model
};

#endif // hifi_SkeletonModel_h
