//
//  Camera.h
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
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
    CAMERA_MODE_FIRST_PERSON_LOOK_AT,
    CAMERA_MODE_FIRST_PERSON,
    CAMERA_MODE_MIRROR,
    CAMERA_MODE_INDEPENDENT,
    CAMERA_MODE_ENTITY,
    CAMERA_MODE_LOOK_AT,
    CAMERA_MODE_SELFIE,
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
    Q_PROPERTY(QString mode READ getModeString WRITE setModeString NOTIFY modeUpdated)
    Q_PROPERTY(QVariantMap frustum READ getViewFrustum CONSTANT)
    Q_PROPERTY(bool captureMouse READ getCaptureMouse WRITE setCaptureMouse NOTIFY captureMouseChanged)
    Q_PROPERTY(float sensitivity READ getSensitivity WRITE setSensitivity)

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
    /*@jsdoc
     * Gets the current camera mode. You can also get the mode using the {@link Camera|Camera.mode} property.
     * @function Camera.getModeString
     * @returns {Camera.Mode} The current camera mode.
     */
    QString getModeString() const;
    
    /*@jsdoc
     * Sets the camera mode. You can also set the mode using the {@link Camera|Camera.mode} property.
     * @function Camera.setModeString
     * @param {Camera.Mode} mode - The mode to set the camera to.
     */
    void setModeString(const QString& mode);

    /*@jsdoc
     * Gets the current camera position. You can also get the position using the {@link Camera|Camera.position} property.
     * @function Camera.getPosition
     * @returns {Vec3} The current camera position.
     */
    glm::vec3 getPosition() const { return _position; }

    /*@jsdoc
     * Sets the camera position. You can also set the position using the {@link Camera|Camera.position} property. Only works if 
     * the camera is in independent mode.
     * @function Camera.setPosition
     * @param {Vec3} position - The position to set the camera at.
     */
    void setPosition(const glm::vec3& position);

    /*@jsdoc
     * Gets the current camera orientation. You can also get the orientation using the {@link Camera|Camera.orientation} 
     * property.
     * @function Camera.getOrientation
     * @returns {Quat} The current camera orientation.
     */
    glm::quat getOrientation() const { return _orientation; }

    /*@jsdoc
     * Sets the camera orientation. You can also set the orientation using the {@link Camera|Camera.orientation} property. Only
     * works if the camera is in independent mode.
     * @function Camera.setOrientation
     * @param {Quat} orientation - The orientation to set the camera to.
     */
    void setOrientation(const glm::quat& orientation);

    /*@jsdoc
     * Gets the current mouse capture state.
     * @function Camera.getCaptureMouse
     * @returns {boolean} <code>true</code> if the mouse is captured (is invisible and cannot leave the bounds of Interface,
     * if Interface is the active window and no menu item is selected), <code>false</code> if the mouse is behaving normally.
     */
    bool getCaptureMouse() const { return _captureMouse; }

    /*@jsdoc
     * Sets the mouse capture state.  When <code>true</code>, the mouse is invisible and cannot leave the bounds of
     * Interface, as long as Interface is the active window and no menu item is selected.  When <code>false</code>, the mouse
     * behaves normally.
     * @function Camera.setCaptureMouse
     * @param {boolean} captureMouse - <code>true</code> to capture the mouse, <code>false</code> to release the mouse.
     */
    void setCaptureMouse(bool captureMouse) { _captureMouse = captureMouse; emit captureMouseChanged(captureMouse); }

    /*@jsdoc
     * Gets the current camera sensitivity.
     * @function Camera.getSensitivity
     * @returns {number} The current camera sensitivity.  Must be positive.
     */
    float getSensitivity() const { return _sensitivity; }

    /*@jsdoc
     * Sets the camera sensitivity.  Higher values mean that the camera will be more sensitive to mouse movements.
     * @function Camera.setSensitivity
     * @param {number} sensitivity - The desired camera sensitivity.  Must be positive.
     */
    void setSensitivity(float sensitivity) { _sensitivity = glm::max(0.0f, sensitivity); }

    /*@jsdoc
     * Computes a {@link PickRay} based on the current camera configuration and the specified <code>x, y</code> position on the 
     * screen. The {@link PickRay} can be used in functions such as {@link Entities.findRayIntersection} and 
     * {@link Overlays.findRayIntersection}.
     * @function Camera.computePickRay
     * @param {number} x - X-coordinate on screen.
     * @param {number} y - Y-coordinate on screen.
     * @returns {PickRay} The computed {@link PickRay}.
     * @example <caption>Use a PickRay to detect mouse clicks on entities.</caption>
     * function onMousePressEvent(event) {
     *     var pickRay = Camera.computePickRay(event.x, event.y);
     *     var intersection = Entities.findRayIntersection(pickRay);
     *     if (intersection.intersects) {
     *         print("You clicked on entity " + intersection.entityID);
     *     }
     * }
     *
     * Controller.mousePressEvent.connect(onMousePressEvent);
     */
    virtual PickRay computePickRay(float x, float y) const = 0;

    /*@jsdoc
     * Rotates the camera to look at the specified <code>position</code>. Only works if the camera is in independent mode.
     * @function Camera.lookAt
     * @param {Vec3} position - The position to look at.
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

    /*@jsdoc
     * Sets the camera to continue looking at the specified <code>position</code> even while the camera moves. Only works if 
     * the camera is in independent mode.
     * @function Camera.keepLookingAt
     * @param {Vec3} position - The position to keep looking at.
     */
    void keepLookingAt(const glm::vec3& position);

    /*@jsdoc
     * Stops the camera from continually looking at the position that was set with {@link Camera.keepLookingAt}.
     * @function Camera.stopLookingAt
     */
    void stopLooking() { _isKeepLookingAt = false; }

signals:
    /*@jsdoc
     * Triggered when the camera mode changes.
     * @function Camera.modeUpdated
     * @param {Camera.Mode} newMode - The new camera mode.
     * @returns {Signal}
     * @example <caption>Report camera mode changes.</caption>
     * function onCameraModeUpdated(newMode) {
     *     print("The camera mode has changed to " + newMode);
     * }
     *
     * Camera.modeUpdated.connect(onCameraModeUpdated);
     */
    void modeUpdated(const QString& newMode);

    /*@jsdoc
     * Triggered when the camera mouse capture state changes.
     * @function Camera.captureMouseChanged
     * @param {boolean} newCaptureMouse - The new mouse capture state.
     * @returns {Signal}
     * @example <caption>Report mouse capture state changes.</caption>
     * function onCaptureMouseChanged(newCaptureMouse) {
     *     print("The mouse capture has changed to " + newCaptureMouse);
     * }
     *
     * Camera.captureMouseChanged.connect(onCaptureMouseChanged);
     */
    void captureMouseChanged(bool newCaptureMouse);

private:
    void recompose();
    void decompose();

    CameraMode _mode{ CAMERA_MODE_LOOK_AT };
    glm::mat4 _transform;
    glm::mat4 _projection;

    // derived
    glm::vec3 _position { 0.0f, 0.0f, 0.0f };
    glm::quat _orientation;
    bool _isKeepLookingAt{ false };
    glm::vec3 _lookingAt;

    bool _captureMouse { false };
    float _sensitivity { 1.0f };
};

#endif // hifi_Camera_h
