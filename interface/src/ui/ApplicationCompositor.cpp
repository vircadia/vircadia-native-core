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

#include <glm/gtc/type_ptr.hpp>

#include <avatar/AvatarManager.h>
#include <gpu/GLBackend.h>
#include <NumericalConstants.h>

#include "CursorManager.h"
#include "Tooltip.h"

#include "Application.h"


// Used to animate the magnification windows

static const quint64 MSECS_TO_USECS = 1000ULL;
static const quint64 TOOLTIP_DELAY = 500 * MSECS_TO_USECS;

static const float RETICLE_COLOR[] = { 0.0f, 198.0f / 255.0f, 244.0f / 255.0f };
static const float reticleSize = TWO_PI / 100.0f;

static const float CURSOR_PIXEL_SIZE = 32.0f;
static const float MOUSE_PITCH_RANGE = 1.0f * PI;
static const float MOUSE_YAW_RANGE = 0.5f * TWO_PI;
static const glm::vec2 MOUSE_RANGE(MOUSE_YAW_RANGE, MOUSE_PITCH_RANGE);

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

ApplicationCompositor::ApplicationCompositor() {
    memset(_reticleActive, 0, sizeof(_reticleActive));
    memset(_magActive, 0, sizeof(_reticleActive));
    memset(_magSizeMult, 0, sizeof(_magSizeMult));

    auto geometryCache = DependencyManager::get<GeometryCache>();

    _reticleQuad = geometryCache->allocateID();
    _magnifierQuad = geometryCache->allocateID();
    _magnifierBorder = geometryCache->allocateID();

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
    if (_alpha == 0.0f) {
        return;
    }

    gpu::FramebufferPointer overlayFramebuffer = qApp->getApplicationOverlay().getOverlayFramebuffer();
    if (!overlayFramebuffer) {
        return;
    }

    updateTooltips();

    auto deviceSize = qApp->getDeviceSize();

    //Handle fading and deactivation/activation of UI
    gpu::Batch batch;

    renderArgs->_context->syncCache();
    auto geometryCache = DependencyManager::get<GeometryCache>();

    geometryCache->useSimpleDrawPipeline(batch);
    batch.setViewportTransform(glm::ivec4(0, 0, deviceSize.width(), deviceSize.height()));
    batch.setModelTransform(Transform());
    batch.setViewTransform(Transform());
    batch.setProjectionTransform(mat4());
    batch.setResourceTexture(0, overlayFramebuffer->getRenderBuffer(0));
    geometryCache->renderUnitQuad(batch, vec4(vec3(1), _alpha));

    // Doesn't actually render
    renderPointers(batch);

    //draw the mouse pointer
    // Get the mouse coordinates and convert to NDC [-1, 1]
    vec2 canvasSize = qApp->getCanvasSize();
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
    renderArgs->_context->render(batch);
}


vec2 getPolarCoordinates(const PalmData& palm) {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    auto avatarOrientation = myAvatar->getOrientation();
    auto eyePos = myAvatar->getDefaultEyePosition();
    glm::vec3 tip = myAvatar->getLaserPointerTipPosition(&palm);
    // Direction of the tip relative to the eye
    glm::vec3 tipDirection = tip - eyePos;
    // orient into avatar space
    tipDirection = glm::inverse(avatarOrientation) * tipDirection;
    // Normalize for trig functions
    tipDirection = glm::normalize(tipDirection);
    // Convert to polar coordinates
    glm::vec2 polar(glm::atan(tipDirection.x, -tipDirection.z), glm::asin(tipDirection.y));
    return polar;
}

