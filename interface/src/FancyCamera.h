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

    /*@jsdoc
     * The <code>Camera</code> API provides access to the "camera" that defines your view in desktop and HMD display modes.
     * The Vircadia camera has axes <code>x</code> = right, <code>y</code> = up, <code>-z</code> = forward.
     *
     * @namespace Camera
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     *
     * @property {Vec3} position - The position of the camera. You can set this value only when the camera is in independent 
     *     mode.
     * @property {Quat} orientation - The orientation of the camera. You can set this value only when the camera is in 
     *     independent mode.
     * @property {Camera.Mode} mode - The camera mode.
     * @property {ViewFrustum} frustum - The camera frustum.
     * @property {Uuid} cameraEntity - The ID of the entity that is used for the camera position and orientation when the 
     *     camera is in entity mode.
     * @property {boolean} captureMouse - The mouse capture state.  When <code>true</code>, the mouse is invisible and cannot leave the bounds of
     * Interface, as long as Interface is the active window and no menu item is selected.  When <code>false</code>, the mouse
     * behaves normally.
     * @property {number} sensitivity - The current camera sensitivity.  Must be positive.
     */
    Q_PROPERTY(QUuid cameraEntity READ getCameraEntity WRITE setCameraEntity)

public:
    FancyCamera() : Camera() {}

    EntityItemPointer getCameraEntityPointer() const { return _cameraEntity; }
    PickRay computePickRay(float x, float y) const override;


public slots:
    /*@jsdoc
     * Gets the ID of the entity that the camera is set to follow (i.e., use the position and orientation from) when it's in 
     * entity mode. You can also get the entity ID using the {@link Camera|Camera.cameraEntity} property.
     * @function Camera.getCameraEntity
     * @returns {Uuid} The ID of the entity that the camera is set to follow when in entity mode; <code>null</code> if no 
     *     camera entity has been set.
     */
    QUuid getCameraEntity() const;

    /*@jsdoc
     * Sets the entity that the camera should follow (i.e., use the position and orientation from) when it's in entity mode. 
     * You can also set the entity using the {@link Camera|Camera.cameraEntity} property.
     * @function Camera.setCameraEntity
     * @param {Uuid} entityID - The entity that the camera should follow when it's in entity mode.
     * @example <caption>Move your camera to the position and orientation of the closest entity.</caption>
     * Camera.setModeString("entity");
     * var entity = Entities.findClosestEntity(MyAvatar.position, 100.0);
     * Camera.setCameraEntity(entity);
     */
    void setCameraEntity(QUuid entityID);

private:
    EntityItemPointer _cameraEntity;
};

#endif // hifi_FancyCamera_h
