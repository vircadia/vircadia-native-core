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
     * @namespace
     * @augments Camera
     */

     // FIXME: JSDoc 3.5.5 doesn't augment @property definitions. The following definition is repeated in Camera.h.
     /**jsdoc
     * @property cameraEntity {Uuid} The ID of the entity that the camera position and orientation follow when the camera is in
     *     entity mode.
     */
    Q_PROPERTY(QUuid cameraEntity READ getCameraEntity WRITE setCameraEntity)

public:
    FancyCamera() : Camera() {}

    EntityItemPointer getCameraEntityPointer() const { return _cameraEntity; }
    PickRay computePickRay(float x, float y) const override;


public slots:
   /**jsdoc
   * Get the ID of the entity that the camera is set to use the position and orientation from when it's in entity mode. You can
   *     also get the entity ID using the <code>Camera.cameraEntity</code> property.
   * @function Camera.getCameraEntity
   * @returns {Uuid} The ID of the entity that the camera is set to follow when in entity mode; <code>null</code> if no camera
   *     entity has been set.
   */
    QUuid getCameraEntity() const;

    /**jsdoc
    * Set the entity that the camera should use the position and orientation from when it's in entity mode. You can also set the
    *     entity using the <code>Camera.cameraEntity</code> property.
    * @function Camera.setCameraEntity
    * @param {Uuid} entityID The entity that the camera should follow when it's in entity mode.
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
