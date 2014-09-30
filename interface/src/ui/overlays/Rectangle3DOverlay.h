//
//  Rectangle3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Rectangle3DOverlay_h
#define hifi_Rectangle3DOverlay_h

#include "Planar3DOverlay.h"

class Rectangle3DOverlay : public Planar3DOverlay {
    Q_OBJECT
    
public:
    Rectangle3DOverlay();
    ~Rectangle3DOverlay();
    virtual void render();
    virtual void setProperties(const QScriptValue& properties);
};

 
#endif // hifi_Rectangle3DOverlay_h
