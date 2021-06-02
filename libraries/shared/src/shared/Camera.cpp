//
//  Camera.cpp
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Camera.h"

/*@jsdoc
 * <p>Camera modes affect the position of the camera and the controls for camera movement. The camera can be in one of the
 * following modes:</p>
 * <table>
 *   <thead>
 *     <tr>
 *       <th>Mode</th>
 *       <th>String</th>
 *       <th>Description</th>
 *     </tr>
 *   </thead>
 *   <tbody>
 *     <tr>
 *       <td><strong>First&nbsp;Person</strong></td>
 *       <td><code>"first&nbsp;person"</code></td>
 *       <td><p>The camera is positioned such that you have the same view as your avatar. The camera moves and rotates with
 *       your avatar.</p>
 *       <p><em>Legacy first person camera mode.</em></p></td>
 *     </tr>
 *     <tr>
 *       <td><strong>First&nbsp;Person&nbsp;Look&nbsp;At</strong></td>
 *       <td><code>"first&nbsp;person&nbsp;look&nbsp;at"</code></td>
 *       <td><p>The camera is positioned such that you have the same view as your avatar. The camera moves and rotates with 
 *       your avatar's head.</p>
 *       <p><em>Default first person camera mode.</em></p></td>
 *     </tr>
 *     <tr>
 *       <td><strong>Third&nbsp;Person</strong></td>
 *       <td><code>"third&nbsp;person"</code></td>
 *       <td><p>The camera is positioned such that you have a view from just behind your avatar. The camera moves and rotates 
 *       with your avatar.</p>  
 *       <p><em>Legacy third person camera camera mode.</em></p> 
 *       <pre class="prettyprint"><code>Camera.mode = "third person";</code></pre></td>
 *     </tr>
 *     <tr>
 *       <td><strong>Look&nbsp;At</strong></td>
 *       <td><code>"look&nbsp;at"</code></td>
 *       <td><p>The camera is positioned behind your avatar. The camera moves and rotates independently from your avatar. The
 *       avatar's head always faces the camera look at point.</p>
 *       <p><em>Default third person camera mode.</em></td>
 *     </tr>
 *     <tr>
 *       <td><strong>Selfie</strong></td>
 *       <td><code>"selfie"</code></td>
 *       <td><p>The camera is positioned in front of your avatar. The camera moves and rotates independently from your avatar.
 *       Your avatar's head is always facing the camera.</p>
 *       <p><em>Default "look at myself" camera mode.</em></p></td>
 *     </tr>
 *     <tr>
 *       <td><strong>Mirror</strong></td>
 *       <td><code>"mirror"</code></td>
 *       <td><p>The camera is positioned such that you are looking directly at your avatar. The camera is fixed and does not 
 *       move with your avatar.</p> 
 *       <p><em>Legacy "look at myself" behavior.</em></p>
 *       <pre class="prettyprint"><code>Camera.mode = "mirror";</code></pre></td>
 *     </tr>
 *     <tr>
 *       <td><strong>Independent</strong></td>
 *       <td><code>"independent"</code></td>
 *       <td>The camera's position and orientation don't change with your avatar movement. Instead, they can be set via 
 *       scripting.</td>
 *     </tr>
 *     <tr>
 *       <td><strong>Entity</strong></td>
 *       <td><code>"entity"</code></td>
 *       <td>The camera's position and orientation are set to be the same as a specified entity's, and move with the entity as
 *       it moves.
 *     </tr>
 *   </tbody>
 * </table>
 * @typedef {string} Camera.Mode
 */
CameraMode stringToMode(const QString& mode) {
    if (mode == "third person") {
        return CAMERA_MODE_THIRD_PERSON;
    } else if (mode == "first person") {
        return CAMERA_MODE_FIRST_PERSON;
    } else if (mode == "first person look at") {
        return CAMERA_MODE_FIRST_PERSON_LOOK_AT;
    } else if (mode == "mirror") {
        return CAMERA_MODE_MIRROR;
    } else if (mode == "independent") {
        return CAMERA_MODE_INDEPENDENT;
    } else if (mode == "entity") {
        return CAMERA_MODE_ENTITY;
    } else if (mode == "look at") {
        return CAMERA_MODE_LOOK_AT;
    } else if (mode == "selfie") {
        return CAMERA_MODE_SELFIE;
    }
    return CAMERA_MODE_NULL;
}

