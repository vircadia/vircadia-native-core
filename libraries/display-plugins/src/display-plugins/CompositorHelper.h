//
//  Created by Bradley Austin Davis Arnold on 2015/06/13
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_display_plugins_Compositor_h
#define hifi_display_plugins_Compositor_h

#include <atomic>
#include <cstdint>

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>

#include <GLMHelpers.h>
#include <Transform.h>
#include <DependencyManager.h>

#include "DisplayPlugin.h"

class ReticleInterface;

const float HUD_RADIUS = 1.5f;
const float DEFAULT_RETICLE_DEPTH = HUD_RADIUS;

const float MAGNIFY_WIDTH = 220.0f;
const float MAGNIFY_HEIGHT = 100.0f;
const float MAGNIFY_MULT = 2.0f;

// Handles the drawing of the overlays to the screen
// TODO, move divide up the rendering, displaying and input handling
// facilities of this class
class CompositorHelper : public QObject, public Dependency {
    Q_OBJECT

    Q_PROPERTY(float alpha READ getAlpha WRITE setAlpha NOTIFY alphaChanged)
    Q_PROPERTY(bool reticleOverDesktop READ getReticleOverDesktop WRITE setReticleOverDesktop)
public:
    static const uvec2 VIRTUAL_SCREEN_SIZE;
    static const QRect VIRTUAL_SCREEN_RECOMMENDED_OVERLAY_RECT;
    static const float VIRTUAL_UI_ASPECT_RATIO;
    static const vec2 VIRTUAL_UI_TARGET_FOV;
    static const vec2 MOUSE_EXTENTS_ANGULAR_SIZE;
    static const vec2 MOUSE_EXTENTS_PIXELS;

    CompositorHelper();
    void setRenderingWidget(QWidget* widget) { _renderingWidget = widget; }

    bool calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const;
    bool calculateParabolaUICollisionPoint(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration, glm::vec3& result, float& parabolicDistance) const;

    bool isHMD() const;
    bool fakeEventActive() const { return _fakeMouseEvent; }

    // Converter from one frame of reference to another.
    // Frame of reference:
    // Spherical: Polar coordinates that gives the position on the sphere we project on (yaw,pitch)
    // Overlay: Position on the overlay (x,y)
    glm::vec2 sphericalToOverlay(const glm::vec2 & sphericalPos) const;
    glm::vec2 overlayToSpherical(const glm::vec2 & overlayPos) const;
    void computeHmdPickRay(const glm::vec2& cursorPos, glm::vec3& origin, glm::vec3& direction) const;

    glm::vec2 overlayFromSphereSurface(const glm::vec3& sphereSurfacePoint) const;
    glm::vec3 sphereSurfaceFromOverlay(const glm::vec2& overlay) const;

    void setCameraBaseTransform(const Transform& transform) { _cameraBaseTransform = transform; }
    const Transform& getCameraBaseTransform() const { return _cameraBaseTransform; }

    void setModelTransform(const Transform& transform) { _modelTransform = transform; }
    const Transform& getModelTransform() const { return _modelTransform; }
    glm::mat4 getUiTransform() const;

    float getAlpha() const { return _alpha; }
    void setAlpha(float alpha) { if (alpha != _alpha) { emit alphaChanged();  _alpha = alpha; } }

    bool getReticleVisible() const { return _reticleVisible; }
    void setReticleVisible(bool visible) { _reticleVisible = visible; }

    float getReticleDepth() const { return _reticleDepth; }
    void setReticleDepth(float depth) { _reticleDepth = depth; }
    void resetReticleDepth() { _reticleDepth = DEFAULT_RETICLE_DEPTH; }

    glm::vec2 getReticlePosition() const;
    void setReticlePosition(const glm::vec2& position, bool sendFakeEvent = true);

    glm::vec2 getReticleMaximumPosition() const;

    glm::mat4 getReticleTransform(const glm::mat4& eyePose = glm::mat4(), const glm::vec3& headPosition = glm::vec3()) const;
    glm::mat4 getPoint2DTransform(const glm::vec2& point, float sizeX , float sizeY) const;

