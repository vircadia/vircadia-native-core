//
//  OverlayRenderer.h
//  interface/src/ui/overlays
//
//  Created by Benjamin Arnold on 5/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OverlayRenderer_h
#define hifi_OverlayRenderer_h

// Handles the drawing of the overlays to the scree
class OverlayRenderer {
public:

    OverlayRenderer();
    ~OverlayRenderer();
    void displayOverlay(class Overlays* overlays);

private:

    float _trailingAudioLoudness;
};

#endif // hifi_OverlayRenderer_h