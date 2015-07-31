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

#include "Planar3DOverlay.h"

class Grid3DOverlay : public Planar3DOverlay {
    Q_OBJECT

public:
    Grid3DOverlay();
    Grid3DOverlay(const Grid3DOverlay* grid3DOverlay);

    virtual void render(RenderArgs* args);
    virtual void setProperties(const QScriptValue& properties);
    virtual QScriptValue getProperty(const QString& property);

    virtual Grid3DOverlay* createClone() const;

private:
    float _minorGridWidth;
    int _majorGridEvery;
};

#endif // hifi_Grid3DOverlay_h
