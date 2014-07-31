//
//  LocalModelsOverlay.h
//  interface/src/ui/overlays
//
//  Created by Ryan Huffman on 07/08/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LocalModelsOverlay_h
#define hifi_LocalModelsOverlay_h

// #include "models/ModelTree.h"
#include "models/ModelTreeRenderer.h"

#include "Volume3DOverlay.h"

class LocalModelsOverlay : public Volume3DOverlay {
    Q_OBJECT
public:
    LocalModelsOverlay(ModelTreeRenderer* modelTreeRenderer);
    ~LocalModelsOverlay();
    
    virtual void update(float deltatime);
    virtual void render();

private:
    ModelTreeRenderer *_modelTreeRenderer;
};

#endif // hifi_LocalModelsOverlay_h
