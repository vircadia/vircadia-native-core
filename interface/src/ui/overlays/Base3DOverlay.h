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

#include <BoxBase.h>

#include "Overlay.h"

class Base3DOverlay : public Overlay {
    Q_OBJECT
    
public:
    Base3DOverlay();
    Base3DOverlay(const Base3DOverlay* base3DOverlay);
    ~Base3DOverlay();

    // getters
    virtual bool is3D() const { return true; }
    const glm::vec3& getPosition() const { return _position; }
    const glm::vec3& getCenter() const { return _position; } // TODO: consider implementing registration points in this class
    float getLineWidth() const { return _lineWidth; }
    bool getIsSolid() const { return _isSolid; }
    bool getIsDashedLine() const { return _isDashedLine; }
    bool getIsSolidLine() const { return !_isDashedLine; }
    const glm::quat& getRotation() const { return _rotation; }
    bool getIgnoreRayIntersection() const { return _ignoreRayIntersection; }
    bool getDrawInFront() const { return _drawInFront; }

    // setters
    void setPosition(const glm::vec3& position) { _position = position; }
    void setLineWidth(float lineWidth) { _lineWidth = lineWidth; }
    void setIsSolid(bool isSolid) { _isSolid = isSolid; }
    void setIsDashedLine(bool isDashedLine) { _isDashedLine = isDashedLine; }
    void setRotation(const glm::quat& value) { _rotation = value; }
    void setIgnoreRayIntersection(bool value) { _ignoreRayIntersection = value; }
    void setDrawInFront(bool value);

    virtual void setProperties(const QScriptValue& properties);
    virtual QScriptValue getProperty(const QString& property);

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face) const;

    virtual bool findRayIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& direction, 
                                                    float& distance, BoxFace& face, QString& extraInfo) const {
            return findRayIntersection(origin, direction, distance, face);
    }

signals:
    void drawInFrontUpdated(bool newValue);

protected:
    void drawDashedLine(const glm::vec3& start, const glm::vec3& end);

    glm::vec3 _position;
    float _lineWidth;
    glm::quat _rotation;
    bool _isSolid;
    bool _isDashedLine;
    bool _ignoreRayIntersection;
    bool _drawInFront;
};
 
#endif // hifi_Base3DOverlay_h