// Draws the FBO texture for Oculus rift.
void ApplicationCompositor::displayOverlayTextureHmd(RenderArgs* renderArgs, int eye) {
    PROFILE_RANGE(__FUNCTION__);
    if (_alpha == 0.0f) {
        return;
    }

    gpu::FramebufferPointer overlayFramebuffer = qApp->getApplicationOverlay().getOverlayFramebuffer();
    if (!overlayFramebuffer) {
        return;
    }

    updateTooltips();

    vec2 canvasSize = qApp->getCanvasSize();
    _textureAspectRatio = aspect(canvasSize);

    renderArgs->_context->syncCache();
    auto geometryCache = DependencyManager::get<GeometryCache>();

    gpu::Batch batch;
    geometryCache->useSimpleDrawPipeline(batch);
    batch._glDisable(GL_DEPTH_TEST);
    batch._glDisable(GL_CULL_FACE);
    //batch._glBindTexture(GL_TEXTURE_2D, texture);
    //batch._glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //batch._glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    batch.setResourceTexture(0, overlayFramebuffer->getRenderBuffer(0));

    batch.setViewTransform(Transform());
    batch.setProjectionTransform(qApp->getEyeProjection(eye));

    mat4 eyePose = qApp->getEyePose(eye);
    glm::mat4 overlayXfm = glm::inverse(eyePose);

#ifdef DEBUG_OVERLAY
    {
        batch.setModelTransform(glm::translate(mat4(), vec3(0, 0, -2)));
        geometryCache->renderUnitQuad(batch, glm::vec4(1));
    }
#else
    {
        batch.setModelTransform(overlayXfm);
        drawSphereSection(batch);
    }
#endif

    // Doesn't actually render
    renderPointers(batch);
    vec3 reticleScale = vec3(Cursor::Manager::instance().getScale() * reticleSize);

    bindCursorTexture(batch);

    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    //Controller Pointers
    for (int i = 0; i < (int)myAvatar->getHand()->getNumPalms(); i++) {
        PalmData& palm = myAvatar->getHand()->getPalms()[i];
        if (palm.isActive()) {
            glm::vec2 polar = getPolarCoordinates(palm);
            // Convert to quaternion
            mat4 pointerXfm = glm::mat4_cast(quat(vec3(polar.y, -polar.x, 0.0f))) * glm::translate(mat4(), vec3(0, 0, -1));
            mat4 reticleXfm = overlayXfm * pointerXfm;
            reticleXfm = glm::scale(reticleXfm, reticleScale);
            batch.setModelTransform(reticleXfm);
            // Render reticle at location
            geometryCache->renderUnitQuad(batch, glm::vec4(1), _reticleQuad);
        }
    }

    //Mouse Pointer
    if (_reticleActive[MOUSE]) {
        glm::vec2 projection = screenToSpherical(glm::vec2(_reticlePosition[MOUSE].x(),
            _reticlePosition[MOUSE].y()));
        mat4 pointerXfm = glm::mat4_cast(quat(vec3(-projection.y, projection.x, 0.0f))) * glm::translate(mat4(), vec3(0, 0, -1));
        mat4 reticleXfm = overlayXfm * pointerXfm;
        reticleXfm = glm::scale(reticleXfm, reticleScale);
        batch.setModelTransform(reticleXfm);
        geometryCache->renderUnitQuad(batch, glm::vec4(1), _reticleQuad);
    }

    renderArgs->_context->render(batch);
}


void ApplicationCompositor::computeHmdPickRay(glm::vec2 cursorPos, glm::vec3& origin, glm::vec3& direction) const {
    cursorPos *= qApp->getCanvasSize();
    const glm::vec2 projection = screenToSpherical(cursorPos);
    // The overlay space orientation of the mouse coordinates
    const glm::quat orientation(glm::vec3(-projection.y, projection.x, 0.0f));
    // FIXME We now have the direction of the ray FROM THE DEFAULT HEAD POSE.
    // Now we need to account for the actual camera position relative to the overlay
    glm::vec3 overlaySpaceDirection = glm::normalize(orientation * IDENTITY_FRONT);


    // We need the RAW camera orientation and position, because this is what the overlay is
    // rendered relative to
    const glm::vec3 overlayPosition = qApp->getCamera()->getPosition();
    const glm::quat overlayOrientation = qApp->getCamera()->getRotation();

    // Intersection UI overlay space
    glm::vec3 worldSpaceDirection = overlayOrientation * overlaySpaceDirection;
    glm::vec3 worldSpaceIntersection = (glm::normalize(worldSpaceDirection) * _oculusUIRadius) + overlayPosition;
    glm::vec3 worldSpaceHeadPosition = (overlayOrientation * glm::vec3(qApp->getHeadPose()[3])) + overlayPosition;

    // Intersection in world space
    origin = worldSpaceHeadPosition;
    direction = glm::normalize(worldSpaceIntersection - worldSpaceHeadPosition);
}

//Caculate the click location using one of the sixense controllers. Scale is not applied
QPoint ApplicationCompositor::getPalmClickLocation(const PalmData *palm) const {
    QPoint rv;
    auto canvasSize = qApp->getCanvasSize();
    if (qApp->isHMDMode()) {
        glm::vec2 polar = getPolarCoordinates(*palm);
        glm::vec2 point = sphericalToScreen(-polar);
        rv.rx() = point.x;
        rv.ry() = point.y;
    } else {
        MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        glm::mat4 projection;
        qApp->getDisplayViewFrustum()->evalProjectionMatrix(projection);
        glm::quat invOrientation = glm::inverse(myAvatar->getOrientation());
        glm::vec3 eyePos = myAvatar->getDefaultEyePosition();
        glm::vec3 tip = myAvatar->getLaserPointerTipPosition(palm);
        glm::vec3 tipPos = invOrientation * (tip - eyePos);

        glm::vec4 clipSpacePos = glm::vec4(projection * glm::vec4(tipPos, 1.0f));
        glm::vec3 ndcSpacePos;
        if (clipSpacePos.w != 0) {
            ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
        }

        rv.setX(((ndcSpacePos.x + 1.0f) / 2.0f) * canvasSize.x);
        rv.setY((1.0f - ((ndcSpacePos.y + 1.0f) / 2.0f)) * canvasSize.y);
    }
    return rv;
}

