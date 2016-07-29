//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableWebEntityItem.h"

#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QOpenGLContext>

#include <glm/gtx/quaternion.hpp>

#include <GeometryCache.h>
#include <PerfStat.h>
#include <gl/OffscreenQmlSurface.h>
#include <AbstractViewStateInterface.h>
#include <GLMHelpers.h>
#include <PathUtils.h>
#include <TextureCache.h>
#include <gpu/Context.h>

#include "EntityTreeRenderer.h"

const float DPI = 30.47f;
const float METERS_TO_INCHES = 39.3701f;
static uint32_t _currentWebCount { 0 };
// Don't allow more than 100 concurrent web views
static const uint32_t MAX_CONCURRENT_WEB_VIEWS = 100;
// If a web-view hasn't been rendered for 30 seconds, de-allocate the framebuffer
static uint64_t MAX_NO_RENDER_INTERVAL = 30 * USECS_PER_SECOND;

EntityItemPointer RenderableWebEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableWebEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

RenderableWebEntityItem::RenderableWebEntityItem(const EntityItemID& entityItemID) :
    WebEntityItem(entityItemID) {
    qDebug() << "Created web entity " << getID();
}

RenderableWebEntityItem::~RenderableWebEntityItem() {
    destroyWebSurface();
    qDebug() << "Destroyed web entity " << getID();
}

bool RenderableWebEntityItem::buildWebSurface(EntityTreeRenderer* renderer) {
    if (_currentWebCount >= MAX_CONCURRENT_WEB_VIEWS) {
        qWarning() << "Too many concurrent web views to create new view";
        return false;
    }
    qDebug() << "Building web surface";

    ++_currentWebCount;
    // Save the original GL context, because creating a QML surface will create a new context
    QOpenGLContext * currentContext = QOpenGLContext::currentContext();
    QSurface * currentSurface = currentContext->surface();
    _webSurface = new OffscreenQmlSurface();
    _webSurface->create(currentContext);
    _webSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
    _webSurface->load("WebEntity.qml");
    _webSurface->resume();
    _webSurface->getRootItem()->setProperty("url", _sourceUrl);
    _connection = QObject::connect(_webSurface, &OffscreenQmlSurface::textureUpdated, [&](GLuint textureId) {
        _texture = textureId;
    });
    // Restore the original GL context
    currentContext->makeCurrent(currentSurface);

    auto forwardMouseEvent = [=](const RayToEntityIntersectionResult& intersection, const QMouseEvent* event) {
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
            glm::vec3 dimensions = getDimensions();
            point -= getPosition();
            point = glm::inverse(getRotation()) * point;
            point /= dimensions;
            point += 0.5f;
            point.y = 1.0f - point.y;
            point *= dimensions * (METERS_TO_INCHES * DPI);

            if (event->button() == Qt::MouseButton::LeftButton) {
                if (event->type() == QEvent::MouseButtonPress) {
                    this->_pressed = true;
                    this->_lastMove = ivec2((int)point.x, (int)point.y);
                } else if (event->type() == QEvent::MouseButtonRelease) {
                    this->_pressed = false;
                }
            }
            if (event->type() == QEvent::MouseMove) {
                this->_lastMove = ivec2((int)point.x, (int)point.y);
            }

            // Forward the mouse event.  
            QMouseEvent mappedEvent(event->type(),
                QPoint((int)point.x, (int)point.y),
                event->screenPos(), event->button(),
                event->buttons(), event->modifiers());
            QCoreApplication::sendEvent(_webSurface->getWindow(), &mappedEvent);
        }
    };
    _mousePressConnection = QObject::connect(renderer, &EntityTreeRenderer::mousePressOnEntity, forwardMouseEvent);
    _mouseReleaseConnection = QObject::connect(renderer, &EntityTreeRenderer::mouseReleaseOnEntity, forwardMouseEvent);
    _mouseMoveConnection = QObject::connect(renderer, &EntityTreeRenderer::mouseMoveOnEntity, forwardMouseEvent);
    _hoverLeaveConnection = QObject::connect(renderer, &EntityTreeRenderer::hoverLeaveEntity, [=](const EntityItemID& entityItemID, const MouseEvent& event) {
        if (this->_pressed && this->getID() == entityItemID) {
            // If the user mouses off the entity while the button is down, simulate a mouse release
            QMouseEvent mappedEvent(QEvent::MouseButtonRelease,
                QPoint(_lastMove.x, _lastMove.y),
                Qt::MouseButton::LeftButton,
                Qt::MouseButtons(), Qt::KeyboardModifiers());
            QCoreApplication::sendEvent(_webSurface->getWindow(), &mappedEvent);
        }
    });
    return true;
}

