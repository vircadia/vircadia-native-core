//
//  Web3DOverlay.cpp
//
//
//  Created by Clement on 7/1/14.
//  Modified and renamed by Zander Otavka on 8/4/15
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Web3DOverlay.h"

#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickItem>

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <GeometryUtil.h>
#include <TextureCache.h>
#include <PathUtils.h>
#include <gpu/Batch.h>
#include <RegisteredMetaTypes.h>
#include <AbstractViewStateInterface.h>

#include <gl/OffscreenQmlSurface.h>

static const float DPI = 30.47f;
static const float INCHES_TO_METERS = 1.0f / 39.3701f;
static float OPAQUE_ALPHA_THRESHOLD = 0.99f;

QString const Web3DOverlay::TYPE = "web3d";

Web3DOverlay::Web3DOverlay() : _dpi(DPI) { 
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Web3DOverlay::Web3DOverlay(const Web3DOverlay* Web3DOverlay) :
    Billboard3DOverlay(Web3DOverlay),
    _url(Web3DOverlay->_url),
    _dpi(Web3DOverlay->_dpi),
    _resolution(Web3DOverlay->_resolution)
{
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

Web3DOverlay::~Web3DOverlay() {
    if (_webSurface) {
        _webSurface->pause();
        _webSurface->disconnect(_connection);
        // The lifetime of the QML surface MUST be managed by the main thread
        // Additionally, we MUST use local variables copied by value, rather than
        // member variables, since they would implicitly refer to a this that
        // is no longer valid
        auto webSurface = _webSurface;
        AbstractViewStateInterface::instance()->postLambdaEvent([webSurface] {
            webSurface->deleteLater();
        });
    }
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

void Web3DOverlay::update(float deltatime) {
    if (usecTimestampNow() > _transformExpiry) {
        Transform transform = getTransform();
        applyTransformTo(transform);
        setTransform(transform);
    }
}

void Web3DOverlay::render(RenderArgs* args) {
    if (!_visible || !getParentVisible()) {
        return;
    }

    QOpenGLContext * currentContext = QOpenGLContext::currentContext();
    QSurface * currentSurface = currentContext->surface();
    if (!_webSurface) {
        auto deleter = [](OffscreenQmlSurface* webSurface) {
            AbstractViewStateInterface::instance()->postLambdaEvent([webSurface] {
                webSurface->deleteLater();
            });
        };
        _webSurface = QSharedPointer<OffscreenQmlSurface>(new OffscreenQmlSurface(), deleter);
        _webSurface->create(currentContext);
        _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/controls/"));
        _webSurface->load("WebView.qml");
        _webSurface->resume();
        _webSurface->getRootItem()->setProperty("url", _url);
        _webSurface->resize(QSize(_resolution.x, _resolution.y));
        currentContext->makeCurrent(currentSurface);
    }

    vec2 size = _resolution / _dpi * INCHES_TO_METERS;
    vec2 halfSize = size / 2.0f;
    vec4 color(toGlm(getColor()), getAlpha());

    Transform transform = getTransform();
    applyTransformTo(transform, true);
    setTransform(transform);
    if (glm::length2(getDimensions()) != 1.0f) {
        transform.postScale(vec3(getDimensions(), 1.0f));
    }

    if (!_texture) {
        auto webSurface = _webSurface;
        _texture = gpu::TexturePointer(gpu::Texture::createExternal2D([webSurface](uint32_t recycleTexture, void* recycleFence) {
            webSurface->releaseTexture({ recycleTexture, recycleFence });
        }));
        _texture->setSource(__FUNCTION__);
    }
    OffscreenQmlSurface::TextureAndFence newTextureAndFence;
    bool newTextureAvailable = _webSurface->fetchTexture(newTextureAndFence);
    if (newTextureAvailable) {
        _texture->setExternalTexture(newTextureAndFence.first, newTextureAndFence.second);
    }

    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setResourceTexture(0, _texture);
    batch.setModelTransform(transform);
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (color.a < OPAQUE_ALPHA_THRESHOLD) {
        geometryCache->bindTransparentWebBrowserProgram(batch);
    } else {
        geometryCache->bindOpaqueWebBrowserProgram(batch);
    }
    geometryCache->renderQuad(batch, halfSize * -1.0f, halfSize, vec2(0), vec2(1), color, _geometryId);
    batch.setResourceTexture(0, args->_whiteTexture); // restore default white color after me
}

const render::ShapeKey Web3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withoutCullFace().withDepthBias();
    if (getAlpha() != 1.0f) {
        builder.withTranslucent();
    }
    return builder.build();
}

void Web3DOverlay::setProperties(const QVariantMap& properties) {
    Billboard3DOverlay::setProperties(properties);

    auto urlValue = properties["url"];
    if (urlValue.isValid()) {
        QString newURL = urlValue.toString();
        if (newURL != _url) {
            setURL(newURL);
        }
    }

    auto resolution = properties["resolution"];
    if (resolution.isValid()) {
        bool valid;
        auto res = vec2FromVariant(resolution, valid);
        if (valid) {
            _resolution = res;
        }
    }


    auto dpi = properties["dpi"];
    if (dpi.isValid()) {
        _dpi = dpi.toFloat();
    }
}

QVariant Web3DOverlay::getProperty(const QString& property) {
    if (property == "url") {
        return _url;
    }
    if (property == "dpi") {
        return _dpi;
    }
    return Billboard3DOverlay::getProperty(property);
}

void Web3DOverlay::setURL(const QString& url) {
    _url = url;
    if (_webSurface) {
        AbstractViewStateInterface::instance()->postLambdaEvent([this, url] {
            _webSurface->getRootItem()->setProperty("url", url);
        });
    }

}

bool Web3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    // FIXME - face and surfaceNormal not being returned

    // Make sure position and rotation is updated.
    Transform transform;
    applyTransformTo(transform, true);
    setTransform(transform);

    vec2 size = _resolution / _dpi * INCHES_TO_METERS * vec2(getDimensions());
    // Produce the dimensions of the overlay based on the image's aspect ratio and the overlay's scale.
    return findRayRectangleIntersection(origin, direction, getRotation(), getPosition(), size, distance);
}

Web3DOverlay* Web3DOverlay::createClone() const {
    return new Web3DOverlay(this);
}
