//
//  Base3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Base3DOverlay_h
#define hifi_Base3DOverlay_h

#include <glm/glm.hpp>

#include "Overlay.h"

class Base3DOverlay : public Overlay {
    Q_OBJECT
    
public:
    Base3DOverlay();
    ~Base3DOverlay();

    // getters
    const glm::vec3& getPosition() const { return _position; }
    float getLineWidth() const { return _lineWidth; }

    // setters
    void setPosition(const glm::vec3& position) { _position = position; }
    void setLineWidth(float lineWidth) { _lineWidth = lineWidth; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    glm::vec3 _position;
    float _lineWidth;
};

 
#endif // hifi_Base3DOverlay_h
