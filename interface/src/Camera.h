//
//  Camera.h
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Camera_h
#define hifi_Camera_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <RegisteredMetaTypes.h>
#include <ViewFrustum.h>

enum CameraMode
{
    CAMERA_MODE_NULL = -1,
    CAMERA_MODE_THIRD_PERSON,
    CAMERA_MODE_FIRST_PERSON,
    CAMERA_MODE_MIRROR,
    CAMERA_MODE_INDEPENDENT,
    CAMERA_MODE_ENTITY,
    NUM_CAMERA_MODES
};

Q_DECLARE_METATYPE(CameraMode);

#if defined(__GNUC__) && !defined(__clang__)
__attribute__((unused))
#endif
static int cameraModeId = qRegisterMetaType<CameraMode>();

/**jsdoc
 * @global
 */
class Camera : public QObject {
    Q_OBJECT

    /**jsdoc
     * @namespace Camera
     * @property position {Vec3} The position of the camera.
     * @property orientation {Quat} The orientation of the camera.
     * @property mode {string} The current camera mode.
     * @property cameraEntity {EntityID} The position and rotation properties of
     *     the entity specified by this ID are then used as the camera's position and
     *     orientation. Only works when <code>mode</code> is "entity".
     * @property frustum {Object} The frustum of the camera.
     */
    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
    Q_PROPERTY(glm::quat orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(QString mode READ getModeString WRITE setModeString)
    Q_PROPERTY(QUuid cameraEntity READ getCameraEntity WRITE setCameraEntity)
    Q_PROPERTY(QVariantMap frustum READ getViewFrustum CONSTANT)

public:
    Camera();

    void initialize(); // instantly put the camera at the ideal position and orientation.

    void update( float deltaTime );

    CameraMode getMode() const { return _mode; }
    void setMode(CameraMode m);

    void loadViewFrustum(ViewFrustum& frustum) const;
    ViewFrustum toViewFrustum() const;

    EntityItemPointer getCameraEntityPointer() const { return _cameraEntity; }

    const glm::mat4& getTransform() const { return _transform; }
    void setTransform(const glm::mat4& transform);

    const glm::mat4& getProjection() const { return _projection; }
    void setProjection(const glm::mat4& projection);

    QVariantMap getViewFrustum();

public slots:
    QString getModeString() const;
    void setModeString(const QString& mode);

    glm::vec3 getPosition() const { return _position; }
    void setPosition(const glm::vec3& position);

    glm::quat getOrientation() const { return _orientation; }
    void setOrientation(const glm::quat& orientation);

    QUuid getCameraEntity() const;
    void setCameraEntity(QUuid entityID);

    /**jsdoc
     * Compute a {PickRay} based on the current camera configuration and the position x,y on the screen.
     * @function Camera.computePickRay
     * @param {float} x
     * @param {float} y
     * @return {PickRay}
     */
    PickRay computePickRay(float x, float y);

    /**jsdoc
     * Set the camera to look at position <code>position</code>. Only works while in <code>independent</code>.
     * camera mode.
     * @function Camera.lookAt
     * @param {Vec3} position position to look at
     */
    void lookAt(const glm::vec3& position);

    /**jsdoc
     * Set the camera to continue looking at position <code>position</code>.
     * Only works while in `independent` camera mode.
     * @function Camera.keepLookingAt
     * @param {Vec3} position position to look at
     */
    void keepLookingAt(const glm::vec3& position);

    /**jsdoc
     * Stops the camera from continually looking at a position that was set with
     * `keepLookingAt`
     * @function Camera.stopLookingAt
     */
    void stopLooking() { _isKeepLookingAt = false; }

signals:
    /**jsdoc
     * Triggered when camera mode has changed.
     * @function Camera.modeUpdated
     * @return {Signal}
     */
    void modeUpdated(const QString& newMode);

private:
    void recompose();
    void decompose();

    CameraMode _mode{ CAMERA_MODE_THIRD_PERSON };
    glm::mat4 _transform;
    glm::mat4 _projection;

    // derived
    glm::vec3 _position;
    glm::quat _orientation;
    bool _isKeepLookingAt{ false };
    glm::vec3 _lookingAt;
    EntityItemPointer _cameraEntity;
};

#endif // hifi_Camera_h
