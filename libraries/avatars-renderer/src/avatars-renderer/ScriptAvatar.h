//
//  ScriptAvatar.h
//  interface/src/avatars
//
//  Created by Stephen Birarda on 4/10/17.
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptAvatar_h
#define hifi_ScriptAvatar_h

#include <ScriptAvatarData.h>

#include "Avatar.h"

/*@jsdoc
 * Information about an avatar.
 *
 * <p>Create using {@link MyAvatar.getTargetAvatar} or {@link AvatarList.getAvatar}.</p>
 *
 * @class ScriptAvatar
 * @hideconstructor
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-assignment-client
 * @hifi-server-entity
 *
 * @property {Vec3} position - The avatar's position.
 * @property {number} scale - The target scale of the avatar without any restrictions on permissible values imposed by the
 *     domain.
 * @property {Vec3} handPosition - A user-defined hand position, in world coordinates. The position moves with the avatar but
 *    is otherwise not used or changed by Interface.
 * @property {number} bodyPitch - The pitch of the avatar's body, in degrees.
 * @property {number} bodyYaw - The yaw of the avatar's body, in degrees.
 * @property {number} bodyRoll - The roll of the avatar's body, in degrees.
 * @property {Quat} orientation - The orientation of the avatar's body.
 * @property {Quat} headOrientation - The orientation of the avatar's head.
 * @property {number} headPitch - The pitch of the avatar's head relative to the body, in degrees.
 * @property {number} headYaw - The yaw of the avatar's head relative to the body, in degrees.
 * @property {number} headRoll - The roll of the avatar's head relative to the body, in degrees.
 *
 * @property {Vec3} velocity - The linear velocity of the avatar.
 * @property {Vec3} angularVelocity - The angular velocity of the avatar.
 *
 * @property {Uuid} sessionUUID - The avatar's session ID.
 * @property {string} displayName - The avatar's display name.
 * @property {string} sessionDisplayName - The avatar's display name, sanitized and versioned, as defined by the avatar mixer.
 *     It is unique among all avatars present in the domain at the time.
 * @property {boolean} isReplicated - <span class="important">Deprecated: This property is deprecated and will be
 *     removed.</span>
 * @property {boolean} lookAtSnappingEnabled - <code>true</code> if the avatar's eyes snap to look at another avatar's eyes
 *     when the other avatar is in the line of sight and also has <code>lookAtSnappingEnabled == true</code>.
 *
 * @property {string} skeletonModelURL - The avatar's FST file.
 * @property {AttachmentData[]} attachmentData - Information on the avatar's attachments.
 *     <p class="important">Deprecated: This property is deprecated and will be removed. Use avatar entities instead.</p>
 * @property {string[]} jointNames - The list of joints in the avatar model.
 *
 * @property {number} audioLoudness - The instantaneous loudness of the audio input that the avatar is injecting into the
 *     domain.
 * @property {number} audioAverageLoudness - The rolling average loudness of the audio input that the avatar is injecting into
 *     the domain.
 *
 * @property {Mat4} sensorToWorldMatrix - The scale, rotation, and translation transform from the user's real world to the
 *     avatar's size, orientation, and position in the virtual world.
 * @property {Mat4} controllerLeftHandMatrix - The rotation and translation of the left hand controller relative to the avatar.
 * @property {Mat4} controllerRightHandMatrix - The rotation and translation of the right hand controller relative to the
 *     avatar.
 *
 * @property {Vec3} skeletonOffset - The rendering offset of the avatar.
 */
class ScriptAvatar : public ScriptAvatarData {
    Q_OBJECT

    Q_PROPERTY(glm::vec3 skeletonOffset READ getSkeletonOffset)

public:
    ScriptAvatar(AvatarSharedPointer avatarData);

public slots:

    /*@jsdoc
     * Gets the default rotation of a joint in the avatar relative to its parent.
     * <p>For information on the joint hierarchy used, see
     * <a href="https://docs.vircadia.com/create/avatars/avatar-standards.html">Avatar Standards</a>.</p>
     * @function ScriptAvatar.getDefaultJointRotation
     * @param {number} index - The joint index.
     * @returns {Quat} The default rotation of the joint if avatar data are available and the joint index is valid, otherwise
     *     {@link Quat(0)|Quat.IDENTITY}.
     */
    glm::quat getDefaultJointRotation(int index) const;

    /*@jsdoc
     * Gets the default translation of a joint in the avatar relative to its parent, in model coordinates.
     * <p><strong>Warning:</strong> These coordinates are not necessarily in meters.</p>
     * <p>For information on the joint hierarchy used, see
     * <a href="https://docs.vircadia.com/create/avatars/avatar-standards.html">Avatar Standards</a>.</p>
     * @function ScriptAvatar.getDefaultJointTranslation
     * @param {number} index - The joint index.
     * @returns {Vec3} The default translation of the joint (in model coordinates) if avatar data are available and the joint
     *     index is valid, otherwise {@link Vec3(0)|Vec3.ZERO}.
     */
    glm::vec3 getDefaultJointTranslation(int index) const;


