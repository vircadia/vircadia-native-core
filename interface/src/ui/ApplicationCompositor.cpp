//
//  ApplicationCompositor.cpp
//  interface/src/ui/overlays
//
//  Created by Benjamin Arnold on 5/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ApplicationCompositor.h"

#include <memory>

#include <QPropertyAnimation>

#include <glm/gtc/type_ptr.hpp>

#include <display-plugins/DisplayPlugin.h>
#include <avatar/AvatarManager.h>
#include <gpu/GLBackend.h>
#include <NumericalConstants.h>

#include "CursorManager.h"
#include "Tooltip.h"

#include "Application.h"
#include <controllers/InputDevice.h>


// Used to animate the magnification windows

static const quint64 MSECS_TO_USECS = 1000ULL;
static const quint64 TOOLTIP_DELAY = 500 * MSECS_TO_USECS;

static const float reticleSize = TWO_PI / 100.0f;

static const float CURSOR_PIXEL_SIZE = 32.0f;

static gpu::BufferPointer _hemiVertices;
static gpu::BufferPointer _hemiIndices;
static int _hemiIndexCount{ 0 };
EntityItemID ApplicationCompositor::_noItemId;
static QString _tooltipId;

// Return a point's cartesian coordinates on a sphere from pitch and yaw
glm::vec3 getPoint(float yaw, float pitch) {
    return glm::vec3(glm::cos(-pitch) * (-glm::sin(yaw)),
                     glm::sin(-pitch),
                     glm::cos(-pitch) * (-glm::cos(yaw)));
}

//Checks if the given ray intersects the sphere at the origin. result will store a multiplier that should
//be multiplied by dir and added to origin to get the location of the collision
bool raySphereIntersect(const glm::vec3 &dir, const glm::vec3 &origin, float r, float* result)
{
    //Source: http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection

    //Compute A, B and C coefficients
    float a = glm::dot(dir, dir);
    float b = 2 * glm::dot(dir, origin);
    float c = glm::dot(origin, origin) - (r * r);

    //Find discriminant
    float disc = b * b - 4 * a * c;

    // if discriminant is negative there are no real roots, so return
    // false as ray misses sphere
    if (disc < 0) {
        return false;
    }

    // compute q as described above
    float distSqrt = sqrtf(disc);
    float q;
    if (b < 0) {
        q = (-b - distSqrt) / 2.0f;
    } else {
        q = (-b + distSqrt) / 2.0f;
    }

    // compute t0 and t1
    float t0 = q / a;
    float t1 = c / q;

    // make sure t0 is smaller than t1
    if (t0 > t1) {
        // if t0 is bigger than t1 swap them around
        float temp = t0;
        t0 = t1;
        t1 = temp;
    }

    // if t1 is less than zero, the object is in the ray's negative direction
    // and consequently the ray misses the sphere
    if (t1 < 0) {
        return false;
    }

    // if t0 is less than zero, the intersection point is at t1
    if (t0 < 0) {
        *result = t1;
        return true;
    } else { // else the intersection point is at t0
        *result = t0;
        return true;
    }
}

ApplicationCompositor::ApplicationCompositor() :
    _alphaPropertyAnimation(new QPropertyAnimation(this, "alpha")),
    _reticleInterface(new ReticleInterface(this))

