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

#include <Model.h>

#include "Volume3DOverlay.h"

class ModelOverlay : public Volume3DOverlay {
    Q_OBJECT
public:
    static QString const TYPE;
    virtual QString getType() const { return TYPE; }

    ModelOverlay();
    ModelOverlay(const ModelOverlay* modelOverlay);

    virtual void update(float deltatime);
    virtual void render(RenderArgs* args);
    virtual void setProperties(const QScriptValue& properties);
    virtual QScriptValue getProperty(const QString& property);
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
                                        BoxFace& face, glm::vec3& surfaceNormal);
    virtual bool findRayIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& direction, 
                                        float& distance, BoxFace& face, glm::vec3& surfaceNormal, QString& extraInfo);

    virtual ModelOverlay* createClone() const;

    virtual bool addToScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);
    virtual void removeFromScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);

private:

    Model _model;
    QVariantMap _modelTextures;
    
    QUrl _url;
    bool _updateModel;
};

#endif // hifi_ModelOverlay_h