//Finds the collision point of a world space ray
bool ApplicationCompositor::calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    glm::quat inverseOrientation = glm::inverse(myAvatar->getOrientation());

    glm::vec3 relativePosition = inverseOrientation * (position - myAvatar->getDefaultEyePosition());
    glm::vec3 relativeDirection = glm::normalize(inverseOrientation * direction);

    float t;
    if (raySphereIntersect(relativeDirection, relativePosition, _oculusUIRadius * myAvatar->getScale(), &t)){
        result = position + direction * t;
        return true;
    }

    return false;
}

//Renders optional pointers
void ApplicationCompositor::renderPointers(gpu::Batch& batch) {
    if (qApp->isHMDMode() && !qApp->getLastMouseMoveWasSimulated() && !qApp->isMouseHidden()) {
        //If we are in oculus, render reticle later
        QPoint position = QPoint(qApp->getTrueMouseX(), qApp->getTrueMouseY());
        _reticlePosition[MOUSE] = position;
        _reticleActive[MOUSE] = true;
        _magActive[MOUSE] = _magnifier;
        _reticleActive[LEFT_CONTROLLER] = false;
        _reticleActive[RIGHT_CONTROLLER] = false;
    } else if (qApp->getLastMouseMoveWasSimulated() && Menu::getInstance()->isOptionChecked(MenuOption::SixenseMouseInput)) {
        //only render controller pointer if we aren't already rendering a mouse pointer
        _reticleActive[MOUSE] = false;
        _magActive[MOUSE] = false;
        renderControllerPointers(batch);
    }
}


void ApplicationCompositor::renderControllerPointers(gpu::Batch& batch) {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    //Static variables used for storing controller state
    static quint64 pressedTime[NUMBER_OF_RETICLES] = { 0ULL, 0ULL, 0ULL };
    static bool isPressed[NUMBER_OF_RETICLES] = { false, false, false };
    static bool stateWhenPressed[NUMBER_OF_RETICLES] = { false, false, false };

    const HandData* handData = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHandData();

    for (unsigned int palmIndex = 2; palmIndex < 4; palmIndex++) {
        const int index = palmIndex - 1;

        const PalmData* palmData = NULL;

        if (palmIndex >= handData->getPalms().size()) {
            return;
        }

        if (handData->getPalms()[palmIndex].isActive()) {
            palmData = &handData->getPalms()[palmIndex];
        } else {
            continue;
        }

        int controllerButtons = palmData->getControllerButtons();

        //Check for if we should toggle or drag the magnification window
        if (controllerButtons & BUTTON_3) {
            if (isPressed[index] == false) {
                //We are now dragging the window
                isPressed[index] = true;
                //set the pressed time in us
                pressedTime[index] = usecTimestampNow();
                stateWhenPressed[index] = _magActive[index];
            }
        } else if (isPressed[index]) {
            isPressed[index] = false;
            //If the button was only pressed for < 250 ms
            //then disable it.

            const int MAX_BUTTON_PRESS_TIME = 250 * MSECS_TO_USECS;
            if (usecTimestampNow() < pressedTime[index] + MAX_BUTTON_PRESS_TIME) {
                _magActive[index] = !stateWhenPressed[index];
            }
        }

        //if we have the oculus, we should make the cursor smaller since it will be
        //magnified
        if (qApp->isHMDMode()) {

            QPoint point = getPalmClickLocation(palmData);

            _reticlePosition[index] = point;

            //When button 2 is pressed we drag the mag window
            if (isPressed[index]) {
                _magActive[index] = true;
            }

            // If oculus is enabled, we draw the crosshairs later
            continue;
        }

        auto canvasSize = qApp->getCanvasSize();
        int mouseX, mouseY;
        if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseLasers)) {
            QPoint res = getPalmClickLocation(palmData);
            mouseX = res.x();
            mouseY = res.y();
        } else {
            // Get directon relative to avatar orientation
            glm::vec3 direction = glm::inverse(myAvatar->getOrientation()) * palmData->getFingerDirection();

            // Get the angles, scaled between (-0.5,0.5)
            float xAngle = (atan2(direction.z, direction.x) + PI_OVER_TWO);
            float yAngle = 0.5f - ((atan2f(direction.z, direction.y) + (float)PI_OVER_TWO));

            // Get the pixel range over which the xAngle and yAngle are scaled
            float cursorRange = canvasSize.x * SixenseManager::getInstance().getCursorPixelRangeMult();

            mouseX = (canvasSize.x / 2.0f + cursorRange * xAngle);
            mouseY = (canvasSize.y / 2.0f + cursorRange * yAngle);
        }

        //If the cursor is out of the screen then don't render it
        if (mouseX < 0 || mouseX >= (int)canvasSize.x || mouseY < 0 || mouseY >= (int)canvasSize.y) {
            _reticleActive[index] = false;
            continue;
        }
        _reticleActive[index] = true;


        const float reticleSize = 40.0f;

        mouseX -= reticleSize / 2.0f;
        mouseY += reticleSize / 2.0f;


        glm::vec2 topLeft(mouseX, mouseY);
        glm::vec2 bottomRight(mouseX + reticleSize, mouseY - reticleSize);
        glm::vec2 texCoordTopLeft(0.0f, 0.0f);
        glm::vec2 texCoordBottomRight(1.0f, 1.0f);

        DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight,
                                                            glm::vec4(RETICLE_COLOR[0], RETICLE_COLOR[1], RETICLE_COLOR[2], 1.0f));

    }
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
    gpu::BufferView posView(_hemiVertices, 0, _hemiVertices->getSize(), VERTEX_STRIDE, streamFormat->getAttributes().at(gpu::Stream::POSITION)._element);
    gpu::BufferView uvView(_hemiVertices, sizeof(vec3), _hemiVertices->getSize(), VERTEX_STRIDE, streamFormat->getAttributes().at(gpu::Stream::TEXCOORD)._element);
    gpu::BufferView colView(_hemiVertices, sizeof(vec3) + sizeof(vec2), _hemiVertices->getSize(), VERTEX_STRIDE, streamFormat->getAttributes().at(gpu::Stream::COLOR)._element);
    batch.setInputBuffer(VERTEX_DATA_SLOT, posView);
    batch.setInputBuffer(TEXTURE_DATA_SLOT, uvView);
    batch.setInputBuffer(COLOR_DATA_SLOT, colView);
    batch.setIndexBuffer(gpu::UINT16, _hemiIndices, 0);
    batch.drawIndexed(gpu::TRIANGLES, _hemiIndexCount);
}

