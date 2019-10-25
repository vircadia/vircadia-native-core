#include "MacQml.h"

#include <cmath>

#include <QtQuick/QQuickItem>

#include <SharedUtil.h>

using TextureAndFence = hifi::qml::OffscreenSurface::TextureAndFence;

void MacQml::update() {
    auto rootItem =_surface->getRootItem();
    float now = sinf(secTimestampNow());
    rootItem->setProperty("level", fabs(now));
    rootItem->setProperty("muted", now > 0.0f);
    rootItem->setProperty("statsValue", rand());

    // Fetch any new textures
    TextureAndFence newTextureAndFence;
    if (_surface->fetchTexture(newTextureAndFence)) {
        if (_texture != 0) {
            auto readFence = _glf.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            glFlush();
            _discardLamdba(_texture, readFence);
        }
        _texture = newTextureAndFence.first;
        _glf.glWaitSync((GLsync)newTextureAndFence.second, 0, GL_TIMEOUT_IGNORED);
    }
}

void MacQml::init() {
    Parent::init();
    _glf.glGenFramebuffers(1, &_fbo);
    _surface.reset(new hifi::qml::OffscreenSurface());
    //QUrl url =getTestResource("qml/main.qml");
    QUrl url = getTestResource("qml/MacQml.qml");
    hifi::qml::QmlContextObjectCallback callback =[](QQmlContext* context, QQuickItem* item) {
    };
    _surface->load(url, callback);
    _surface->resize(_window->size());
    _surface->resume();
    _window->installEventFilter(_surface.get());
}

void MacQml::draw() {
    auto size = _window->geometry().size();
    if (_texture) {
        _glf.glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
        _glf.glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);
        _glf.glBlitFramebuffer(
                               // src coordinates
                               0, 0, size.width(), size.height(),
                               // dst coordinates
                               0, 0, size.width(), size.height(),
                               // blit mask and filter
                               GL_COLOR_BUFFER_BIT, GL_NEAREST);
        _glf.glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        _glf.glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
}
