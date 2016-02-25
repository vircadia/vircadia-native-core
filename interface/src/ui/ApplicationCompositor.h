//
//  Created by Bradley Austin Davis Arnold on 2015/06/13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ApplicationCompositor_h
#define hifi_ApplicationCompositor_h

#include <atomic>
#include <cstdint>

#include <QCursor>
#include <QMouseEvent>
#include <QObject>
#include <QPropertyAnimation>

#include <EntityItemID.h>
#include <GeometryCache.h>
#include <GLMHelpers.h>
#include <gpu/Batch.h>
#include <gpu/Texture.h>

class Camera;
class PalmData;
class RenderArgs;
class ReticleInterface;


const float DEFAULT_RETICLE_DEPTH = 1.0f; // FIXME - probably should be based on UI radius

const float MAGNIFY_WIDTH = 220.0f;
const float MAGNIFY_HEIGHT = 100.0f;
const float MAGNIFY_MULT = 2.0f;

const int VIRTUAL_SCREEN_SIZE_X = 3960; // ~10% more pixel density than old version, 72dx240d FOV
const int VIRTUAL_SCREEN_SIZE_Y = 1188; // ~10% more pixel density than old version, 72dx240d FOV
const float DEFAULT_HMD_UI_HORZ_ANGULAR_SIZE = 240.0f;
const float DEFAULT_HMD_UI_VERT_ANGULAR_SIZE = DEFAULT_HMD_UI_HORZ_ANGULAR_SIZE * (float)VIRTUAL_SCREEN_SIZE_Y / (float)VIRTUAL_SCREEN_SIZE_X;

// Handles the drawing of the overlays to the screen
// TODO, move divide up the rendering, displaying and input handling
// facilities of this class
class ApplicationCompositor : public QObject {
    Q_OBJECT

    Q_PROPERTY(float alpha READ getAlpha WRITE setAlpha)
    Q_PROPERTY(bool reticleOverDesktop READ getReticleOverDesktop WRITE setReticleOverDesktop)
public:
    ApplicationCompositor();
    ~ApplicationCompositor();

    void displayOverlayTexture(RenderArgs* renderArgs);
    void displayOverlayTextureHmd(RenderArgs* renderArgs, int eye);

    bool calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const;

    float getHmdUIAngularSize() const { return _hmdUIAngularSize; }
    void setHmdUIAngularSize(float hmdUIAngularSize) { _hmdUIAngularSize = hmdUIAngularSize; }

    // Converter from one frame of reference to another.
    // Frame of reference:
    // Spherical: Polar coordinates that gives the position on the sphere we project on (yaw,pitch)
    // Overlay: Position on the overlay (x,y)
    glm::vec2 sphericalToOverlay(const glm::vec2 & sphericalPos) const;
    glm::vec2 overlayToSpherical(const glm::vec2 & overlayPos) const;
    void computeHmdPickRay(glm::vec2 cursorPos, glm::vec3& origin, glm::vec3& direction) const;
    uint32_t getOverlayTexture() const;

    glm::vec2 overlayFromSphereSurface(const glm::vec3& sphereSurfacePoint) const;
    glm::vec3 sphereSurfaceFromOverlay(const glm::vec2& overlay) const;

    void setCameraBaseTransform(const Transform& transform) { _cameraBaseTransform = transform; }
    const Transform& getCameraBaseTransform() const { return _cameraBaseTransform; }

    void setModelTransform(const Transform& transform) { _modelTransform = transform; }
    const Transform& getModelTransform() const { return _modelTransform; }

    void fadeIn();
    void fadeOut();
    void toggle();

    float getAlpha() const { return _alpha; }
    void setAlpha(float alpha) { _alpha = alpha; }

    bool getReticleVisible() const { return _reticleVisible; }
    void setReticleVisible(bool visible) { _reticleVisible = visible; }

    float getReticleDepth() const { return _reticleDepth; }
    void setReticleDepth(float depth) { _reticleDepth = depth; }
    void resetReticleDepth() { _reticleDepth = DEFAULT_RETICLE_DEPTH; }

    glm::vec2 getReticlePosition() const;
    void setReticlePosition(glm::vec2 position, bool sendFakeEvent = true);

    glm::vec2 getReticleMaximumPosition() const;

    ReticleInterface* getReticleInterface() { return _reticleInterface; }

