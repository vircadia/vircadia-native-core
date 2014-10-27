//
//  Planar3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Planar3DOverlay_h
#define hifi_Planar3DOverlay_h

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <glm/glm.hpp>

#include <QGLWidget>
#include <QScriptValue>

#include "Base3DOverlay.h"

class Planar3DOverlay : public Base3DOverlay {
    Q_OBJECT
    
public:
    Planar3DOverlay();
    ~Planar3DOverlay();

    // getters
    const glm::vec2& getDimensions() const { return _dimensions; }

    // setters
    void setSize(float size) { _dimensions = glm::vec2(size, size); }
    void setDimensions(const glm::vec2& value) { _dimensions = value; }

    virtual void setProperties(const QScriptValue& properties);

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face) const;

protected:
    glm::vec2 _dimensions;
};

 
#endif // hifi_Planar3DOverlay_h
