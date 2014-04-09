//
//  Line3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __interface__Line3DOverlay__
#define __interface__Line3DOverlay__

#include "Base3DOverlay.h"

class Line3DOverlay : public Base3DOverlay {
    Q_OBJECT
    
public:
    Line3DOverlay();
    ~Line3DOverlay();
    virtual void render();

    // getters
    const glm::vec3& getEnd() const { return _end; }

    // setters
    void setEnd(const glm::vec3& end) { _end = end; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    glm::vec3 _end;
};

 
#endif /* defined(__interface__Line3DOverlay__) */