{
    auto geometryCache = DependencyManager::get<GeometryCache>();

    _reticleQuad = geometryCache->allocateID();

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    connect(entityScriptingInterface.data(), &EntityScriptingInterface::hoverEnterEntity, [=](const EntityItemID& entityItemID, const MouseEvent& event) {
        if (_hoverItemId != entityItemID) {
            _hoverItemId = entityItemID;
            _hoverItemEnterUsecs = usecTimestampNow();
            auto properties = entityScriptingInterface->getEntityProperties(_hoverItemId);

            // check the format of this href string before we parse it
            QString hrefString = properties.getHref();

            auto cursor = Cursor::Manager::instance().getCursor();
            if (!hrefString.isEmpty()) {
                if (!hrefString.startsWith("hifi:")) {
                    hrefString.prepend("hifi://");
                }

                // parse out a QUrl from the hrefString
                QUrl href = QUrl(hrefString);

                _hoverItemTitle = href.host();
                _hoverItemDescription = properties.getDescription();
                cursor->setIcon(Cursor::Icon::LINK);
            } else {
                _hoverItemTitle.clear();
                _hoverItemDescription.clear();
                cursor->setIcon(Cursor::Icon::DEFAULT);
            }
        }
    });

    connect(entityScriptingInterface.data(), &EntityScriptingInterface::hoverLeaveEntity, [=](const EntityItemID& entityItemID, const MouseEvent& event) {
        if (_hoverItemId == entityItemID) {
            _hoverItemId = _noItemId;

            _hoverItemTitle.clear();
            _hoverItemDescription.clear();

            auto cursor = Cursor::Manager::instance().getCursor();
            cursor->setIcon(Cursor::Icon::DEFAULT);
            if (!_tooltipId.isEmpty()) {
                qDebug() << "Closing tooltip " << _tooltipId;
                Tooltip::closeTip(_tooltipId);
                _tooltipId.clear();
            }
        }
    });

    _alphaPropertyAnimation.reset(new QPropertyAnimation(this, "alpha"));
}

ApplicationCompositor::~ApplicationCompositor() {
}


void ApplicationCompositor::bindCursorTexture(gpu::Batch& batch, uint8_t cursorIndex) {
    auto& cursorManager = Cursor::Manager::instance();
    auto cursor = cursorManager.getCursor(cursorIndex);
    auto iconId = cursor->getIcon();
    if (!_cursors.count(iconId)) {
        auto iconPath = cursorManager.getIconImage(cursor->getIcon());
        _cursors[iconId] = DependencyManager::get<TextureCache>()->
            getImageTexture(iconPath);
    }
    batch.setResourceTexture(0, _cursors[iconId]);
}

// Draws the FBO texture for the screen
void ApplicationCompositor::displayOverlayTexture(RenderArgs* renderArgs) {
    PROFILE_RANGE(__FUNCTION__);

    if (_alpha <= 0.0f) {
        return;
    }

    gpu::FramebufferPointer overlayFramebuffer = qApp->getApplicationOverlay().getOverlayFramebuffer();
    if (!overlayFramebuffer) {
        return;
    }

    updateTooltips();

    //Handle fading and deactivation/activation of UI
    gpu::doInBatch(renderArgs->_context, [&](gpu::Batch& batch) {

        auto geometryCache = DependencyManager::get<GeometryCache>();

        geometryCache->useSimpleDrawPipeline(batch);
        batch.setViewportTransform(renderArgs->_viewport);
        batch.setModelTransform(Transform());
        batch.setViewTransform(Transform());
        batch.setProjectionTransform(mat4());
        batch.setResourceTexture(0, overlayFramebuffer->getRenderBuffer(0));
        geometryCache->renderUnitQuad(batch, vec4(vec3(1), _alpha));

        //draw the mouse pointer
        // Get the mouse coordinates and convert to NDC [-1, 1]
        vec2 canvasSize = qApp->getCanvasSize(); // desktop, use actual canvas...
        vec2 mousePosition = toNormalizedDeviceScale(vec2(qApp->getMouse()), canvasSize);
        // Invert the Y axis
        mousePosition.y *= -1.0f;

        Transform model;
        model.setTranslation(vec3(mousePosition, 0));
        vec2 mouseSize = CURSOR_PIXEL_SIZE / canvasSize;
        model.setScale(vec3(mouseSize, 1.0f));
        batch.setModelTransform(model);
        bindCursorTexture(batch);
        geometryCache->renderUnitQuad(batch, vec4(1));
    });
}