    ReticleInterface* getReticleInterface() { return _reticleInterface; }

    /// return value - true means the caller should not process the event further
    bool handleRealMouseMoveEvent(bool sendFakeEvent = true);
    void handleLeaveEvent();
    QPointF getMouseEventPosition(QMouseEvent* event);

    bool shouldCaptureMouse() const;

    bool getAllowMouseCapture() const { return _allowMouseCapture; }
    void setAllowMouseCapture(bool capture);

    /// if the reticle is pointing to a system overlay (a dialog box for example) then the function returns true otherwise false
    bool getReticleOverDesktop() const;
    void setReticleOverDesktop(bool value) { _isOverDesktop = value; }

    void setDisplayPlugin(const DisplayPluginPointer& displayPlugin) { _currentDisplayPlugin = displayPlugin; }
    void setFrameInfo(uint32_t frame, const glm::mat4& camera, const glm::mat4& sensorToWorldMatrix) {
        _currentCamera = camera;
        _sensorToWorldMatrix = sensorToWorldMatrix;
    }

signals:
    void allowMouseCaptureChanged();
    void alphaChanged();

protected slots:
    void sendFakeMouseEvent();

private:
    void updateTooltips();

    DisplayPluginPointer _currentDisplayPlugin;
    glm::mat4 _currentCamera;
    glm::mat4 _sensorToWorldMatrix;
    QWidget* _renderingWidget{ nullptr };

    //// Support for hovering and tooltips
    //static EntityItemID _noItemId;
    //EntityItemID _hoverItemId { _noItemId };

    //QString _hoverItemTitle;
    //QString _hoverItemDescription;
    //quint64 _hoverItemEnterUsecs { 0 };

    bool _isOverDesktop { true };
    float _textureFov { VIRTUAL_UI_TARGET_FOV.y };
    float _textureAspectRatio { VIRTUAL_UI_ASPECT_RATIO };

    float _alpha { 1.0f };

    int _previousBorderWidth { -1 };
    int _previousBorderHeight { -1 };

    Transform _modelTransform;
    Transform _cameraBaseTransform;

    std::unique_ptr<QPropertyAnimation> _alphaPropertyAnimation;

    std::atomic<bool> _reticleVisible { true };
    std::atomic<float> _reticleDepth { DEFAULT_RETICLE_DEPTH };

    // NOTE: when the compositor is running in HMD mode, it will control the reticle position as a custom
    // application specific position, when it's in desktop mode, the reticle position will simply move
    // the system mouse.
    glm::vec2 _reticlePositionInHMD { 0.0f, 0.0f };
    mutable QRecursiveMutex _reticleLock;

    QPointF _lastKnownRealMouse;
    bool _ignoreMouseMove { false };

    bool _reticleOverQml { false };

    std::atomic<bool> _allowMouseCapture { true };

    bool _fakeMouseEvent { false };

    ReticleInterface* _reticleInterface { nullptr };
};

/*@jsdoc
 * The <code>Reticle</code> API provides access to the mouse cursor. The cursor may be an arrow or a reticle circle, depending 
 * on Interface settings. The mouse cursor is visible in HMD mode if controllers aren't being used.
 *
 * @namespace Reticle
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {boolean} allowMouseCapture=true - <code>true</code> if the mouse cursor will be captured when in HMD mode and the 
 *     Interface window content (excluding menus) has focus, <code>false</code> if the mouse cursor will not be captured.
 * @property {number} depth - The depth (distance) that the reticle is displayed at relative to the HMD view, in HMD mode.
 * @property {Vec2} maximumPosition - The maximum reticle coordinates on the display device in desktop mode or the HUD surface 
 *     in HMD mode. (The minimum reticle coordinates on the desktop display device or HUD surface are <code>0</code>, 
 *     <code>0</code>.) <em>Read-only.</em>
 * @property {boolean} mouseCaptured - <code>true</code> if the mouse cursor is captured, displaying only in Interface and 
 *     not on the rest of the desktop. The mouse cursor may be captured when in HMD mode and the Interface window content 
 *     (excluding menu items) has focus, if capturing is enabled (<code>allowMouseCapture</code> is <code>true</code>). 
 *     <em>Read-only.</em>
 * @property {boolean} pointingAtSystemOverlay - <code>true</code> if the mouse cursor is pointing at UI in the Interface 
 *     window in desktop mode or on the HUD surface in HMD mode, <code>false</code> if it isn't. <em>Read-only.</em>
 * @property {Vec2} position - The position of the cursor. This is the position relative to the Interface window in desktop 
 *     mode, and the HUD surface in HMD mode.
 *     <p><strong>Note:</strong> The position values may be negative.</p>
 * @property {number} scale=1 - The scale of the reticle circle in desktop mode, and the arrow and reticle circle in HMD mode. 
 *     (Does not affect the size of the arrow in desktop mode.)
 * @property {boolean} visible=true - <code>true</code> if the reticle circle is visible in desktop mode, and the arrow or 
 *     reticle circle are visible in HMD mode; <code>false</code> otherwise. (Does not affect the visibility of the mouse 
 *     pointer in desktop mode.)
 */
