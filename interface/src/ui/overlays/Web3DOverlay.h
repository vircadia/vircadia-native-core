//
//  Created by Bradley Austin Davis on 2015/08/31
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Web3DOverlay_h
#define hifi_Web3DOverlay_h

#include "Billboard3DOverlay.h"

#include <QTouchEvent>

#include <PointerEvent.h>

class OffscreenQmlSurface;

class Web3DOverlay : public Billboard3DOverlay {
    Q_OBJECT

public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    Web3DOverlay();
    Web3DOverlay(const Web3DOverlay* Web3DOverlay);
    virtual ~Web3DOverlay();

    virtual void render(RenderArgs* args) override;
    virtual const render::ShapeKey getShapeKey() override;

    virtual void update(float deltatime) override;

    QObject* getEventHandler();
    void setProxyWindow(QWindow* proxyWindow);
    void handlePointerEvent(const PointerEvent& event);

    // setters
    void setURL(const QString& url);
    void setScriptURL(const QString& script);

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    glm::vec2 getSize();

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
        BoxFace& face, glm::vec3& surfaceNormal) override;

    virtual Web3DOverlay* createClone() const override;

public slots:
    void emitScriptEvent(const QVariant& scriptMessage);

signals:
    void scriptEventReceived(const QVariant& message);
    void webEventReceived(const QVariant& message);

private:
    QSharedPointer<OffscreenQmlSurface> _webSurface;
    QMetaObject::Connection _connection;
    gpu::TexturePointer _texture;
    QString _url;
    QString _scriptURL;
    float _dpi;
    vec2 _resolution{ 640, 480 };
    int _geometryId { 0 };

    bool _pressed{ false };
    QTouchDevice _touchDevice;

    QMetaObject::Connection _mousePressConnection;
    QMetaObject::Connection _mouseReleaseConnection;
    QMetaObject::Connection _mouseMoveConnection;
    QMetaObject::Connection _hoverLeaveConnection;

    QMetaObject::Connection _emitScriptEventConnection;
    QMetaObject::Connection _webEventReceivedConnection;
};

#endif // hifi_Web3DOverlay_h