// Draws the FBO texture for Oculus rift.
void ApplicationCompositor::displayOverlayTextureHmd(RenderArgs* renderArgs, int eye) {
    PROFILE_RANGE(__FUNCTION__);

    if (_alpha <= 0.0f) {
        return;
    }

    gpu::FramebufferPointer overlayFramebuffer = qApp->getApplicationOverlay().getOverlayFramebuffer();
    if (!overlayFramebuffer) {
        return;
    }

    updateTooltips();

    glm::uvec2 screenSize = qApp->getUiSize(); // HMD use virtual screen size
    vec2 canvasSize = screenSize;
    _textureAspectRatio = aspect(canvasSize);

    auto geometryCache = DependencyManager::get<GeometryCache>();

    gpu::doInBatch(renderArgs->_context, [&](gpu::Batch& batch) {
        geometryCache->useSimpleDrawPipeline(batch);

        batch.setResourceTexture(0, overlayFramebuffer->getRenderBuffer(0));

        mat4 camMat;
        _cameraBaseTransform.getMatrix(camMat);
        auto displayPlugin = qApp->getActiveDisplayPlugin();
        auto headPose = qApp->getHMDSensorPose();
        auto eyeToHead = displayPlugin->getEyeToHeadTransform((Eye)eye);
        camMat = (headPose * eyeToHead) * camMat; // FIXME - why are not all transforms are doing this aditioanl eyeToHead
        batch.setViewportTransform(renderArgs->_viewport);
        batch.setViewTransform(camMat);
        batch.setProjectionTransform(qApp->getEyeProjection(eye));

    #ifdef DEBUG_OVERLAY
        {
            batch.setModelTransform(glm::translate(mat4(), vec3(0, 0, -2)));
            geometryCache->renderUnitQuad(batch, glm::vec4(1));
        }
    #else
        {
            batch.setModelTransform(_modelTransform);
            drawSphereSection(batch);
        }
    #endif


        vec3 reticleScale = vec3(Cursor::Manager::instance().getScale() * reticleSize);

        bindCursorTexture(batch);

        //Mouse Pointer
        if (getReticleVisible()) {
            if (getReticleDepth() != 1.0f) {
                // calculate the "apparent location" based on the depth and the current ray
                glm::vec3 origin, direction;
                auto reticlePosition = getReticlePosition();
                computeHmdPickRay(reticlePosition, origin, direction);
                auto apparentPosition = origin + (direction * getReticleDepth());

                // same code as used to render for apparent location
                auto myCamera = qApp->getCamera();
                mat4 cameraMat = myCamera->getTransform();
                auto UITransform = cameraMat * glm::inverse(headPose);
                auto relativePosition4 = glm::inverse(UITransform) * vec4(apparentPosition, 1);
                auto relativePosition = vec3(relativePosition4) / relativePosition4.w;
                auto relativeDistance = glm::length(relativePosition);

                // look at borrowed from overlays
                float elevation = -asinf(relativePosition.y / glm::length(relativePosition));
                float azimuth = atan2f(relativePosition.x, relativePosition.z);
                glm::quat faceCamera = glm::quat(glm::vec3(elevation, azimuth, 0)) * quat(vec3(0, 0, -1)); // this extra *quat(vec3(0,0,-1)) was required to get the quad to flip this seems like we could optimize

                Transform transform;
                transform.setTranslation(relativePosition);
                transform.setScale(reticleScale);
                transform.postScale(relativeDistance); // scale not quite working, distant things too large
                transform.setRotation(faceCamera);
                batch.setModelTransform(transform);
            } else {
                glm::mat4 overlayXfm;
                _modelTransform.getMatrix(overlayXfm);

                auto reticlePosition = getReticlePosition();
                glm::vec2 projection = overlayToSpherical(reticlePosition);
                mat4 pointerXfm = glm::mat4_cast(quat(vec3(-projection.y, projection.x, 0.0f))) * glm::translate(mat4(), vec3(0, 0, -1));
                mat4 reticleXfm = overlayXfm * pointerXfm;
                reticleXfm = glm::scale(reticleXfm, reticleScale);
                batch.setModelTransform(reticleXfm);
            }
            geometryCache->renderUnitQuad(batch, glm::vec4(1), _reticleQuad);
        }
    });
}

QPointF ApplicationCompositor::getMouseEventPosition(QMouseEvent* event) {
    if (qApp->isHMDMode()) {
        QMutexLocker locker(&_reticleLock);
        return QPointF(_reticlePositionInHMD.x, _reticlePositionInHMD.y);
    }
    return event->localPos();
}

bool ApplicationCompositor::shouldCaptureMouse() const {
    // if we're in HMD mode, and some window of ours is active, but we're not currently showing a popup menu
    return qApp->isHMDMode() && QApplication::activeWindow() && !Menu::isSomeSubmenuShown();
}

