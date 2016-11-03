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

class Camera : public QObject {
    Q_OBJECT
    
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

    PickRay computePickRay(float x, float y);

    // These only work on independent cameras
    /// one time change to what the camera is looking at
    void lookAt(const glm::vec3& value);

    /// fix what the camera is looking at, and keep the camera looking at this even if position changes
    void keepLookingAt(const glm::vec3& value);

    /// stops the keep looking at feature, doesn't change what's being looked at, but will stop camera from
    /// continuing to update it's orientation to keep looking at the item
    void stopLooking() { _isKeepLookingAt = false; }

signals:
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
