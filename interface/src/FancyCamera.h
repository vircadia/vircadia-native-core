//
//  FancyCamera.h
//  interface/src
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FancyCamera_h
#define hifi_FancyCamera_h

#include <shared/Camera.h>

#include <EntityTypes.h>

class FancyCamera : public Camera {
    Q_OBJECT

    /**jsdoc
     * @namespace Camera
     * @property cameraEntity {EntityID} The position and rotation properties of
     *     the entity specified by this ID are then used as the camera's position and
     *     orientation. Only works when <code>mode</code> is "entity".
     */
    Q_PROPERTY(QUuid cameraEntity READ getCameraEntity WRITE setCameraEntity)

public:
    FancyCamera() : Camera() {}

    EntityItemPointer getCameraEntityPointer() const { return _cameraEntity; }
    PickRay computePickRay(float x, float y) const override;


public slots:
    QUuid getCameraEntity() const;
    void setCameraEntity(QUuid entityID);

private:
    EntityItemPointer _cameraEntity;
};

#endif // hifi_FancyCamera_h
