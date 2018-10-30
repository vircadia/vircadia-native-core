#include "StressWeb.h"

#include <QtQuick/QQuickItem>

#include <SharedUtil.h>

using TextureAndFence = hifi::qml::OffscreenSurface::TextureAndFence;

static const int DEFAULT_MAX_FPS = 10;
static const QString CONTROL_URL{ "/qml/controls/WebEntityView.qml" };
static const char* URL_PROPERTY{ "url" };

QString StressWeb::getSourceUrl(bool video) {
    static const std::vector<QString> SOURCE_URLS{
        "https://www.reddit.com/wiki/random",
        "https://en.wikipedia.org/wiki/Wikipedia:Random",
        "https://slashdot.org/",
    };

    static const std::vector<QString> VIDEO_SOURCE_URLS{
        "https://www.youtube.com/watch?v=gDXwhHm4GhM",
        "https://www.youtube.com/watch?v=Ch_hoYPPeGc",
    };

    const auto& sourceUrls = video ? VIDEO_SOURCE_URLS : SOURCE_URLS;
    auto index = rand() % sourceUrls.size();
    return sourceUrls[index];
}



void StressWeb::buildSurface(QmlInfo& qmlInfo, bool video) {
    ++_surfaceCount;
    auto lifetimeSecs = (uint32_t)(5.0f + (randFloat() * 10.0f));
    auto lifetimeUsecs = (USECS_PER_SECOND * lifetimeSecs);
    qmlInfo.lifetime = lifetimeUsecs + usecTimestampNow();
    qmlInfo.texture = 0;
    qmlInfo.surface.reset(new hifi::qml::OffscreenSurface());
    qmlInfo.surface->load(getTestResource(CONTROL_URL), [video](QQmlContext* context, QQuickItem* item) {
        item->setProperty(URL_PROPERTY, getSourceUrl(video));
    });
    qmlInfo.surface->setMaxFps(DEFAULT_MAX_FPS);
    qmlInfo.surface->resize(_qmlSize);
    qmlInfo.surface->resume();
}

void StressWeb::destroySurface(QmlInfo& qmlInfo) {
    auto& surface = qmlInfo.surface;
    auto& currentTexture = qmlInfo.texture;
    if (currentTexture) {
        auto readFence = _glf.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glFlush();
        _discardLamdba(currentTexture, readFence);
    }
    auto webView = surface->getRootItem();
    if (webView) {
        // stop loading
        QMetaObject::invokeMethod(webView, "stop");
        webView->setProperty(URL_PROPERTY, "about:blank");
    }
    surface->pause();
    surface.reset();
}

void StressWeb::update() {
    auto now = usecTimestampNow();
    // Fetch any new textures
    for (size_t x = 0; x < DIVISIONS_X; ++x) {
        for (size_t y = 0; y < DIVISIONS_Y; ++y) {
            auto& qmlInfo = _surfaces[x][y];
            if (!qmlInfo.surface) {
                if (now < _createStopTime && randFloat() > 0.99f) {
                    buildSurface(qmlInfo, x == 0 && y == 0);
                } else {
                    continue;
                }
            }

            if (now > qmlInfo.lifetime) {
                destroySurface(qmlInfo);
                continue;
            }

            auto& surface = qmlInfo.surface;
            auto& currentTexture = qmlInfo.texture;

            TextureAndFence newTextureAndFence;
            if (surface->fetchTexture(newTextureAndFence)) {
                if (currentTexture != 0) {
                    auto readFence = _glf.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                    glFlush();
                    _discardLamdba(currentTexture, readFence);
                }
                currentTexture = newTextureAndFence.first;
                _glf.glWaitSync((GLsync)newTextureAndFence.second, 0, GL_TIMEOUT_IGNORED);
            }
        }
    }
}

void StressWeb::init() {
    Parent::init();
    _createStopTime = usecTimestampNow() + (3000u * USECS_PER_SECOND);
    _glf.glGenFramebuffers(1, &_fbo);
}

void StressWeb::draw() {
    auto size = _window->geometry().size();
    auto incrementX = size.width() / DIVISIONS_X;
    auto incrementY = size.height() / DIVISIONS_Y;

    for (uint32_t x = 0; x < DIVISIONS_X; ++x) {
        for (uint32_t y = 0; y < DIVISIONS_Y; ++y) {
            auto& qmlInfo = _surfaces[x][y];
            if (!qmlInfo.surface || !qmlInfo.texture) {
                continue;
            }
            _glf.glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
            _glf.glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, qmlInfo.texture, 0);
            _glf.glBlitFramebuffer(
                // src coordinates
                0, 0, _qmlSize.width() - 1, _qmlSize.height() - 1,
                // dst coordinates
                incrementX * x, incrementY * y, incrementX * (x + 1), incrementY * (y + 1),
                // blit mask and filter
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }
    _glf.glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    _glf.glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}
