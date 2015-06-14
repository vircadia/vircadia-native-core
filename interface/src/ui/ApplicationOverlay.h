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

#include <gpu/Texture.h>
class QOpenGLFramebufferObject;

// Handles the drawing of the overlays to the screen
// TODO, move divide up the rendering, displaying and input handling
// facilities of this class
class ApplicationOverlay : public QObject {
    Q_OBJECT
public:
    ApplicationOverlay();
    ~ApplicationOverlay();

    void renderOverlay(RenderArgs* renderArgs);
    GLuint getOverlayTexture();

private:

    void renderAudioMeter();
    void renderCameraToggle();
    void renderStatsAndLogs();
    void renderDomainConnectionStatusBorder();
    void buildFramebufferObject();
    float _alpha = 1.0f;
    float _trailingAudioLoudness;
    GLuint _uiTexture{ 0 };
    
    int _audioRedQuad;
    int _audioGreenQuad;
    int _audioBlueQuad;
    int _domainStatusBorder;
    int _magnifierBorder;

    int _previousBorderWidth;
    int _previousBorderHeight;

    QOpenGLFramebufferObject* _framebufferObject;
};

#endif // hifi_ApplicationOverlay_h
