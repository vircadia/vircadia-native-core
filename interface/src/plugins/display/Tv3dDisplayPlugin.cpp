//  Tv3dDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Tv3dDisplayPlugin.h"
#include <GlWindow.h>
#include <QApplication>
#include <QDesktopWidget>

#include "gpu/Texture.h"
#include "gpu/GLBackend.h"
#include "PathUtils.h"

#include "Application.h"
#include "ui/ApplicationOverlay.h"


#include <GL/glew.h>
#define OGLPLUS_LOW_PROFILE 1
#define OGLPLUS_USE_GLEW 1
#define OGLPLUS_USE_BOOST_CONFIG 1
#define OGLPLUS_NO_SITE_CONFIG 1
#define OGLPLUS_USE_GLCOREARB_H 0
#include <oglplus/gl.hpp>

#pragma warning(disable : 4068)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
#pragma warning( disable : 4244 4267 4065 4101)
#include <oglplus/all.hpp>
#include <oglplus/interop/glm.hpp>
#include <oglplus/bound/texture.hpp>
#include <oglplus/bound/framebuffer.hpp>
#include <oglplus/bound/renderbuffer.hpp>
#include <oglplus/shapes/wrapper.hpp>
#include <oglplus/shapes/plane.hpp>
#pragma warning( default : 4244 4267 4065 4101)
#pragma clang diagnostic pop

typedef std::shared_ptr<oglplus::shapes::ShapeWrapper> ShapeWrapperPtr;
typedef std::shared_ptr<oglplus::Buffer> BufferPtr;
typedef std::shared_ptr<oglplus::VertexArray> VertexArrayPtr;
typedef std::shared_ptr<oglplus::Program> ProgramPtr;
typedef oglplus::Uniform<mat4> Mat4Uniform;

const QString Tv3dDisplayPlugin::NAME("Tv3dDisplayPlugin");

const QString & Tv3dDisplayPlugin::getName() {
    return NAME;
}

Tv3dDisplayPlugin::Tv3dDisplayPlugin() {
    connect(&_timer, &QTimer::timeout, this, [&] {
        emit requestRender();
    });
}

gpu::TexturePointer _crosshairTexture; 


void compileProgram(ProgramPtr & result, std::string vs, std::string fs) {
    using namespace oglplus;
    try {
        result = ProgramPtr(new Program());
        // attach the shaders to the program
        result->AttachShader(
            VertexShader()
            .Source(GLSLSource(vs))
            .Compile()
            );
        result->AttachShader(
            FragmentShader()
            .Source(GLSLSource(fs))
            .Compile()
            );
        result->Link();
    } catch (ProgramBuildError & err) {
        qFatal((const char*)err.Message);
        result.reset();
    }
}


ShapeWrapperPtr loadPlane(ProgramPtr program, float aspect) {
    using namespace oglplus;
    Vec3f a(1, 0, 0);
    Vec3f b(0, 1, 0);
    if (aspect > 1) {
        b[1] /= aspect;
    } else {
        a[0] *= aspect;
    }
    return ShapeWrapperPtr(
        new shapes::ShapeWrapper({ "Position", "TexCoord" }, shapes::Plane(a, b), *program)
    );
}


static const char * QUAD_VS = R"VS(#version 330

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);
uniform vec2 UvMultiplier = vec2(1);

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = Projection * ModelView * vec4(Position, 1);
  vTexCoord = TexCoord * UvMultiplier;
}
)VS";

static const char * QUAD_FS = R"FS(#version 330
uniform sampler2D sampler;
uniform float Alpha = 1.0;

in vec2 vTexCoord;
out vec4 vFragColor;

void main() {
    vec4 c = texture(sampler, vTexCoord);
    c.a = min(Alpha, c.a);
    vFragColor = c; 
    //vFragColor = vec4(fract(vTexCoord), log(vTexCoord.x), 1.0);
}
)FS";

static ProgramPtr program;
static ShapeWrapperPtr plane;

