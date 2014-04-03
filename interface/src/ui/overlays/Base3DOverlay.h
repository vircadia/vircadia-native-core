//
//  Base3DOverlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Base3DOverlay__
#define __interface__Base3DOverlay__

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

 
#endif /* defined(__interface__Base3DOverlay__) */
