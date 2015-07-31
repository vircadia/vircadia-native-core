//
//  Created by Bradley Austin Davis Arnold on 2015/06/13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ApplicationCompositor_h
#define hifi_ApplicationCompositor_h

#include <QObject>
#include <cstdint>

#include <EntityItemID.h>
#include <GeometryCache.h>
#include <GLMHelpers.h>
#include <gpu/Batch.h>
#include <gpu/Texture.h>

class Camera;
class PalmData;
class RenderArgs;

const float MAGNIFY_WIDTH = 220.0f;
const float MAGNIFY_HEIGHT = 100.0f;
const float MAGNIFY_MULT = 2.0f;

const float DEFAULT_HMD_UI_ANGULAR_SIZE = 72.0f;

// Handles the drawing of the overlays to the screen
// TODO, move divide up the rendering, displaying and input handling
// facilities of this class
class ApplicationCompositor : public QObject {
    Q_OBJECT
public:
    ApplicationCompositor();
    ~ApplicationCompositor();

    void displayOverlayTexture(RenderArgs* renderArgs);
    void displayOverlayTextureHmd(RenderArgs* renderArgs, int eye);

    QPoint getPalmClickLocation(const PalmData *palm) const;
    bool calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const;

    bool hasMagnifier() const { return _magnifier; }
    void toggleMagnifier() { _magnifier = !_magnifier; }

    float getHmdUIAngularSize() const { return _hmdUIAngularSize; }
    void setHmdUIAngularSize(float hmdUIAngularSize) { _hmdUIAngularSize = hmdUIAngularSize; }

    // Converter from one frame of reference to another.
    // Frame of reference:
    // Direction: Ray that represents the spherical values
    // Screen: Position on the screen (x,y)
    // Spherical: Pitch and yaw that gives the position on the sphere we project on (yaw,pitch)
    // Overlay: Position on the overlay (x,y)
    // (x,y) in Overlay are similar than (x,y) in Screen except they can be outside of the bound of te screen.
    // This allows for picking outside of the screen projection in 3D.
    glm::vec2 sphericalToOverlay(const glm::vec2 & sphericalPos) const;
    glm::vec2 overlayToSpherical(const glm::vec2 & overlayPos) const;
    glm::vec2 screenToOverlay(const glm::vec2 & screenPos) const;
    glm::vec2 overlayToScreen(const glm::vec2 & overlayPos) const;
    void computeHmdPickRay(glm::vec2 cursorPos, glm::vec3& origin, glm::vec3& direction) const;
    uint32_t getOverlayTexture() const;

    static glm::vec2 directionToSpherical(const glm::vec3 & direction);
    static glm::vec3 sphericalToDirection(const glm::vec2 & sphericalPos);
    static glm::vec2 screenToSpherical(const glm::vec2 & screenPos);
    static glm::vec2 sphericalToScreen(const glm::vec2 & sphericalPos);

private:
    void displayOverlayTextureStereo(RenderArgs* renderArgs, float aspectRatio, float fov);
    void bindCursorTexture(gpu::Batch& batch, uint8_t cursorId = 0);
    void buildHemiVertices(const float fov, const float aspectRatio, const int slices, const int stacks);
    void drawSphereSection(gpu::Batch& batch);
    void updateTooltips();

    void renderPointers(gpu::Batch& batch);
    void renderControllerPointers(gpu::Batch& batch);
    void renderPointersOculus(gpu::Batch& batch);

    // Support for hovering and tooltips
    static EntityItemID _noItemId;
    EntityItemID _hoverItemId { _noItemId };
    QString _hoverItemTitle;
    QString _hoverItemDescription;
    quint64 _hoverItemEnterUsecs { 0 };

    float _hmdUIAngularSize = DEFAULT_HMD_UI_ANGULAR_SIZE;
    float _textureFov{ glm::radians(DEFAULT_HMD_UI_ANGULAR_SIZE) };
    float _textureAspectRatio{ 1.0f };
    int _hemiVerticesID{ GeometryCache::UNKNOWN_ID };

    enum Reticles { MOUSE, LEFT_CONTROLLER, RIGHT_CONTROLLER, NUMBER_OF_RETICLES };
    bool _reticleActive[NUMBER_OF_RETICLES];
    QPoint _reticlePosition[NUMBER_OF_RETICLES];
    bool _magActive[NUMBER_OF_RETICLES];
    float _magSizeMult[NUMBER_OF_RETICLES];
    bool _magnifier{ true };

    float _alpha{ 1.0f };
    float _oculusUIRadius{ 1.0f };

    QMap<uint16_t, gpu::TexturePointer> _cursors;

    int _reticleQuad;
    int _magnifierQuad;
    int _magnifierBorder;

    int _previousBorderWidth{ -1 };
    int _previousBorderHeight{ -1 };

    glm::vec3 _previousMagnifierBottomLeft;
    glm::vec3 _previousMagnifierBottomRight;
    glm::vec3 _previousMagnifierTopLeft;
    glm::vec3 _previousMagnifierTopRight;
};

#endif // hifi_ApplicationCompositor_h
