//
//  Created by Bradley Austin Davis on 2016/01/27
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_QmlOverlay_h
#define hifi_QmlOverlay_h

#include <QString>
#include <memory>

#include <SharedUtil.h>
#include "Overlay2D.h"

class QQuickItem;

class QmlOverlay : public Overlay2D {
    Q_OBJECT

public:
    QmlOverlay(const QUrl& url);
    QmlOverlay(const QUrl& url, const QmlOverlay* textOverlay);
    ~QmlOverlay();

    // Cannot fetch properties from QML based overlays due to race conditions
    bool supportsGetProperty() const override { return false; }

    void setProperties(const QVariantMap& properties) override;
    void render(RenderArgs* args) override;

private:
    Q_INVOKABLE void buildQmlElement(const QUrl& url);

protected:
    std::shared_ptr<QQuickItem> _qmlElement;
};

#endif // hifi_QmlOverlay_h
