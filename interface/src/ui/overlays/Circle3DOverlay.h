//
//  Circle3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Circle3DOverlay_h
#define hifi_Circle3DOverlay_h

#include "Planar3DOverlay.h"

class Circle3DOverlay : public Planar3DOverlay {
    Q_OBJECT
    
public:
    Circle3DOverlay();
    ~Circle3DOverlay();
    virtual void render();
    virtual void setProperties(const QScriptValue& properties);

    float getStartAt() const { return _startAt; }
    float getEndAt() const { return _endAt; }
    float getOuterRadius() const { return _outerRadius; }
    float getInnerRadius() const { return _innerRadius; }
    
    void setStartAt(float value) { _startAt = value; }
    void setEndAt(float value) { _endAt = value; }
    void setOuterRadius(float value) { _outerRadius = value; }
    void setInnerRadius(float value) { _innerRadius = value; }
    
protected:
    float _startAt;
    float _endAt;
    float _outerRadius;
    float _innerRadius;
};

 
#endif // hifi_Circle3DOverlay_h
