//
//  Grid3DOverlay.h
//  interface/src/ui/overlays
//
//  Created by Ryan Huffman on 11/06/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Grid3DOverlay_h
#define hifi_Grid3DOverlay_h

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <glm/glm.hpp>

#include <QGLWidget>
#include <SharedUtil.h>

#include "Base3DOverlay.h"

#include "renderer/ProgramObject.h"

class Grid3DOverlay : public Base3DOverlay {
    Q_OBJECT

public:
    Grid3DOverlay();
    ~Grid3DOverlay();

    virtual void render(RenderArgs* args);
    virtual void setProperties(const QScriptValue& properties);

    virtual Grid3DOverlay* createClone();

private:
    virtual void writeToClone(Grid3DOverlay* clone);

    float _minorGridWidth;
    int _majorGridEvery;

    static ProgramObject _gridProgram;
};

#endif // hifi_Grid3DOverlay_h
