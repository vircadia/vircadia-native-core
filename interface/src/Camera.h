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
    NUM_CAMERA_MODES
};

Q_DECLARE_METATYPE(CameraMode);
static int cameraModeId = qRegisterMetaType<CameraMode>();

class Camera : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
    Q_PROPERTY(glm::quat orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(QString mode READ getModeString WRITE setModeString)
public:
    Camera();

    void initialize(); // instantly put the camera at the ideal position and rotation. 

    void update( float deltaTime );

    void setRotation(const glm::quat& rotation);
    void setProjection(const glm::mat4 & projection);
    void setMode(CameraMode m);
    
    glm::quat getRotation() const { return _rotation; }
    const glm::mat4& getProjection() const { return _projection; }
    CameraMode getMode() const { return _mode; }

public slots:
    QString getModeString() const;
    void setModeString(const QString& mode);

    glm::vec3 getPosition() const { return _position; }
    void setPosition(const glm::vec3& position);

    glm::quat getOrientation() const { return getRotation(); }
    void setOrientation(const glm::quat& orientation) { setRotation(orientation); }

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
    CameraMode _mode;
    glm::vec3 _position;
    glm::quat _rotation;
    glm::mat4 _projection;
    bool _isKeepLookingAt;
    glm::vec3 _lookingAt;
};

#endif // hifi_Camera_h