QString modeToString(CameraMode mode) {
    if (mode == CAMERA_MODE_THIRD_PERSON) {
        return "third person";
    } else if (mode == CAMERA_MODE_FIRST_PERSON) {
        return "first person";
    } else if (mode == CAMERA_MODE_FIRST_PERSON_LOOK_AT) {
        return "first person look at";
    } else if (mode == CAMERA_MODE_MIRROR) {
        return "mirror";
    } else if (mode == CAMERA_MODE_INDEPENDENT) {
        return "independent";
    } else if (mode == CAMERA_MODE_ENTITY) {
        return "entity";
    } else if (mode == CAMERA_MODE_LOOK_AT) {
        return "look at";
    } else if (mode == CAMERA_MODE_SELFIE) {
        return "selfie";
    }
    return "unknown";
}

Camera::Camera() : 
    _projection(glm::perspective(glm::radians(DEFAULT_FIELD_OF_VIEW_DEGREES), 16.0f/9.0f, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP))
{
}

void Camera::update() {
    if (_isKeepLookingAt) {
        lookAt(_lookingAt);
    }
    return;
}

void Camera::recompose() {
    mat4 orientation = glm::mat4_cast(_orientation);
    mat4 translation = glm::translate(mat4(), _position);
    _transform = translation * orientation;
}

void Camera::decompose() {
    _position = vec3(_transform[3]);
    _orientation = glm::quat_cast(_transform);
}

void Camera::setTransform(const glm::mat4& transform) {
    _transform = transform;
    decompose();
}

void Camera::setPosition(const glm::vec3& position) {
    _position = position;
    recompose();
    if (_isKeepLookingAt) {
        lookAt(_lookingAt);
    }
}

void Camera::setOrientation(const glm::quat& orientation) {
    _orientation = orientation;
    recompose();
    if (_isKeepLookingAt) {
        lookAt(_lookingAt);
    }
}

void Camera::setMode(CameraMode mode) {
    _mode = mode;
    emit modeUpdated(modeToString(mode));
}

void Camera::setProjection(const glm::mat4& projection) {
    _projection = projection;
}

void Camera::setModeString(const QString& mode) {
    CameraMode targetMode = stringToMode(mode);

    if (_mode != targetMode) {
        setMode(targetMode);
    }
}

QString Camera::getModeString() const {
    return modeToString(_mode);
}

void Camera::lookAt(const glm::vec3& lookAt) {
    glm::vec3 up = IDENTITY_UP;
    glm::mat4 lookAtMatrix = glm::lookAt(_position, lookAt, up);
    glm::quat orientation = glm::quat_cast(lookAtMatrix);
    orientation.w = -orientation.w; // Rosedale approved
    _orientation = orientation;
}

void Camera::keepLookingAt(const glm::vec3& point) {
    lookAt(point);
    _isKeepLookingAt = true;
    _lookingAt = point;
}

void Camera::loadViewFrustum(ViewFrustum& frustum) const {
    // We will use these below, from either the camera or head vectors calculated above
    frustum.setProjection(getProjection());

    // Set the viewFrustum up with the correct position and orientation of the camera
    frustum.setPosition(getPosition());
    frustum.setOrientation(getOrientation());

    // Ask the ViewFrustum class to calculate our corners
    frustum.calculate();
}

/*@jsdoc
 * A ViewFrustum has a "keyhole" shape: a regular frustum for stuff that is visible plus a central sphere for stuff that is
 * nearby (for physics simulation).
 *
 * @typedef {object} ViewFrustum
 * @property {Vec3} position - The location of the frustum's apex.
 * @property {Quat} orientation - The direction that the frustum is looking at.
 * @property {number} centerRadius - Center radius of the keyhole in meters.
 * @property {number} fieldOfView - Horizontal field of view in degrees.
 * @property {number} aspectRatio - Aspect ratio of the frustum.
 * @property {Mat4} projection - The projection matrix for the view defined by the frustum.
 */
QVariantMap Camera::getViewFrustum() {
    ViewFrustum frustum;
    loadViewFrustum(frustum);

    QVariantMap result;
    result["position"].setValue(frustum.getPosition());
    result["orientation"].setValue(frustum.getOrientation());
    result["projection"].setValue(frustum.getProjection());
    result["centerRadius"].setValue(frustum.getCenterRadius());
    result["fieldOfView"].setValue(frustum.getFieldOfView());
    result["aspectRatio"].setValue(frustum.getAspectRatio());

    return result;
}
