//
//  Volume3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Volume3DOverlay_h
#define hifi_Volume3DOverlay_h

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <glm/glm.hpp>

#include <QGLWidget>
#include <QScriptValue>

#include "Base3DOverlay.h"

class Volume3DOverlay : public Base3DOverlay {
    Q_OBJECT
    
public:
    Volume3DOverlay();
    ~Volume3DOverlay();

    // getters
    const glm::vec3& getCenter() const { return _position; } // TODO: consider adding registration point!!
    glm::vec3 getCorner() const { return _position - (_dimensions * 0.5f); } // TODO: consider adding registration point!!
    const glm::vec3& getDimensions() const { return _dimensions; }

    // setters
    void setSize(float size) { _dimensions = glm::vec3(size, size, size); }
    void setDimensions(const glm::vec3& value) { _dimensions = value; }

    virtual void setProperties(const QScriptValue& properties);
    virtual QScriptValue getProperty(const QString& property);

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face) const;

protected:
    glm::vec3 _dimensions;
};

 
#endif // hifi_Volume3DOverlay_h
