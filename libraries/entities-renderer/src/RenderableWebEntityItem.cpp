//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableWebEntityItem.h"

#include <gpu/GPUConfig.h>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QOpenGLContext>

#include <glm/gtx/quaternion.hpp>

#include <DeferredLightingEffect.h>
#include <GeometryCache.h>
#include <PerfStat.h>
#include <OffscreenQmlSurface.h>
#include <AbstractViewStateInterface.h>
#include <GLMHelpers.h>
#include <PathUtils.h>
#include <TextureCache.h>
#include <gpu/GLBackend.h>

#include "EntityTreeRenderer.h"

const float DPI = 30.47f;
const float METERS_TO_INCHES = 39.3701f;

EntityItemPointer RenderableWebEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderableWebEntityItem>(entityID, properties);
}

RenderableWebEntityItem::RenderableWebEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    WebEntityItem(entityItemID, properties) {
    qDebug() << "Created web entity " << getID();
}

RenderableWebEntityItem::~RenderableWebEntityItem() {
    if (_webSurface) {
        _webSurface->pause();
        _webSurface->disconnect(_connection);
        // After the disconnect, ensure that we have the latest texture by acquiring the 
        // lock used when updating the _texture value
        _textureLock.lock();
        _textureLock.unlock();
        // The lifetime of the QML surface MUST be managed by the main thread
        // Additionally, we MUST use local variables copied by value, rather than
        // member variables, since they would implicitly refer to a this that 
        // is no longer valid
        auto webSurface = _webSurface;
        auto texture = _texture;
        AbstractViewStateInterface::instance()->postLambdaEvent([webSurface, texture] {
            if (texture) {
                webSurface->releaseTexture(texture);
            }
            webSurface->deleteLater();
        });
    }
    qDebug() << "Destroyed web entity " << getID();
}

void RenderableWebEntityItem::render(RenderArgs* args) {
    QOpenGLContext * currentContext = QOpenGLContext::currentContext();
    QSurface * currentSurface = currentContext->surface();
    if (!_webSurface) {
        _webSurface = new OffscreenQmlSurface();
        _webSurface->create(currentContext);
        _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
        _webSurface->load("WebEntity.qml");
        _webSurface->resume();
        _webSurface->getRootItem()->setProperty("url", _sourceUrl);
        _connection = QObject::connect(_webSurface, &OffscreenQmlSurface::textureUpdated, [&](GLuint textureId) {
            _webSurface->lockTexture(textureId);
            assert(!glGetError());
            // TODO change to atomic<GLuint>?
            withLock(_textureLock, [&] {
                std::swap(_texture, textureId);
            });
            if (textureId) {
                _webSurface->releaseTexture(textureId);
            }
            if (_texture) {
                _webSurface->makeCurrent();
                glBindTexture(GL_TEXTURE_2D, _texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);
                _webSurface->doneCurrent();
            }
        });

        auto forwardMouseEvent = [=](const RayToEntityIntersectionResult& intersection, const QMouseEvent* event, unsigned int deviceId) {
            // Ignore mouse interaction if we're locked
            if (this->getLocked()) {
                return;
            }

            if (event->button() == Qt::MouseButton::RightButton) {
                if (event->type() == QEvent::MouseButtonPress) {
                    const QMouseEvent* mouseEvent = static_cast<const QMouseEvent*>(event);
                    _lastPress = toGlm(mouseEvent->pos());
                }
            }

            if (intersection.entityID == getID()) {
                if (event->button() == Qt::MouseButton::RightButton) {
                    if (event->type() == QEvent::MouseButtonRelease) {
                        const QMouseEvent* mouseEvent = static_cast<const QMouseEvent*>(event);
                        ivec2 dist = glm::abs(toGlm(mouseEvent->pos()) - _lastPress);
                        if (!glm::any(glm::greaterThan(dist, ivec2(1)))) {
                            AbstractViewStateInterface::instance()->postLambdaEvent([this] {
                                QMetaObject::invokeMethod(_webSurface->getRootItem(), "goBack");
                            });
                        }
                        _lastPress = ivec2(INT_MIN);
                    }
                    return;
                }

                // FIXME doesn't work... double click events not received
                if (event->type() == QEvent::MouseButtonDblClick) {
                    AbstractViewStateInterface::instance()->postLambdaEvent([this] {
                        _webSurface->getRootItem()->setProperty("url", _sourceUrl);
                    });
                }

                if (event->button() == Qt::MouseButton::MiddleButton) {
                    if (event->type() == QEvent::MouseButtonRelease) {
                        AbstractViewStateInterface::instance()->postLambdaEvent([this] {
                            _webSurface->getRootItem()->setProperty("url", _sourceUrl);
                        });
                    }
                    return;
                }

                // Map the intersection point to an actual offscreen pixel
                glm::vec3 point = intersection.intersection;
                point -= getPosition();
                point = glm::inverse(getRotation()) * point;
                point /= getDimensions();
                point += 0.5f;
                point.y = 1.0f - point.y;
                point *= getDimensions() * METERS_TO_INCHES * DPI;
                // Forward the mouse event.  
                QMouseEvent mappedEvent(event->type(),
                    QPoint((int)point.x, (int)point.y),
                    event->screenPos(), event->button(),
                    event->buttons(), event->modifiers());
                QCoreApplication::sendEvent(_webSurface->getWindow(), &mappedEvent);
            }
        };

        EntityTreeRenderer* renderer = static_cast<EntityTreeRenderer*>(args->_renderer);
        QObject::connect(renderer, &EntityTreeRenderer::mousePressOnEntity, forwardMouseEvent);
        QObject::connect(renderer, &EntityTreeRenderer::mouseReleaseOnEntity, forwardMouseEvent);
        QObject::connect(renderer, &EntityTreeRenderer::mouseMoveOnEntity, forwardMouseEvent);
    }

    glm::vec2 dims = glm::vec2(getDimensions());
    dims *= METERS_TO_INCHES * DPI;
    // The offscreen surface is idempotent for resizes (bails early
    // if it's a no-op), so it's safe to just call resize every frame 
    // without worrying about excessive overhead.
    _webSurface->resize(QSize(dims.x, dims.y));
    currentContext->makeCurrent(currentSurface);

    PerformanceTimer perfTimer("RenderableWebEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Web);
    static const glm::vec2 texMin(0.0f), texMax(1.0f), topLeft(-0.5f), bottomRight(0.5f);

    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(getTransformToCenter());
    bool textured = false, culled = false, emissive = false;
    if (_texture) {
        batch._glActiveTexture(GL_TEXTURE0);
        batch._glBindTexture(GL_TEXTURE_2D, _texture);
        textured = emissive = true;
    }
    
    DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch, textured, culled, emissive);
    DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, texMin, texMax, glm::vec4(1.0f));
}

void RenderableWebEntityItem::setSourceUrl(const QString& value) {
    if (_sourceUrl != value) {
        qDebug() << "Setting web entity source URL to " << value;
        _sourceUrl = value;
        if (_webSurface) {
            AbstractViewStateInterface::instance()->postLambdaEvent([this] {
                _webSurface->getRootItem()->setProperty("url", _sourceUrl);
            });
        }
    }
}
