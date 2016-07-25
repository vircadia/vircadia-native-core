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

QString const Web3DOverlay::TYPE = "web3d";

Web3DOverlay::Web3DOverlay() : _dpi(DPI) { }

Web3DOverlay::Web3DOverlay(const Web3DOverlay* Web3DOverlay) :
    Billboard3DOverlay(Web3DOverlay),
    _url(Web3DOverlay->_url),
    _dpi(Web3DOverlay->_dpi),
    _resolution(Web3DOverlay->_resolution)
{
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
}

void Web3DOverlay::update(float deltatime) {
    applyTransformTo(_transform);
}

void Web3DOverlay::render(RenderArgs* args) {
    if (!_visible || !getParentVisible()) {
        return;
    }

    QOpenGLContext * currentContext = QOpenGLContext::currentContext();
    QSurface * currentSurface = currentContext->surface();
    if (!_webSurface) {
        _webSurface = new OffscreenQmlSurface();
        _webSurface->create(currentContext);
        _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
        _webSurface->load("WebEntity.qml");
        _webSurface->resume();
        _webSurface->getRootItem()->setProperty("url", _url);
        _webSurface->resize(QSize(_resolution.x, _resolution.y));
        _connection = QObject::connect(_webSurface, &OffscreenQmlSurface::textureUpdated, [&](GLuint textureId) {
            _texture = textureId;
        });
        currentContext->makeCurrent(currentSurface);
    }

    vec2 size = _resolution / _dpi * INCHES_TO_METERS;
    vec2 halfSize = size / 2.0f;
    vec4 color(toGlm(getColor()), getAlpha());

    applyTransformTo(_transform, true);
    Transform transform = _transform;
    if (glm::length2(getDimensions()) != 1.0f) {
        transform.postScale(vec3(getDimensions(), 1.0f));
    }

    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    if (_texture) {
        batch._glActiveBindTexture(GL_TEXTURE0, GL_TEXTURE_2D, _texture);
    } else {
        batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }

    batch.setModelTransform(transform);
    auto geometryCache = DependencyManager::get<GeometryCache>();
    geometryCache->bindSimpleSRGBTexturedUnlitNoDstAlphaProgram(batch);
    geometryCache->renderQuad(batch, halfSize * -1.0f, halfSize, vec2(0), vec2(1), color);
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
    applyTransformTo(_transform, true);
    vec2 size = _resolution / _dpi * INCHES_TO_METERS * vec2(getDimensions());
    // Produce the dimensions of the overlay based on the image's aspect ratio and the overlay's scale.
    return findRayRectangleIntersection(origin, direction, getRotation(), getPosition(), size, distance);
}

Web3DOverlay* Web3DOverlay::createClone() const {
    return new Web3DOverlay(this);
}