    /// return value - true means the caller should not process the event further
    bool handleRealMouseMoveEvent(bool sendFakeEvent = true);
    void handleLeaveEvent();
    QPointF getMouseEventPosition(QMouseEvent* event);

    bool shouldCaptureMouse() const;

    /// if the reticle is pointing to a system overlay (a dialog box for example) then the function returns true otherwise false
    bool getReticleOverDesktop() const { return _isOverDesktop; }
    void setReticleOverDesktop(bool value) { _isOverDesktop = value; }

private:
    bool _isOverDesktop { true };

    void displayOverlayTextureStereo(RenderArgs* renderArgs, float aspectRatio, float fov);
    void bindCursorTexture(gpu::Batch& batch, uint8_t cursorId = 0);
    void buildHemiVertices(const float fov, const float aspectRatio, const int slices, const int stacks);
    void drawSphereSection(gpu::Batch& batch);
    void updateTooltips();

    // Support for hovering and tooltips
    static EntityItemID _noItemId;
    EntityItemID _hoverItemId { _noItemId };
    QString _hoverItemTitle;
    QString _hoverItemDescription;
    quint64 _hoverItemEnterUsecs { 0 };

    float _hmdUIAngularSize { DEFAULT_HMD_UI_VERT_ANGULAR_SIZE };
    float _textureFov { glm::radians(DEFAULT_HMD_UI_VERT_ANGULAR_SIZE) };
    float _textureAspectRatio { 1.0f };
    int _hemiVerticesID { GeometryCache::UNKNOWN_ID };

    float _alpha { 1.0f };
    float _prevAlpha { 1.0f };
    float _fadeInAlpha { true };
    float _oculusUIRadius { 1.0f };

    QMap<uint16_t, gpu::TexturePointer> _cursors;

    int _reticleQuad;

    int _previousBorderWidth { -1 };
    int _previousBorderHeight { -1 };

    Transform _modelTransform;
    Transform _cameraBaseTransform;

    std::unique_ptr<QPropertyAnimation> _alphaPropertyAnimation;

    std::atomic<bool> _reticleVisible { true };
    std::atomic<float> _reticleDepth { 1.0f };

    // NOTE: when the compositor is running in HMD mode, it will control the reticle position as a custom
    // application specific position, when it's in desktop mode, the reticle position will simply move
    // the system mouse.
    glm::vec2 _reticlePositionInHMD { 0.0f, 0.0f };
    mutable QMutex _reticleLock { QMutex::Recursive };

    QPointF _lastKnownRealMouse;
    bool _ignoreMouseMove { false };

    bool _reticleOverQml { false };

    ReticleInterface* _reticleInterface;
};

// Scripting interface available to control the Reticle
class ReticleInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(glm::vec2 position READ getPosition WRITE setPosition)
    Q_PROPERTY(bool visible READ getVisible WRITE setVisible)
    Q_PROPERTY(float depth READ getDepth WRITE setDepth)
    Q_PROPERTY(glm::vec2 maximumPosition READ getMaximumPosition)
    Q_PROPERTY(bool mouseCaptured READ isMouseCaptured)
    Q_PROPERTY(bool pointingAtSystemOverlay READ isPointingAtSystemOverlay)

public:
    ReticleInterface(ApplicationCompositor* outer) : QObject(outer), _compositor(outer) {}

    Q_INVOKABLE bool isMouseCaptured() { return _compositor->shouldCaptureMouse(); }
    Q_INVOKABLE bool isPointingAtSystemOverlay() { return !_compositor->getReticleOverDesktop(); }

    Q_INVOKABLE bool getVisible() { return _compositor->getReticleVisible(); }
    Q_INVOKABLE void setVisible(bool visible) { _compositor->setReticleVisible(visible); }

    Q_INVOKABLE float getDepth() { return _compositor->getReticleDepth(); }
    Q_INVOKABLE void setDepth(float depth) { _compositor->setReticleDepth(depth); }

    Q_INVOKABLE glm::vec2 getPosition() { return _compositor->getReticlePosition(); }
    Q_INVOKABLE void setPosition(glm::vec2 position) { _compositor->setReticlePosition(position); }

    Q_INVOKABLE glm::vec2 getMaximumPosition() { return _compositor->getReticleMaximumPosition(); }

private:
    ApplicationCompositor* _compositor;
};



#endif // hifi_ApplicationCompositor_h
