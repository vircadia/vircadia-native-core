#include "ApplicationOverlayCompositor.h"

#include <TextureCache.h>
#include <PathUtils.h>
#include <ViewFrustum.h>
#include <MatrixStack.h>
#include <gpu/GLBackend.h>

#define DEFAULT_HMD_UI_ANGULAR_SIZE 72.0f

ApplicationOverlayCompositor::ApplicationOverlayCompositor() {
    using namespace oglplus;

    auto currentContext = QOpenGLContext::currentContext();
    Q_ASSERT(currentContext);
    _canvas.create(currentContext);
    _canvas.makeCurrent();
    Q_ASSERT(0 == glGetError());
    currentContext = QOpenGLContext::currentContext();
    Q_ASSERT(currentContext);

    Context::ClearColor(0, 0, 0, 1);
    Q_ASSERT(0 == glGetError());
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    Q_ASSERT(0 == glGetError());
    Context::Disable(Capability::Blend);
    Q_ASSERT(0 == glGetError());
    Context::Disable(Capability::DepthTest);
    Q_ASSERT(0 == glGetError());
    Context::Disable(Capability::CullFace);
    Q_ASSERT(0 == glGetError());

    _program = loadDefaultShader();
    _plane = loadPlane(_program);

    _program = loadDefaultShader();
    _plane = loadPlane(_program);
    _hmdUiSurface = loadSphereSection(_program, glm::radians(DEFAULT_HMD_UI_ANGULAR_SIZE));

    _crosshairTexture = DependencyManager::get<TextureCache>()->
        getImageTexture(PathUtils::resourcesPath() + "images/sixense-reticle.png");
}


GLuint ApplicationOverlayCompositor::composite(DisplayPlugin* plugin,
        GLuint sceneTexture, const glm::uvec2& sceneSize,
        GLuint overlayTexture, const glm::uvec2& overlaySize) {
    using namespace oglplus;
    _canvas.makeCurrent();
    if (!_fbo || sceneSize != _fbo->size) {
        _fbo = BasicFramebufferWrapperPtr(new BasicFramebufferWrapper());
        _fbo->Init(sceneSize);
    }
    _fbo->Bound(Framebuffer::Target::Draw, [&] {
        Context::Clear().ColorBuffer();
        if (plugin->isHmd()) {
            compositeHmd(plugin, sceneTexture, sceneSize, overlayTexture, overlaySize);
        } else if (plugin->isStereo()) {
            compositeStereo(plugin, sceneTexture, sceneSize, overlayTexture, overlaySize);
        } else {
            composite2D(plugin, sceneTexture, sceneSize, overlayTexture, overlaySize);
        }
    });
    GLuint result = GetName(_fbo->color);
    _canvas.doneCurrent();
    return result;
}

void ApplicationOverlayCompositor::composite2D(DisplayPlugin* plugin,
    GLuint sceneTexture, const glm::uvec2& sceneSize,
    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    using namespace oglplus;
    const uvec2 size = toGlm(plugin->getDeviceSize());

    Q_ASSERT(0 == glGetError());

    Context::Viewport(size.x, size.y);
    _program->Bind();
    Mat4Uniform(*_program, "ModelView").Set(mat4());
    Mat4Uniform(*_program, "Projection").Set(mat4());
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    _plane->Use();
    _plane->Draw();
    Context::Enable(Capability::Blend);
    glBindTexture(GL_TEXTURE_2D, overlayTexture);
    _plane->Draw();
    Context::Disable(Capability::Blend);
    glBindTexture(GL_TEXTURE_2D, 0);

    // FIXME add the cursor

    Q_ASSERT(0 == glGetError());
}


template <typename F>
void sbs_for_each_eye(const uvec2& size, F f) {
    QRect r(QPoint(0, 0), QSize(size.x / 2, size.y));
    for_each_eye([&](Eye eye) {
        oglplus::Context::Viewport(r.x(), r.y(), r.width(), r.height());
        f(eye);
    }, [&] {
        r.moveLeft(r.width());
    });
}