void ApplicationCompositor::handleLeaveEvent() {

    if (shouldCaptureMouse()) {
        QWidget* mainWidget = (QWidget*)qApp->getWindow();
        QRect mainWidgetFrame = qApp->getRenderingGeometry();
        QRect uncoveredRect = mainWidgetFrame;
        foreach(QWidget* widget, QApplication::topLevelWidgets()) {
            if (widget->isWindow() && widget->isVisible() && widget != mainWidget) {
                QRect widgetFrame = widget->frameGeometry();
                if (widgetFrame.intersects(uncoveredRect)) {
                    QRect intersection = uncoveredRect & widgetFrame;
                    if (intersection.top() > uncoveredRect.top()) {
                        uncoveredRect.setBottom(intersection.top() - 1);
                    } else if (intersection.bottom() < uncoveredRect.bottom()) {
                        uncoveredRect.setTop(intersection.bottom() + 1);
                    }

                    if (intersection.left() > uncoveredRect.left()) {
                        uncoveredRect.setRight(intersection.left() - 1);
                    } else if (intersection.right() < uncoveredRect.right()) {
                        uncoveredRect.setLeft(intersection.right() + 1);
                    }
                }
            }
        }

        _ignoreMouseMove = true;
        auto sendToPos = uncoveredRect.center();
        QCursor::setPos(sendToPos);
        _lastKnownRealMouse = sendToPos;
    }
}

bool ApplicationCompositor::handleRealMouseMoveEvent(bool sendFakeEvent) {

    // If the mouse move came from a capture mouse related move, we completely ignore it.
    if (_ignoreMouseMove) {
        _ignoreMouseMove = false;
        return true; // swallow the event
    }

    // If we're in HMD mode
    if (shouldCaptureMouse()) {
        QMutexLocker locker(&_reticleLock);
        auto newPosition = QCursor::pos();
        auto changeInRealMouse = newPosition - _lastKnownRealMouse;
        auto newReticlePosition = _reticlePositionInHMD + toGlm(changeInRealMouse);
        setReticlePosition(newReticlePosition, sendFakeEvent);
        _ignoreMouseMove = true;
        QCursor::setPos(QPoint(_lastKnownRealMouse.x(), _lastKnownRealMouse.y())); // move cursor back to where it was
        return true;  // swallow the event
    } else {
        _lastKnownRealMouse = QCursor::pos();
    }
    return false; // let the caller know to process the event
}

glm::vec2 ApplicationCompositor::getReticlePosition() const {
    if (qApp->isHMDMode()) {
        QMutexLocker locker(&_reticleLock);
        return _reticlePositionInHMD;
    }
    return toGlm(QCursor::pos());
}

void ApplicationCompositor::setReticlePosition(glm::vec2 position, bool sendFakeEvent) {
    if (qApp->isHMDMode()) {
        QMutexLocker locker(&_reticleLock);
        const float MOUSE_EXTENTS_VERT_ANGULAR_SIZE = 170.0f; // 5deg from poles
        const float MOUSE_EXTENTS_VERT_PIXELS = VIRTUAL_SCREEN_SIZE_Y * (MOUSE_EXTENTS_VERT_ANGULAR_SIZE / DEFAULT_HMD_UI_VERT_ANGULAR_SIZE);
        const float MOUSE_EXTENTS_HORZ_ANGULAR_SIZE = 360.0f; // full sphere
        const float MOUSE_EXTENTS_HORZ_PIXELS = VIRTUAL_SCREEN_SIZE_X * (MOUSE_EXTENTS_HORZ_ANGULAR_SIZE / DEFAULT_HMD_UI_HORZ_ANGULAR_SIZE);

        glm::vec2 maxOverlayPosition = qApp->getUiSize();
        float extaPixelsX = (MOUSE_EXTENTS_HORZ_PIXELS - maxOverlayPosition.x) / 2.0f;
        float extaPixelsY = (MOUSE_EXTENTS_VERT_PIXELS - maxOverlayPosition.y) / 2.0f;
        glm::vec2 mouseExtra { extaPixelsX, extaPixelsY };
        glm::vec2 minMouse = vec2(0) - mouseExtra;
        glm::vec2 maxMouse = maxOverlayPosition + mouseExtra;

        _reticlePositionInHMD = glm::clamp(position, minMouse, maxMouse);

        if (sendFakeEvent) {
            // in HMD mode we need to fake our mouse moves...
            QPoint globalPos(_reticlePositionInHMD.x, _reticlePositionInHMD.y);
            auto button = Qt::NoButton;
            auto buttons = QApplication::mouseButtons();
            auto modifiers = QApplication::keyboardModifiers();
            QMouseEvent event(QEvent::MouseMove, globalPos, button, buttons, modifiers);
            qApp->fakeMouseEvent(&event);
        }
    } else {
        // NOTE: This is some debugging code we will leave in while debugging various reticle movement strategies,
        // remove it after we're done
        const float REASONABLE_CHANGE = 50.0f;
        glm::vec2 oldPos = toGlm(QCursor::pos());
        auto distance = glm::distance(oldPos, position);
        if (distance > REASONABLE_CHANGE) {
            qDebug() << "Contrller::ScriptingInterface ---- UNREASONABLE CHANGE! distance:" << distance << " oldPos:" << oldPos << " newPos:" << position;
        }

        QCursor::setPos(position.x, position.y);
    }
}

