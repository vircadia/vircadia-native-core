//
//  Volume3DOverlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Volume3DOverlay__
#define __interface__Volume3DOverlay__

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QScriptValue>

#include "Base3DOverlay.h"

class Volume3DOverlay : public Base3DOverlay {
    Q_OBJECT
    
public:
    Volume3DOverlay();
    ~Volume3DOverlay();

    // getters
    float getSize() const { return _size; }
    bool getIsSolid() const { return _isSolid; }

    // setters
    void setSize(float size) { _size = size; }
    void setIsSolid(bool isSolid) { _isSolid = isSolid; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    float _size;
    bool _isSolid;
};

 
#endif /* defined(__interface__Volume3DOverlay__) */