// Scripting interface available to control the Reticle
class ReticleInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant position READ getPosition WRITE setPosition)
    Q_PROPERTY(bool visible READ getVisible WRITE setVisible)
    Q_PROPERTY(float depth READ getDepth WRITE setDepth)
    Q_PROPERTY(float scale READ getScale WRITE setScale)
    Q_PROPERTY(glm::vec2 maximumPosition READ getMaximumPosition)
    Q_PROPERTY(bool mouseCaptured READ isMouseCaptured)
    Q_PROPERTY(bool allowMouseCapture READ getAllowMouseCapture WRITE setAllowMouseCapture)
    Q_PROPERTY(bool pointingAtSystemOverlay READ isPointingAtSystemOverlay)

public:
    ReticleInterface(CompositorHelper* outer) : QObject(outer), _compositor(outer) {}

    /*@jsdoc
     * Checks whether the mouse cursor is captured, displaying only in Interface and not on the rest of the desktop. The mouse 
     * cursor is captured when in HMD mode and the Interface window content (excluding menu items) has focus, if capturing is 
     * enabled (<code>allowMouseCapture</code> property value is <code>true</code>).
     * @function Reticle.isMouseCaptured
     * @returns {boolean} <code>true</code> if the mouse cursor is captured, displaying only in Interface and not on the 
     *     desktop.
     */
    Q_INVOKABLE bool isMouseCaptured() { return _compositor->shouldCaptureMouse(); }

    /*@jsdoc
     * Gets whether the mouse cursor will be captured when in HMD mode and the Interface window content (excluding menu items) 
     * has focus. When captured, the mouse cursor displays only in Interface, not on the rest of the desktop.
     * @function Reticle.getAllowMouseCapture
     * @returns {boolean} <code>true</code> if the mouse cursor will be captured when in HMD mode and the Interface window 
     *     content has focus, <code>false</code> if the mouse cursor will not be captured.
     */
    Q_INVOKABLE bool getAllowMouseCapture() { return _compositor->getAllowMouseCapture(); }

    /*@jsdoc
     * Sets whether the mouse cursor will be captured when in HMD mode and the Interface window content (excluding menu items)
     * has focus. When captured, the mouse cursor displays only in Interface, not on the rest of desktop.
     * @function Reticle.setAllowMouseCapture
     * @param {boolean} allowMouseCaptured - <code>true</code> if the mouse cursor will be captured when in HMD mode and the 
     *     Interface window content has focus, <code>false</code> if the mouse cursor will not be captured.
     */
    Q_INVOKABLE void setAllowMouseCapture(bool value) { return _compositor->setAllowMouseCapture(value); }

    /*@jsdoc
     * Gets whether the mouse cursor is pointing at UI in the Interface window in desktop mode or on the HUD surface in HMD 
     * mode.
     * @function Reticle.isPointingAtSystemOverlay
     * @returns {boolean} <code>true</code> if the mouse cursor is pointing at UI in the Interface window in desktop mode or on 
     *     the HUD surface in HMD mode, <code>false</code> if it isn't.
     */
    Q_INVOKABLE bool isPointingAtSystemOverlay() { return !_compositor->getReticleOverDesktop(); }

    /*@jsdoc
     * Gets whether the reticle circle is visible in desktop mode, or the arrow or reticle circle are visible in HMD mode.
     * @function Reticle.getVisible
     * @returns {boolean} <code>true</code> if the reticle circle is visible in desktop mode, and the arrow or 
     *     reticle circle are visible in HMD mode; <code>false</code> otherwise. (The mouse pointer is always visible in 
     *     desktop mode.)
     */
    Q_INVOKABLE bool getVisible() { return _compositor->getReticleVisible(); }

    /*@jsdoc
     * Sets whether the reticle circle is visible in desktop mode, or the arrow or reticle circle are visible in HMD mode.
     * @function Reticle.setVisible
     * @param {boolean} visible - <code>true</code> if the reticle circle is visible in desktop mode, and the arrow or reticle 
     *     circle are visible in HMD mode; <code>false</code> otherwise. (Does not affect the visibility of the mouse pointer 
     *     in desktop mode.)
     */
    Q_INVOKABLE void setVisible(bool visible) { _compositor->setReticleVisible(visible); }

    /*@jsdoc
     * Gets the depth (distance) that the reticle is displayed at relative to the HMD view, in HMD mode.
     * @function Reticle.getDepth
     * @returns {number} The depth (distance) that the reticle is displayed at relative to the HMD view, in HMD mode.
     */
    Q_INVOKABLE float getDepth() { return _compositor->getReticleDepth(); }

    /*@jsdoc
     * Sets the depth (distance) that the reticle is displayed at relative to the HMD view, in HMD mode.
     * @function Reticle.setDepth
     * @param {number} depth - The depth (distance) that the reticle is displayed at relative to the HMD view, in HMD mode.
     */
    Q_INVOKABLE void setDepth(float depth) { _compositor->setReticleDepth(depth); }

    /*@jsdoc
     * Gets the scale of the reticle circle in desktop mode, and the arrow and reticle circle in HMD mode. (Does not affect the 
     * size of the arrow in desktop mode.) The default scale is <code>1.0</code>.
     * @function Reticle.getScale
     * @returns {number} The scale of the reticle.
     */
    Q_INVOKABLE float getScale() const;

    /*@jsdoc
     * Sets the scale of the reticle circle in desktop mode, and the arrow and reticle circle in HMD mode. (Does not affect the
     * size of the arrow in desktop mode.) The default scale is <code>1.0</code>.
     * @function Reticle.setScale
     * @param {number} scale -  The scale of the reticle.
     */
    Q_INVOKABLE void setScale(float scale);

    /*@jsdoc
     * Gets the position of the cursor. This is the position relative to the Interface window in desktop mode, and the HUD 
     * surface in HMD mode.
     * <p><strong>Note:</strong> The position values may be negative.</p>
     * @function Reticle.getPosition
     * @returns {Vec2} The position of the cursor.
     */
    Q_INVOKABLE QVariant getPosition() const;

    /*@jsdoc
     * Sets the position of the cursor. This is the position relative to the Interface window in desktop mode, and the HUD 
     * surface in HMD mode.
     * <p><strong>Note:</strong> The position values may be negative.</p>
     * @function Reticle.setPosition
     * @param {Vec2} position - The position of the cursor.
     */
    Q_INVOKABLE void setPosition(QVariant position);

    /*@jsdoc
     * Gets the maximum reticle coordinates on the display device in desktop mode or the HUD surface in HMD mode. (The minimum 
     * reticle coordinates on the desktop display device or HUD surface are <code>0</code>, <code>0</code>.)
     * @function Reticle.getMaximumPosition
     * @returns {Vec2} The maximum reticle coordinates on the display device in desktop mode or the HUD surface in HMD mode.
     */
    Q_INVOKABLE glm::vec2 getMaximumPosition() { return _compositor->getReticleMaximumPosition(); }

private:
    CompositorHelper* _compositor;
};

#endif // hifi_CompositorHelper_h
