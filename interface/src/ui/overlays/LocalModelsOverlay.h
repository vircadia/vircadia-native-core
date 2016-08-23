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

#include "Volume3DOverlay.h"

class EntityTreeRenderer;

class LocalModelsOverlay : public Volume3DOverlay {
    Q_OBJECT
public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    LocalModelsOverlay(EntityTreeRenderer* entityTreeRenderer);
    LocalModelsOverlay(const LocalModelsOverlay* localModelsOverlay);

    virtual void update(float deltatime) override;
    virtual void render(RenderArgs* args) override;

    virtual LocalModelsOverlay* createClone() const override;

private:
    EntityTreeRenderer* _entityTreeRenderer;
};

#endif // hifi_LocalModelsOverlay_h