void RenderableWebEntityItem::render(RenderArgs* args) {
    #ifdef WANT_EXTRA_DEBUGGING
    {
        gpu::Batch& batch = *args->_batch;
        batch.setModelTransform(getTransformToCenter()); // we want to include the scale as well
        glm::vec4 cubeColor{ 1.0f, 0.0f, 0.0f, 1.0f};
        DependencyManager::get<GeometryCache>()->renderWireCube(batch, 1.0f, cubeColor);
    }
    #endif

    if (!_webSurface) {
        #if defined(Q_OS_LINUX)
        // these don't seem to work on Linux
        return;
        #else
        if (!buildWebSurface(static_cast<EntityTreeRenderer*>(args->_renderer))) {
            return;
        }
        #endif
    }

    _lastRenderTime = usecTimestampNow();
    glm::vec2 dims = glm::vec2(getDimensions());
    dims *= METERS_TO_INCHES * DPI;
    // The offscreen surface is idempotent for resizes (bails early
    // if it's a no-op), so it's safe to just call resize every frame 
    // without worrying about excessive overhead.
    _webSurface->resize(QSize(dims.x, dims.y));

    PerformanceTimer perfTimer("RenderableWebEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Web);
    static const glm::vec2 texMin(0.0f), texMax(1.0f), topLeft(-0.5f), bottomRight(0.5f);

    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    bool success;
    batch.setModelTransform(getTransformToCenter(success));
    if (!success) {
        return;
    }
    if (_texture) {
        batch._glActiveBindTexture(GL_TEXTURE0, GL_TEXTURE_2D, _texture);
    }

    DependencyManager::get<GeometryCache>()->bindSimpleSRGBTexturedUnlitNoTexAlphaProgram(batch);
    DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, texMin, texMax, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
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

void RenderableWebEntityItem::setProxyWindow(QWindow* proxyWindow) {
    if (_webSurface) {
        _webSurface->setProxyWindow(proxyWindow);
    }
}

QObject* RenderableWebEntityItem::getEventHandler() {
    if (!_webSurface) {
        return nullptr;
    }
    return _webSurface->getEventHandler();
}

void RenderableWebEntityItem::destroyWebSurface() {
    if (_webSurface) {
        --_currentWebCount;
        _webSurface->pause();
        _webSurface->disconnect(_connection);
        QObject::disconnect(_mousePressConnection);
        _mousePressConnection = QMetaObject::Connection();
        QObject::disconnect(_mouseReleaseConnection);
        _mouseReleaseConnection = QMetaObject::Connection();
        QObject::disconnect(_mouseMoveConnection);
        _mouseMoveConnection = QMetaObject::Connection();
        QObject::disconnect(_hoverLeaveConnection);
        _hoverLeaveConnection = QMetaObject::Connection();

        // The lifetime of the QML surface MUST be managed by the main thread
        // Additionally, we MUST use local variables copied by value, rather than
        // member variables, since they would implicitly refer to a this that 
        // is no longer valid
        auto webSurface = _webSurface;
        AbstractViewStateInterface::instance()->postLambdaEvent([webSurface] {
            webSurface->deleteLater();
        });
        _webSurface = nullptr;
    }
}


void RenderableWebEntityItem::update(const quint64& now) {
    auto interval = now - _lastRenderTime;
    if (interval > MAX_NO_RENDER_INTERVAL) {
        destroyWebSurface();
    }
}
