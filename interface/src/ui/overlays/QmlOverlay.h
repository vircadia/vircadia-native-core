//
//  Created by Bradley Austin Davis on 2016/01/27
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_QmlOverlay_h
#define hifi_QmlOverlay_h

#include "Overlay2D.h"

class QQuickItem;

class QmlOverlay : public Overlay2D {
    Q_OBJECT

public:
    QmlOverlay(const QUrl& url);
    QmlOverlay(const QUrl& url, const QmlOverlay* overlay);
    ~QmlOverlay();

    void setProperties(const QVariantMap& properties) override;
    void render(RenderArgs* args) override {}

private:
    Q_INVOKABLE void qmlElementDestroyed();
    Q_INVOKABLE void buildQmlElement(const QUrl& url);

protected:
    QQuickItem* _qmlElement { nullptr };
};

#endif // hifi_QmlOverlay_h