#include <QDesktopWidget>

glm::vec2 ApplicationCompositor::getReticleMaximumPosition() const {
    glm::vec2 result;
    if (qApp->isHMDMode()) {
        result = glm::vec2(VIRTUAL_SCREEN_SIZE_X, VIRTUAL_SCREEN_SIZE_Y);
    } else {
        QRect rec = QApplication::desktop()->screenGeometry();
        result = glm::vec2(rec.right(), rec.bottom());
    }
    return result;
}

void ApplicationCompositor::computeHmdPickRay(glm::vec2 cursorPos, glm::vec3& origin, glm::vec3& direction) const {
    auto surfacePointAt = sphereSurfaceFromOverlay(cursorPos); // in world space
    glm::vec3 worldSpaceCameraPosition = qApp->getCamera()->getPosition();
    origin = worldSpaceCameraPosition;
    direction = glm::normalize(surfacePointAt - worldSpaceCameraPosition);
}

//Finds the collision point of a world space ray
bool ApplicationCompositor::calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const {
    auto headPose = qApp->getHMDSensorPose();
    auto myCamera = qApp->getCamera();
    mat4 cameraMat = myCamera->getTransform();
    auto UITransform = cameraMat * glm::inverse(headPose);
    auto relativePosition4 = glm::inverse(UITransform) * vec4(position, 1);
    auto relativePosition = vec3(relativePosition4) / relativePosition4.w;
    auto relativeDirection = glm::inverse(glm::quat_cast(UITransform)) * direction;

    float uiRadius = _oculusUIRadius; // * myAvatar->getUniformScale(); // FIXME - how do we want to handle avatar scale

    float instersectionDistance;
    if (raySphereIntersect(relativeDirection, relativePosition, uiRadius, &instersectionDistance)){
        result = position + glm::normalize(direction) * instersectionDistance;
        return true;
    }

    return false;
}

