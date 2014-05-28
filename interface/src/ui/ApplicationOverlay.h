//
//  ApplicationOverlay.h
//  interface/src/ui/overlays
//
//  Created by Benjamin Arnold on 5/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ApplicationOverlay_h
#define hifi_ApplicationOverlay_h

class Overlays;
class QOpenGLFramebufferObject;

// Handles the drawing of the overlays to the scree
class ApplicationOverlay {
public:

    ApplicationOverlay();
    ~ApplicationOverlay();

    void renderOverlay(bool renderToTexture = false);
    void displayOverlayTexture(Camera& whichCamera);
    void displayOverlayTextureOculus(Camera& whichCamera);

    // Getters
    QOpenGLFramebufferObject* getFramebufferObject();

private:

    QOpenGLFramebufferObject* _framebufferObject;
    float _trailingAudioLoudness;
};

#endif // hifi_ApplicationOverlay_h