void Tv3dDisplayPlugin::display(
    GLuint sceneTexture, const glm::uvec2& sceneSize,
    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    QSize size = getDeviceSize();
    makeCurrent();


    if (!program) {
        using namespace oglplus;
        Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
        Context::Disable(Capability::Blend);
        Context::Disable(Capability::DepthTest);
        Context::Disable(Capability::CullFace);

        program = ProgramPtr(new oglplus::Program());
        compileProgram(program, QUAD_VS, QUAD_FS);
        plane = loadPlane(program, 1.0f);
        _crosshairTexture = DependencyManager::get<TextureCache>()->
            getImageTexture(PathUtils::resourcesPath() + "images/sixense-reticle.png");
    }



    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, size.width(), size.height());
    Mat4Uniform(*program, "ModelView").Set(mat4());
    Mat4Uniform(*program, "Projection").Set(mat4());
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    plane->Draw();

    const float overlayAspect = aspect(toGlm(size));
    const GLfloat distance = 1.0f;
    const GLfloat halfQuadHeight = distance * tan(DEFAULT_FIELD_OF_VIEW_DEGREES);
    const GLfloat halfQuadWidth = halfQuadHeight * (float)size.width() / (float)size.height();
    const GLfloat quadWidth = halfQuadWidth * 2.0f;
    const GLfloat quadHeight = halfQuadHeight * 2.0f;

    vec3 quadSize(quadWidth, quadHeight, 1.0f);
    quadSize = vec3(1.0f) / quadSize;

    using namespace oglplus;

    Context::Enable(Capability::Blend);
    glBindTexture(GL_TEXTURE_2D, overlayTexture);
    
    mat4 pr = glm::perspective(glm::radians(DEFAULT_FIELD_OF_VIEW_DEGREES), aspect(toGlm(size)), DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP);
    Mat4Uniform(*program, "Projection").Set(pr);

    MatrixStack mv;
    mv.translate(vec3(0, 0, -distance)).scale(vec3(0.7f, 0.7f / overlayAspect, 1.0f)); // .scale(vec3(quadWidth, quadHeight, 1.0));

    QRect r(QPoint(0, 0), QSize(size.width() / 2, size.height()));
    for (int i = 0; i < 2; ++i) {
        Context::Viewport(r.x(), r.y(), r.width(), r.height());
        Mat4Uniform(*program, "ModelView").Set(mv.top());
        plane->Draw();
        r.moveLeft(r.width());
    }

    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(_crosshairTexture));
    glm::vec2 canvasSize = qApp->getCanvasSize();
    glm::vec2 mouse = qApp->getMouse();
    mouse /= canvasSize;
    mouse *= 2.0f;
    mouse -= 1.0f;
    mouse.y *= -1.0f;
    mv.translate(mouse);
    mv.scale(0.1f);
    Mat4Uniform(*program, "ModelView").Set(mv.top());
    r = QRect(QPoint(0, 0), QSize(size.width() / 2, size.height()));
    for (int i = 0; i < 2; ++i) {
        Context::Viewport(r.x(), r.y(), r.width(), r.height());
        plane->Draw();
        r.moveLeft(r.width());
    }
    Context::Disable(Capability::Blend);
}


void Tv3dDisplayPlugin::activate() {
    GlWindowDisplayPlugin::activate();
    _window->installEventFilter(this);
    _window->setFlags(Qt::FramelessWindowHint);
    auto desktop = QApplication::desktop();
    _window->setGeometry(desktop->screenGeometry());
    _window->setCursor(Qt::BlankCursor);
    _window->show();

    _timer.start(8);
}

void Tv3dDisplayPlugin::deactivate() {
    makeCurrent();
    if (plane) {
        plane.reset();
        program.reset();
        _crosshairTexture.reset();
    }
    _timer.stop();
    GlWindowDisplayPlugin::deactivate();
}
/*
std::function<QPointF(const QPointF&)> Tv3dDisplayPlugin::getMouseTranslator() {
    return [=](const QPointF& point){
        QPointF result{ point };
        QSize size = getDeviceSize();
        result.rx() *= 2.0f;
        if (result.x() > size.width()) {
            result.rx() -= size.width();
        }
        return result;
    };
}

glm::ivec2 Tv3dDisplayPlugin::trueMouseToUiMouse(const glm::ivec2 & position) const {
    ivec2 result{ position };
    uvec2 size = getCanvasSize();
    result.x *= 2;
    result.x %= size.x;
    return result;
}

*/

/*
void Tv3dDisplayPlugin::activate() {
    GlWindowDisplayPlugin::activate();
    _window->setPosition(100, 100);
    _window->resize(512, 512);
    _window->setVisible(true);
    _window->show();
}

void Tv3dDisplayPlugin::deactivate() {
    GlWindowDisplayPlugin::deactivate();
}
*/

/*
glm::ivec2 LegacyDisplayPlugin::getCanvasSize() const {
    return toGlm(_window->size());
}

bool LegacyDisplayPlugin::hasFocus() const {
    return _window->hasFocus();
}

PickRay LegacyDisplayPlugin::computePickRay(const glm::vec2 & pos) const {
    return PickRay();
}

bool isMouseOnScreen() {
    return false;
}

void LegacyDisplayPlugin::preDisplay() {
    SimpleDisplayPlugin::preDisplay();
    auto size = toGlm(_window->size());
    glViewport(0, 0, size.x, size.y);
}

bool LegacyDisplayPlugin::isThrottled() const {
    return _window->isThrottleRendering();


    */

#if 0