void ApplicationCompositor::buildHemiVertices(
    const float fov, const float aspectRatio, const int slices, const int stacks) {
    static float textureFOV = 0.0f, textureAspectRatio = 1.0f;
    if (textureFOV == fov && textureAspectRatio == aspectRatio) {
        return;
    }

    textureFOV = fov;
    textureAspectRatio = aspectRatio;

    auto geometryCache = DependencyManager::get<GeometryCache>();

    _hemiVertices = std::make_shared<gpu::Buffer>();
    _hemiIndices = std::make_shared<gpu::Buffer>();


    if (fov >= PI) {
        qDebug() << "TexturedHemisphere::buildVBO(): FOV greater or equal than Pi will create issues";
    }

    //UV mapping source: http://www.mvps.org/directx/articles/spheremap.htm

    vec3 pos;
    vec2 uv;
    // Compute vertices positions and texture UV coordinate
    // Create and write to buffer
    for (int i = 0; i < stacks; i++) {
        uv.y = (float)i / (float)(stacks - 1); // First stack is 0.0f, last stack is 1.0f
        // abs(theta) <= fov / 2.0f
        float pitch = -fov * (uv.y - 0.5f);
        for (int j = 0; j < slices; j++) {
            uv.x = (float)j / (float)(slices - 1); // First slice is 0.0f, last slice is 1.0f
            // abs(phi) <= fov * aspectRatio / 2.0f
            float yaw = -fov * aspectRatio * (uv.x - 0.5f);
            pos = getPoint(yaw, pitch);
            static const vec4 color(1);
            _hemiVertices->append(sizeof(pos), (gpu::Byte*)&pos);
            _hemiVertices->append(sizeof(vec2), (gpu::Byte*)&uv);
            _hemiVertices->append(sizeof(vec4), (gpu::Byte*)&color);
        }
    }

    // Compute number of indices needed
    static const int VERTEX_PER_TRANGLE = 3;
    static const int TRIANGLE_PER_RECTANGLE = 2;
    int numberOfRectangles = (slices - 1) * (stacks - 1);
    _hemiIndexCount = numberOfRectangles * TRIANGLE_PER_RECTANGLE * VERTEX_PER_TRANGLE;

    // Compute indices order
    std::vector<GLushort> indices;
    for (int i = 0; i < stacks - 1; i++) {
        for (int j = 0; j < slices - 1; j++) {
            GLushort bottomLeftIndex = i * slices + j;
            GLushort bottomRightIndex = bottomLeftIndex + 1;
            GLushort topLeftIndex = bottomLeftIndex + slices;
            GLushort topRightIndex = topLeftIndex + 1;
            // FIXME make a z-order curve for better vertex cache locality
            indices.push_back(topLeftIndex);
            indices.push_back(bottomLeftIndex);
            indices.push_back(topRightIndex);

            indices.push_back(topRightIndex);
            indices.push_back(bottomLeftIndex);
            indices.push_back(bottomRightIndex);
        }
    }
    _hemiIndices->append(sizeof(GLushort) * indices.size(), (gpu::Byte*)&indices[0]);
}