glm::vec2 ApplicationCompositor::directionToSpherical(const glm::vec3& direction) {
    glm::vec2 result;
    // Compute yaw
    glm::vec3 normalProjection = glm::normalize(glm::vec3(direction.x, 0.0f, direction.z));
    result.x = glm::acos(glm::dot(IDENTITY_FRONT, normalProjection));
    if (glm::dot(IDENTITY_RIGHT, normalProjection) > 0.0f) {
        result.x = -glm::abs(result.x);
    } else {
        result.x = glm::abs(result.x);
    }
    // Compute pitch
    result.y = angleBetween(IDENTITY_UP, direction) - PI_OVER_TWO;

    return result;
}

glm::vec3 ApplicationCompositor::sphericalToDirection(const glm::vec2& sphericalPos) {
    glm::quat rotation(glm::vec3(sphericalPos.y, sphericalPos.x, 0.0f));
    return rotation * IDENTITY_FRONT;
}

glm::vec2 ApplicationCompositor::screenToSpherical(const glm::vec2& screenPos) {
    auto screenSize = qApp->getCanvasSize();
    glm::vec2 result;
    result.x = -(screenPos.x / screenSize.x - 0.5f);
    result.y = (screenPos.y / screenSize.y - 0.5f);
    result.x *= MOUSE_YAW_RANGE;
    result.y *= MOUSE_PITCH_RANGE;

    return result;
}

glm::vec2 ApplicationCompositor::sphericalToScreen(const glm::vec2& sphericalPos) {
    glm::vec2 result = sphericalPos;
    result.x *= -1.0f;
    result /= MOUSE_RANGE;
    result += 0.5f;
    result *= qApp->getCanvasSize();
    return result;
}

glm::vec2 ApplicationCompositor::sphericalToOverlay(const glm::vec2&  sphericalPos) const {
    glm::vec2 result = sphericalPos;
    result.x *= -1.0f;
    result /= _textureFov;
    result.x /= _textureAspectRatio;
    result += 0.5f;
    result *= qApp->getCanvasSize();
    return result;
}

glm::vec2 ApplicationCompositor::overlayToSpherical(const glm::vec2&  overlayPos) const {
    glm::vec2 result = overlayPos;
    result /= qApp->getCanvasSize();
    result -= 0.5f;
    result *= _textureFov;
    result.x *= _textureAspectRatio;
    result.x *= -1.0f;
    return result;
}

glm::vec2 ApplicationCompositor::screenToOverlay(const glm::vec2& screenPos) const {
    return sphericalToOverlay(screenToSpherical(screenPos));
}

glm::vec2 ApplicationCompositor::overlayToScreen(const glm::vec2& overlayPos) const {
    return sphericalToScreen(overlayToSpherical(overlayPos));
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