int TV3DManager::_screenWidth = 1;
int TV3DManager::_screenHeight = 1;
double TV3DManager::_aspect = 1.0;
eyeFrustum TV3DManager::_leftEye;
eyeFrustum TV3DManager::_rightEye;
eyeFrustum* TV3DManager::_activeEye = NULL;


bool TV3DManager::isConnected() {
    return Menu::getInstance()->isOptionChecked(MenuOption::Enable3DTVMode);
}

void TV3DManager::connect() {
    auto deviceSize = qApp->getDeviceSize();
    configureCamera(*(qApp->getCamera()), deviceSize.width(), deviceSize.height());
}


// The basic strategy of this stereoscopic rendering is explained here:
//    http://www.orthostereo.com/geometryopengl.html
void TV3DManager::setFrustum(const Camera& whichCamera) {
    const double DTR = 0.0174532925; // degree to radians
    const double IOD = 0.05; //intraocular distance
    double fovy = DEFAULT_FIELD_OF_VIEW_DEGREES; // field of view in y-axis
    double nearZ = DEFAULT_NEAR_CLIP; // near clipping plane
    double screenZ = 0.25f; // screen projection plane

    double top = nearZ * tan(DTR * fovy / 2.0); //sets top of frustum based on fovy and near clipping plane
    double right = _aspect * top; // sets right of frustum based on aspect ratio
    double frustumshift = (IOD / 2) * nearZ / screenZ;

    _leftEye.top = top;
    _leftEye.bottom = -top;
    _leftEye.left = -right + frustumshift;
    _leftEye.right = right + frustumshift;
    _leftEye.modelTranslation = IOD / 2;

    _rightEye.top = top;
    _rightEye.bottom = -top;
    _rightEye.left = -right - frustumshift;
    _rightEye.right = right - frustumshift;
    _rightEye.modelTranslation = -IOD / 2;
}

void TV3DManager::configureCamera(Camera& whichCamera_, int screenWidth, int screenHeight) {
    const Camera& whichCamera = whichCamera_;

    if (screenHeight == 0) {
        screenHeight = 1; // prevent divide by 0
    }
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    _aspect = (double)_screenWidth / (double)_screenHeight;
    setFrustum(whichCamera);

    glViewport(0, 0, _screenWidth, _screenHeight); // sets drawing viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void TV3DManager::display(Camera& whichCamera) {
    double nearZ = DEFAULT_NEAR_CLIP; // near clipping plane
    double farZ = DEFAULT_FAR_CLIP; // far clipping plane

    // left eye portal
    int portalX = 0;
    int portalY = 0;
    QSize deviceSize = qApp->getDeviceSize() *
        qApp->getRenderResolutionScale();
    int portalW = deviceSize.width() / 2;
    int portalH = deviceSize.height();


    DependencyManager::get<GlowEffect>()->prepare();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Camera eyeCamera;
    eyeCamera.setRotation(whichCamera.getRotation());
    eyeCamera.setPosition(whichCamera.getPosition());

    glEnable(GL_SCISSOR_TEST);
    glPushMatrix();
    forEachEye([&](eyeFrustum& eye) {
        _activeEye = &eye;
        glViewport(portalX, portalY, portalW, portalH);
        glScissor(portalX, portalY, portalW, portalH);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity(); // reset projection matrix
        glFrustum(eye.left, eye.right, eye.bottom, eye.top, nearZ, farZ); // set left view frustum
        GLfloat p[4][4];
        // Really?
        glGetFloatv(GL_PROJECTION_MATRIX, &(p[0][0]));
        float cotangent = p[1][1];
        GLfloat fov = atan(1.0f / cotangent);
        glTranslatef(eye.modelTranslation, 0.0, 0.0); // translate to cancel parallax

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        qApp->displaySide(eyeCamera, false, RenderArgs::MONO);
#if 0
        qApp->getApplicationOverlay().displayOverlayTextureStereo(whichCamera, _aspect, fov);
#endif
        _activeEye = NULL;
    }, [&] {
        // render right side view
        portalX = deviceSize.width() / 2;
    });
    glPopMatrix();
    glDisable(GL_SCISSOR_TEST);

    auto finalFbo = DependencyManager::get<GlowEffect>()->render();
    auto fboSize = finalFbo->getSize();
    // Get the ACTUAL device size for the BLIT
    deviceSize = qApp->getDeviceSize();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(finalFbo));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, fboSize.x, fboSize.y,
        0, 0, deviceSize.width(), deviceSize.height(),
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // reset the viewport to how we started
    glViewport(0, 0, deviceSize.width(), deviceSize.height());
}

void TV3DManager::overrideOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
    float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) {
    if (_activeEye) {
        left = _activeEye->left;
        right = _activeEye->right;
        bottom = _activeEye->bottom;
        top = _activeEye->top;
    }
}

#endif