void ApplicationCompositor::drawSphereSection(gpu::Batch& batch) {
    buildHemiVertices(_textureFov, _textureAspectRatio, 80, 80);
    static const int VERTEX_DATA_SLOT = 0;
    static const int TEXTURE_DATA_SLOT = 1;
    static const int COLOR_DATA_SLOT = 2;
    auto streamFormat = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
    streamFormat->setAttribute(gpu::Stream::POSITION, VERTEX_DATA_SLOT, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
    streamFormat->setAttribute(gpu::Stream::TEXCOORD, TEXTURE_DATA_SLOT, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
    streamFormat->setAttribute(gpu::Stream::COLOR, COLOR_DATA_SLOT, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::RGBA));
    batch.setInputFormat(streamFormat);

    static const int VERTEX_STRIDE = sizeof(vec3) + sizeof(vec2) + sizeof(vec4);

    if (_prevAlpha != _alpha) {
        // adjust alpha by munging vertex color alpha.
        // FIXME we should probably just use a uniform for this.
        float* floatPtr = reinterpret_cast<float*>(_hemiVertices->editData());
        const auto ALPHA_FLOAT_OFFSET = (sizeof(vec3) + sizeof(vec2) + sizeof(vec3)) / sizeof(float);
        const auto VERTEX_FLOAT_STRIDE = (sizeof(vec3) + sizeof(vec2) + sizeof(vec4)) / sizeof(float);
        const auto NUM_VERTS = _hemiVertices->getSize() / VERTEX_STRIDE;
        for (size_t i = 0; i < NUM_VERTS; i++) {
            floatPtr[i * VERTEX_FLOAT_STRIDE + ALPHA_FLOAT_OFFSET] = _alpha;
        }
    }

    gpu::BufferView posView(_hemiVertices, 0, _hemiVertices->getSize(), VERTEX_STRIDE, streamFormat->getAttributes().at(gpu::Stream::POSITION)._element);
    gpu::BufferView uvView(_hemiVertices, sizeof(vec3), _hemiVertices->getSize(), VERTEX_STRIDE, streamFormat->getAttributes().at(gpu::Stream::TEXCOORD)._element);
    gpu::BufferView colView(_hemiVertices, sizeof(vec3) + sizeof(vec2), _hemiVertices->getSize(), VERTEX_STRIDE, streamFormat->getAttributes().at(gpu::Stream::COLOR)._element);
    batch.setInputBuffer(VERTEX_DATA_SLOT, posView);
    batch.setInputBuffer(TEXTURE_DATA_SLOT, uvView);
    batch.setInputBuffer(COLOR_DATA_SLOT, colView);
    batch.setIndexBuffer(gpu::UINT16, _hemiIndices, 0);
    batch.drawIndexed(gpu::TRIANGLES, _hemiIndexCount);
}

glm::vec2 ApplicationCompositor::sphericalToOverlay(const glm::vec2&  sphericalPos) const {
    glm::vec2 result = sphericalPos;
    result.x *= -1.0f;
    result /= _textureFov;
    result.x /= _textureAspectRatio;
    result += 0.5f;
    result *= qApp->getUiSize();
    return result;
}

glm::vec2 ApplicationCompositor::overlayToSpherical(const glm::vec2&  overlayPos) const {
    glm::vec2 result = overlayPos;
    result /= qApp->getUiSize();
    result -= 0.5f;
    result *= _textureFov;
    result.x *= _textureAspectRatio;
    result.x *= -1.0f;
    return result;
}

glm::vec2 ApplicationCompositor::overlayFromSphereSurface(const glm::vec3& sphereSurfacePoint) const {
    auto headPose = qApp->getHMDSensorPose();
    auto myCamera = qApp->getCamera();
    mat4 cameraMat = myCamera->getTransform();
    auto UITransform = cameraMat * glm::inverse(headPose);
    auto relativePosition4 = glm::inverse(UITransform) * vec4(sphereSurfacePoint, 1);
    auto relativePosition = vec3(relativePosition4) / relativePosition4.w;
    auto center = vec3(0); // center of HUD in HUD space
    auto direction = relativePosition - center; // direction to relative position in HUD space
    glm::vec2 polar = glm::vec2(glm::atan(direction.x, -direction.z), glm::asin(direction.y)) * -1.0f;
    auto overlayPos = sphericalToOverlay(polar);
    return overlayPos;
}

glm::vec3 ApplicationCompositor::sphereSurfaceFromOverlay(const glm::vec2& overlay) const {
    auto spherical = overlayToSpherical(overlay);
    auto sphereSurfacePoint = getPoint(spherical.x, spherical.y);
    auto headPose = qApp->getHMDSensorPose();
    auto myCamera = qApp->getCamera();
    mat4 cameraMat = myCamera->getTransform();
    auto UITransform = cameraMat * glm::inverse(headPose);
    auto position4 = UITransform * vec4(sphereSurfacePoint, 1);
    auto position = vec3(position4) / position4.w;
    return position;
}


void ApplicationCompositor::updateTooltips() {
    if (_hoverItemId != _noItemId) {
        quint64 hoverDuration = usecTimestampNow() - _hoverItemEnterUsecs;
        if (_hoverItemEnterUsecs != UINT64_MAX && !_hoverItemTitle.isEmpty() && hoverDuration > TOOLTIP_DELAY) {
            // TODO Enable and position the tooltip
            _hoverItemEnterUsecs = UINT64_MAX;
            _tooltipId = Tooltip::showTip(_hoverItemTitle, _hoverItemDescription);
        }
    }
}

static const float FADE_DURATION = 500.0f;
void ApplicationCompositor::fadeIn() {
    _fadeInAlpha = true;

    _alphaPropertyAnimation->setDuration(FADE_DURATION);
    _alphaPropertyAnimation->setStartValue(_alpha);
    _alphaPropertyAnimation->setEndValue(1.0f);
    _alphaPropertyAnimation->start();
}
void ApplicationCompositor::fadeOut() {
    _fadeInAlpha = false;

    _alphaPropertyAnimation->setDuration(FADE_DURATION);
    _alphaPropertyAnimation->setStartValue(_alpha);
    _alphaPropertyAnimation->setEndValue(0.0f);
    _alphaPropertyAnimation->start();
}

void ApplicationCompositor::toggle() {
    if (_fadeInAlpha) {
        fadeOut();
    } else {
        fadeIn();
    }
}
