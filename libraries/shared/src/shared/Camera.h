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

#include "../GLMHelpers.h"
#include "../RegisteredMetaTypes.h"
#include "../ViewFrustum.h"

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

    /**jsdoc
     * The Camera API provides access to the "camera" that defines your view in desktop and HMD modes.
     *
     * <p />
     *
     * <h5>Modes:</h5>
     *
     * <p>Camera modes affect the position of the camera and the controls for camera movement. The camera can be in one of the
     * following modes:</p>
     *
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
     *       <td>The camera is positioned such that you have the same view as your avatar. The camera moves and rotates with 
     *       your avatar.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Third&nbsp;Person</strong></td>
     *       <td><code>"third&nbsp;person"</code></td>
     *       <td>The camera is positioned such that you have a view from just behind your avatar. The camera moves and rotates 
     *       with your avatar.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>Mirror</strong></td>
     *       <td><code>"mirror"</code></td>
     *       <td>The camera is positioned such that you are looking directly at your avatar. The camera moves and rotates with 
     *       your avatar.</td>
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
     *       <td>The camera's position and orientation are set to be the same as a specified entity's, and move with the entity
     *       as it moves.
     *     </tr>
     *   </tbody>
     * </table>
     *
     * <p>The camera mode can be changed using <code>Camera.mode</code> and {@link Camera.setModeString}, and Interface's "View" 
     * menu.</p>
     *
     * @namespace Camera
     * @property position {Vec3} The position of the camera. You can set this value only when the camera is in entity mode.
     * @property orientation {Quat} The orientation of the camera. You can set this value only when the camera is in entity 
     *     mode.
     * @property mode {string} The camera mode.
     * @property frustum {ViewFrustum} The camera frustum.
     * @property cameraEntity {Uuid} The ID of the entity that is used for the camera position and orientation when the camera
     *     is in entity mode.
     */
    // FIXME: The cameraEntity property definition is copied from FancyCamera.h.
    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
    Q_PROPERTY(glm::quat orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(QString mode READ getModeString WRITE setModeString)
    Q_PROPERTY(QVariantMap frustum READ getViewFrustum CONSTANT)

public:
    Camera();

    void initialize(); // instantly put the camera at the ideal position and orientation.

    void update();

    CameraMode getMode() const { return _mode; }
    void setMode(CameraMode m);

    void loadViewFrustum(ViewFrustum& frustum) const;

    const glm::mat4& getTransform() const { return _transform; }
    void setTransform(const glm::mat4& transform);

    const glm::mat4& getProjection() const { return _projection; }
    void setProjection(const glm::mat4& projection);

    QVariantMap getViewFrustum();

public slots:
    /**jsdoc
     * Get the current camera mode. You can also get the mode using the <code>Camera.mode</code> property.
     * @function Camera.getModeString
     * @returns {string} The current camera mode.
     */
    QString getModeString() const;
    
    /**jsdoc
    * Set the camera mode. You can also set the mode using the <code>Camera.mode</code> property.
    * @function Camera.setModeString
    * @param {string} mode - The mode to set the camera to.
    */
    void setModeString(const QString& mode);

    /**jsdoc
    * Get the current camera position. You can also get the position using the <code>Camera.position</code> property.
    * @function Camera.getPosition
    * @returns {Vec3} The current camera position.
    */
    glm::vec3 getPosition() const { return _position; }

    /**jsdoc
    * Set the camera position. You can also set the position using the <code>Camera.position</code> property. Only works if the
    *     camera is in independent mode.
    * @function Camera.setPosition
    * @param {Vec3} position - The position to set the camera at.
    */
    void setPosition(const glm::vec3& position);

    /**jsdoc
    * Get the current camera orientation. You can also get the orientation using the <code>Camera.orientation</code> property.
    * @function Camera.getOrientation
    * @returns {Quat} The current camera orientation.
    */
    glm::quat getOrientation() const { return _orientation; }

    /**jsdoc
    * Set the camera orientation. You can also set the orientation using the <code>Camera.orientation</code> property. Only
    *     works if the camera is in independent mode.
    * @function Camera.setOrientation
    * @param {Quat} orientation - The orientation to set the camera to.
    */
    void setOrientation(const glm::quat& orientation);

    /**jsdoc
     * Compute a {@link PickRay} based on the current camera configuration and the specified <code>x, y</code> position on the 
     *     screen. The {@link PickRay} can be used in functions such as {@link Entities.findRayIntersection} and 
     *     {@link Overlays.findRayIntersection}.
     * @function Camera.computePickRay
     * @param {number} x - X-coordinate on screen.
     * @param {number} y - Y-coordinate on screen.
     * @returns {PickRay} The computed {@link PickRay}.
     * @example <caption>Use a PickRay to detect mouse clicks on entities.</caption>
     * function onMousePressEvent(event) {
     *     var pickRay = Camera.computePickRay(event.x, event.y);
     *     var intersection = Entities.findRayIntersection(pickRay);
     *     if (intersection.intersects) {
     *         print ("You clicked on entity " + intersection.entityID);
     *     }
     * }
     *
     * Controller.mousePressEvent.connect(onMousePressEvent);
     */
    virtual PickRay computePickRay(float x, float y) const = 0;

    /**jsdoc
     * Rotate the camera to look at the specified <code>position</code>. Only works if the camera is in independent mode.
     * @function Camera.lookAt
     * @param {Vec3} position - Position to look at.
     * @example <caption>Rotate your camera to look at entities as you click on them with your mouse.</caption>
     * function onMousePressEvent(event) {
     *     var pickRay = Camera.computePickRay(event.x, event.y);
     *     var intersection = Entities.findRayIntersection(pickRay);
     *     if (intersection.intersects) {
     *         // Switch to independent mode.
     *         Camera.mode = "independent";
     *         // Look at the entity that was clicked.
     *         var properties = Entities.getEntityProperties(intersection.entityID, "position");
     *         Camera.lookAt(properties.position);
     *     }
     * }
     *
     * Controller.mousePressEvent.connect(onMousePressEvent);
     */
    void lookAt(const glm::vec3& position);

    /**jsdoc
     * Set the camera to continue looking at the specified <code>position</code> even while the camera moves. Only works if the 
     * camera is in independent mode.
     * @function Camera.keepLookingAt
     * @param {Vec3} position - Position to keep looking at.
     */
    void keepLookingAt(const glm::vec3& position);

    /**jsdoc
     * Stops the camera from continually looking at the position that was set with <code>Camera.keepLookingAt</code>.
     * @function Camera.stopLookingAt
     */
    void stopLooking() { _isKeepLookingAt = false; }

signals:
    /**jsdoc
     * Triggered when the camera mode changes.
     * @function Camera.modeUpdated
     * @returns {Signal}
     * @example <caption>Report camera mode changes.</caption>
     * function onCameraModeUpdated(newMode) {
     *     print("The camera mode has changed to " + newMode);
     * }
     *
     * Camera.modeUpdated.connect(onCameraModeUpdated);
     */
    void modeUpdated(const QString& newMode);

private:
    void recompose();
    void decompose();

    CameraMode _mode{ CAMERA_MODE_THIRD_PERSON };
    glm::mat4 _transform;
    glm::mat4 _projection;

    // derived
    glm::vec3 _position { 0.0f, 0.0f, 0.0f };
    glm::quat _orientation;
    bool _isKeepLookingAt{ false };
    glm::vec3 _lookingAt;
};

#endif // hifi_Camera_h