void ApplicationOverlayCompositor::compositeStereo(DisplayPlugin* plugin,
    GLuint sceneTexture, const glm::uvec2& sceneSize,
    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    using namespace oglplus;

    Q_ASSERT(0 == glGetError());
    const uvec2 size = sceneSize;

    Context::Viewport(size.x, size.y);

    _program->Bind();

    Mat4Uniform(*_program, "ModelView").Set(mat4());
    Mat4Uniform(*_program, "Projection").Set(mat4());

    glBindTexture(GL_TEXTURE_2D, sceneTexture);

    _plane->Use();
    _plane->Draw();

    // FIXME the 
    const float screenAspect = aspect(size);
    const GLfloat distance = 1.0f;
    const GLfloat halfQuadHeight = distance * tan(DEFAULT_FIELD_OF_VIEW_DEGREES);
    const GLfloat halfQuadWidth = halfQuadHeight * screenAspect;
    const GLfloat quadWidth = halfQuadWidth * 2.0f;
    const GLfloat quadHeight = halfQuadHeight * 2.0f;

    vec3 quadSize(quadWidth, quadHeight, 1.0f);
    quadSize = vec3(1.0f) / quadSize;

    using namespace oglplus;
    Context::Enable(Capability::Blend);
    glBindTexture(GL_TEXTURE_2D, overlayTexture);

    mat4 pr = glm::perspective(glm::radians(DEFAULT_FIELD_OF_VIEW_DEGREES), screenAspect, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP);
    Mat4Uniform(*_program, "Projection").Set(pr);

    // Position the camera relative to the overlay texture
    MatrixStack mv;
    mv.translate(vec3(0, 0, -distance)).scale(vec3(0.7f, 0.7f / screenAspect, 1.0f)); // .scale(vec3(quadWidth, quadHeight, 1.0));
    sbs_for_each_eye(size, [&](Eye eye) {
        mv.withPush([&] {
            // translate 
            mv.top() = plugin->getModelview(eye, mv.top());
            Mat4Uniform(*_program, "ModelView").Set(mv.top());
            _plane->Draw();
        });
    });

    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(_crosshairTexture));
    glm::vec2 canvasSize = plugin->getCanvasSize();
    glm::vec2 mouse = vec2(0); // toGlm(_window->mapFromGlobal(QCursor::pos()));
    mouse /= canvasSize;
    mouse *= 2.0f;
    mouse -= 1.0f;
    mouse.y *= -1.0f;
    sbs_for_each_eye(size, [&](Eye eye) {
        mv.withPush([&] {
            // translate 
            mv.top() = plugin->getModelview(eye, mv.top());
            mv.translate(mouse);
            //mv.scale(0.05f);
            mv.scale(vec3(0.025f, 0.05f, 1.0f));
            Mat4Uniform(*_program, "ModelView").Set(mv.top());
            _plane->Draw();
        });
    });
    Context::Disable(Capability::Blend);

}

void ApplicationOverlayCompositor::compositeHmd(DisplayPlugin* plugin,
    GLuint sceneTexture, const glm::uvec2& sceneSize,
    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    using namespace oglplus;
    auto size = sceneSize;

    Context::Viewport(size.x, size.y);

    glClearColor(0, 0, 0, 0);
    Context::Clear().ColorBuffer();

    _program->Bind();
    Mat4Uniform(*_program, "Projection").Set(mat4());
    Mat4Uniform(*_program, "ModelView").Set(mat4());
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    _plane->Use();
    _plane->Draw();


    Context::Enable(Capability::Blend);
    glBindTexture(GL_TEXTURE_2D, overlayTexture);
    for_each_eye([&](Eye eye) {
        Context::Viewport(eye == Left ? 0 : size.x / 2, 0, size.x / 2, size.y);
        glm::mat4 m = plugin->getProjection(eye, glm::mat4());
        Mat4Uniform(*_program, "Projection").Set(m);
        Mat4Uniform(*_program, "ModelView").Set(glm::scale(glm::inverse(plugin->getModelview(eye, mat4())), vec3(1)));
        _hmdUiSurface->Use();
        _hmdUiSurface->Draw();
    });
    Context::Disable(Capability::Blend);
}
