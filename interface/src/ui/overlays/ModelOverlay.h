//
//  ModelOverlay.h
//
//
//  Created by Clement on 6/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelOverlay_h
#define hifi_ModelOverlay_h

#include "Base3DOverlay.h"

#include "../../renderer/Model.h"

class ModelOverlay : public Base3DOverlay {
    Q_OBJECT
public:
    ModelOverlay();
    
    virtual void update(float deltatime);
    virtual void render(RenderArgs* args);
    virtual void setProperties(const QScriptValue& properties);
    virtual QScriptValue getProperty(const QString& property);
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face) const;
    virtual bool findRayIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& direction, 
                                                    float& distance, BoxFace& face, QString& extraInfo) const;

private:
    
    Model _model;
    QVariantMap _modelTextures;
    
    QUrl _url;
    glm::quat _rotation;
    float _scale;
    
    bool _updateModel;
};

#endif // hifi_ModelOverlay_h