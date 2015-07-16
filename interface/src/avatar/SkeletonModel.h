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


#include <CapsuleShape.h>
#include <Model.h>

class Avatar;
class MuscleConstraint;

/// A skeleton loaded from a model.
class SkeletonModel : public Model {
    Q_OBJECT
    
public:

    SkeletonModel(Avatar* owningAvatar, QObject* parent = NULL);
    ~SkeletonModel();
   
    virtual void initJointStates(QVector<JointState> states);

    void simulate(float deltaTime, bool fullUpdate = true);

    /// \param jointIndex index of hand joint
    /// \param shapes[out] list in which is stored pointers to hand shapes
    void getHandShapes(int jointIndex, QVector<const Shape*>& shapes) const;

    void renderIKConstraints(gpu::Batch& batch);
    
    /// Returns the index of the left hand joint, or -1 if not found.
    int getLeftHandJointIndex() const { return isActive() ? _geometry->getFBXGeometry().leftHandJointIndex : -1; }
    
    /// Returns the index of the right hand joint, or -1 if not found.
    int getRightHandJointIndex() const { return isActive() ? _geometry->getFBXGeometry().rightHandJointIndex : -1; }

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
    
    /// Returns the rotation of the neck joint's parent from default orientation
    /// \return whether or not the neck was found
    bool getNeckParentRotationFromDefaultOrientation(glm::quat& neckParentRotation) const;
    
    /// Retrieve the positions of up to two eye meshes.
    /// \return whether or not both eye meshes were found
    bool getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const;

    /// Gets the default position of the mid eye point in model frame coordinates.
    /// \return whether or not the head was found.
    glm::vec3 getDefaultEyeModelPosition() const;

    /// skeleton offset caused by moving feet
    void updateStandingFoot();
    const glm::vec3& getStandingOffset() const { return _standingOffset; }

    void computeBoundingShape(const FBXGeometry& geometry);
    void renderBoundingCollisionShapes(gpu::Batch& batch, float alpha);
    float getBoundingShapeRadius() const { return _boundingShape.getRadius(); }
    const CapsuleShape& getBoundingShape() const { return _boundingShape; }
    const glm::vec3 getBoundingShapeOffset() const { return _boundingShapeLocalOffset; }

    void resetShapePositionsToDefaultPose(); // DEBUG method

    bool hasSkeleton();

    float getHeadClipDistance() const { return _headClipDistance; }

    void setIsFirstPerson(bool value) { _isFirstPerson = value; }
    bool getIsFirstPerson() const { return _isFirstPerson; }

    virtual void onInvalidate() override;

signals:

    void skeletonLoaded();

protected:

    void buildShapes();

    /// \param jointIndex index of joint in model
    /// \param position position of joint in model-frame
    void applyHandPosition(int jointIndex, const glm::vec3& position);
    
    void applyPalmData(int jointIndex, PalmData& palm);
    
    /// Updates the state of the joint at the specified index.
    virtual void updateJointState(int index);   
    
    void maybeUpdateLeanRotation(const JointState& parentState, JointState& state);
    void maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    void maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);

    void cauterizeHead();
    void initHeadBones();
    void invalidateHeadBones();

private:

    void renderJointConstraints(gpu::Batch& batch, int jointIndex);
    void renderOrientationDirections(int jointIndex, glm::vec3 position, const glm::quat& orientation, float size);
    
    struct OrientationLineIDs {
        int _up;
        int _front;
        int _right;
    };
    QHash<int, OrientationLineIDs> _jointOrientationLines;
    int _triangleFanID;

    /// \param jointIndex index of joint in model
    /// \param position position of joint in model-frame
    /// \param rotation rotation of joint in model-frame
    void setHandPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation);

    bool getEyeModelPositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const;
    
    Avatar* _owningAvatar;

    CapsuleShape _boundingShape;
    glm::vec3 _boundingShapeLocalOffset;

    glm::vec3 _defaultEyeModelPosition;
    int _standingFoot;
    glm::vec3 _standingOffset;
    glm::vec3 _clampedFootPosition;

    float _headClipDistance;  // Near clip distance to use if no separate head model

    bool _isFirstPerson;
    std::vector<int> _headBones;
};

#endif // hifi_SkeletonModel_h
