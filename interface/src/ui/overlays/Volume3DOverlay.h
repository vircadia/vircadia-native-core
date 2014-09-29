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
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/extented_min_max.hpp>

#include <QGLWidget>
#include <QScriptValue>

#include "Base3DOverlay.h"

class Volume3DOverlay : public Base3DOverlay {
    Q_OBJECT
    
public:
    Volume3DOverlay();
    ~Volume3DOverlay();

    // getters
    bool getIsSolid() const { return _isSolid; }
    const glm::vec3& getPosition() const { return _position; }
    const glm::vec3& getCenter() const { return _position; } // TODO: registration point!!
    const glm::vec3& getDimensions() const { return _dimensions; }
    const glm::quat& getRotation() const { return _rotation; }

    // setters
    void setSize(float size) { _dimensions = glm::vec3(size, size, size); }
    void setIsSolid(bool isSolid) { _isSolid = isSolid; }
    void setDimensions(const glm::vec3& value) { _dimensions = value; }
    void setRotation(const glm::quat& value) { _rotation = value; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    glm::vec3 _dimensions;
    glm::quat _rotation;
    bool _isSolid;
};

 
#endif // hifi_Volume3DOverlay_h
