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

class OffscreenQmlSurface;

class Web3DOverlay : public Billboard3DOverlay {
    Q_OBJECT

public:
    static QString const TYPE;
    virtual QString getType() const { return TYPE; }

    Web3DOverlay();
    Web3DOverlay(const Web3DOverlay* Web3DOverlay);
    virtual ~Web3DOverlay();

    virtual void render(RenderArgs* args);

    virtual void update(float deltatime);

    // setters
    void setURL(const QString& url);

    virtual void setProperties(const QScriptValue& properties);
    virtual QScriptValue getProperty(const QString& property);

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
                                        BoxFace& face, glm::vec3& surfaceNormal);

    virtual Web3DOverlay* createClone() const;

private:
    OffscreenQmlSurface* _webSurface{ nullptr };
    QMetaObject::Connection _connection;
    uint32_t _texture{ 0 };
    QString _url;
    float _dpi;
    vec2 _resolution{ 640, 480 };
};

#endif // hifi_Web3DOverlay_h