    /*@jsdoc
     * Gets the offset applied to the avatar for rendering.
     * @function ScriptAvatar.getSkeletonOffset
     * @returns {Vec3} The skeleton offset if avatar data are available, otherwise {@link Vec3(0)|Vec3.ZERO}.
     */
    glm::vec3 getSkeletonOffset() const;


    /*@jsdoc
     * Gets the position of a joint in the avatar.
     * @function ScriptAvatar.getJointPosition
     * @param {number} index - The index of the joint.
     * @returns {Vec3} The position of the joint in world coordinates, or {@link Vec3(0)|Vec3.ZERO} if avatar data aren't
     *     available.
     */
    glm::vec3 getJointPosition(int index) const;

    /*@jsdoc
     * Gets the position of a joint in the current avatar.
     * @function ScriptAvatar.getJointPosition
     * @param {string} name - The name of the joint.
     * @returns {Vec3} The position of the joint in world coordinates, or {@link Vec3(0)|Vec3.ZERO} if avatar data aren't
     *     available.
     */
    glm::vec3 getJointPosition(const QString& name) const;

    /*@jsdoc
     * Gets the position of the current avatar's neck in world coordinates.
     * @function ScriptAvatar.getNeckPosition
     * @returns {Vec3} The position of the neck in world coordinates, or {@link Vec3(0)|Vec3.ZERO} if avatar data aren't
     *     available.
     */
    glm::vec3 getNeckPosition() const;


    /*@jsdoc
     * Gets the current acceleration of the avatar.
     * @function ScriptAvatar.getAcceleration
     * @returns {Vec3} The current acceleration of the avatar, or {@link Vec3(0)|Vec3.ZERO} if avatar data aren't available..
     */
    glm::vec3 getAcceleration() const;


    /*@jsdoc
     * Gets the ID of the entity or avatar that the avatar is parented to.
     * @function ScriptAvatar.getParentID
     * @returns {Uuid} The ID of the entity or avatar that the avatar is parented to. {@link Uuid(0)|Uuid.NULL} if not parented
     *     or avatar data aren't available.
     */
    QUuid getParentID() const;

    /*@jsdoc
     * Gets the joint of the entity or avatar that the avatar is parented to.
     * @function ScriptAvatar.getParentJointIndex
     * @returns {number} The joint of the entity or avatar that the avatar is parented to. <code>65535</code> or
     *     <code>-1</code> if parented to the entity or avatar's position and orientation rather than a joint, or avatar data
     *     aren't available.
     */
    quint16 getParentJointIndex() const;


    /*@jsdoc
     * Gets information on all the joints in the avatar's skeleton.
     * @function ScriptAvatar.getSkeleton
     * @returns {SkeletonJoint[]} Information about each joint in the avatar's skeleton.
     */
    QVariantList getSkeleton() const;


    /*@jsdoc
     * @function ScriptAvatar.getSimulationRate
     * @param {AvatarSimulationRate} [rateName=""] - Rate name.
     * @returns {number} Simulation rate in Hz, or <code>0.0</code> if avatar data aren't available.
     * @deprecated This function is deprecated and will be removed.
     */
    float getSimulationRate(const QString& rateName = QString("")) const;


    /*@jsdoc
     * Gets the position of the left palm in world coordinates.
     * @function ScriptAvatar.getLeftPalmPosition
     * @returns {Vec3} The position of the left palm in world coordinates, or {@link Vec3(0)|Vec3.ZERO} if avatar data aren't
     *     available.
     */
    glm::vec3 getLeftPalmPosition() const;

    /*@jsdoc
     * Gets the rotation of the left palm in world coordinates.
     * @function ScriptAvatar.getLeftPalmRotation
     * @returns {Quat} The rotation of the left palm in world coordinates, or {@link Quat(0)|Quat.IDENTITY} if the avatar data
     *     aren't available.
     */
    glm::quat getLeftPalmRotation() const;

    /*@jsdoc
     * Gets the position of the right palm in world coordinates.
     * @function ScriptAvatar.getLeftPalmPosition
     * @returns {Vec3} The position of the right palm in world coordinates, or {@link Vec3(0)|Vec3.ZERO} if avatar data aren't
     *     available.
     */
    glm::vec3 getRightPalmPosition() const;

    /*@jsdoc
     * Gets the rotation of the right palm in world coordinates.
     * @function ScriptAvatar.getLeftPalmRotation
     * @returns {Quat} The rotation of the right palm in world coordinates, or {@link Quat(0)|Quat.IDENTITY} if the avatar data
     *     aren't available.
     */
    glm::quat getRightPalmRotation() const;

private:
    std::shared_ptr<Avatar> lockAvatar() const;
};

#endif // hifi_ScriptAvatar_h
