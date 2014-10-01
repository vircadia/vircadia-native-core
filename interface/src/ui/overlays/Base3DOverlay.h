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
#include <glm/gtc/quaternion.hpp>
//#include <glm/gtx/extented_min_max.hpp>

#include "Overlay.h"

class Base3DOverlay : public Overlay {
    Q_OBJECT
    
public:
    Base3DOverlay();
    ~Base3DOverlay();

    // getters
    const glm::vec3& getPosition() const { return _position; }
    const glm::vec3& getCenter() const { return _position; } // TODO: consider implementing registration points in this class
    float getLineWidth() const { return _lineWidth; }
    bool getIsSolid() const { return _isSolid; }
    bool getIsDashedLine() const { return _isDashedLine; }
    bool getIsSolidLine() const { return !_isDashedLine; }
    const glm::quat& getRotation() const { return _rotation; }

    // setters
    void setPosition(const glm::vec3& position) { _position = position; }
    void setLineWidth(float lineWidth) { _lineWidth = lineWidth; }
    void setIsSolid(bool isSolid) { _isSolid = isSolid; }
    void setIsDashedLine(bool isDashedLine) { _isDashedLine = isDashedLine; }
    void setRotation(const glm::quat& value) { _rotation = value; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    void drawDashedLine(const glm::vec3& start, const glm::vec3& end);

    glm::vec3 _position;
    float _lineWidth;
    glm::quat _rotation;
    bool _isSolid;
    bool _isDashedLine;
};

 
#endif // hifi_Base3DOverlay_h
