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

    void setMaxFPS(uint8_t maxFPS);
    virtual void render(RenderArgs* args) override;
    virtual const render::ShapeKey getShapeKey() override;

    virtual void update(float deltatime) override;

    QObject* getEventHandler();
    void setProxyWindow(QWindow* proxyWindow);
    Q_INVOKABLE void hoverEnterOverlay(const PointerEvent& event);
    Q_INVOKABLE void hoverLeaveOverlay(const PointerEvent& event);
    Q_INVOKABLE void handlePointerEvent(const PointerEvent& event);
    void handlePointerEventAsTouch(const PointerEvent& event);
    void handlePointerEventAsMouse(const PointerEvent& event);

    // setters
    void setURL(const QString& url);
    void setScriptURL(const QString& script);

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    virtual Web3DOverlay* createClone() const override;

    enum InputMode {
        Touch,
        Mouse
    };

    void buildWebSurface();
    void destroyWebSurface();
    void onResizeWebSurface();

    Q_INVOKABLE unsigned int deviceIdByTouchPoint(qreal x, qreal y);

public slots:
    void emitScriptEvent(const QVariant& scriptMessage);

signals:
    void scriptEventReceived(const QVariant& message);
    void webEventReceived(const QVariant& message);
    void resizeWebSurface();
    void requestWebSurface();
    void releaseWebSurface();

protected:
    Transform evalRenderTransform() override;

private:
    void setupQmlSurface();
    void rebuildWebSurface();
    bool isWebContent() const;

    InputMode _inputMode { Touch };
    QSharedPointer<OffscreenQmlSurface> _webSurface;
    bool _cachedWebSurface{ false };
    gpu::TexturePointer _texture;
    QString _url;
    QString _scriptURL;
    float _dpi { 30.0f };
    int _geometryId { 0 };
    bool _showKeyboardFocusHighlight { true };

    QTouchDevice _touchDevice;

    uint8_t _desiredMaxFPS { 10 };
    uint8_t _currentMaxFPS { 0 };

    bool _mayNeedResize { false };
};

#endif // hifi_Web3DOverlay_h
