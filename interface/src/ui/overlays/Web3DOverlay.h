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
    using Parent = Billboard3DOverlay;

public:

    static const QString QML;
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    Web3DOverlay();
    Web3DOverlay(const Web3DOverlay* Web3DOverlay);
    virtual ~Web3DOverlay();

    QString pickURL();
    void loadSourceURL();
    void setMaxFPS(uint8_t maxFPS);
    virtual void render(RenderArgs* args) override;
    virtual const render::ShapeKey getShapeKey() override;

    virtual void update(float deltatime) override;

    QObject* getEventHandler();
    void setProxyWindow(QWindow* proxyWindow);
    void handlePointerEvent(const PointerEvent& event);
    void handlePointerEventAsTouch(const PointerEvent& event);
    void handlePointerEventAsMouse(const PointerEvent& event);

    // setters
    void setURL(const QString& url);
    void setScriptURL(const QString& script);

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    glm::vec2 getSize() const override;

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance,
        BoxFace& face, glm::vec3& surfaceNormal) override;

    virtual Web3DOverlay* createClone() const override;

    enum InputMode {
        Touch,
        Mouse
    };

    void buildWebSurface();
    void destroyWebSurface();
    void onResizeWebSurface();

public slots:
    void emitScriptEvent(const QVariant& scriptMessage);

signals:
    void scriptEventReceived(const QVariant& message);
    void webEventReceived(const QVariant& message);
    void resizeWebSurface();
    void requestWebSurface();
    void releaseWebSurface();

private:
    InputMode _inputMode { Touch };
    QSharedPointer<OffscreenQmlSurface> _webSurface;
    gpu::TexturePointer _texture;
    QString _url;
    QString _scriptURL;
    float _dpi;
    vec2 _resolution{ 640, 480 };
    int _geometryId { 0 };
    bool _showKeyboardFocusHighlight{ true };

    bool _pressed{ false };
    bool _touchBeginAccepted { false };
    std::map<uint32_t, QTouchEvent::TouchPoint> _activeTouchPoints;
    QTouchDevice _touchDevice;

    uint8_t _desiredMaxFPS { 10 };
    uint8_t _currentMaxFPS { 0 };

    bool _mayNeedResize { false };
};

#endif // hifi_Web3DOverlay_h
