//
//  Face.h
//  interface
//
//  Created by Andrzej Kapolka on 7/11/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Face__
#define __interface__Face__

#include <QObject>

#include <opencv2/opencv.hpp>

#include "InterfaceConfig.h"

class Head;
class ProgramObject;

class Face : public QObject {
    Q_OBJECT
    
public:
    
    Face(Head* owningHead);

    void setColorTextureID(GLuint colorTextureID) { _colorTextureID = colorTextureID; }
    void setDepthTextureID(GLuint depthTextureID) { _depthTextureID = depthTextureID; }
    void setTextureSize(const cv::Size2f& textureSize) { _textureSize = textureSize; }
    void setTextureRect(const cv::RotatedRect& textureRect) { _textureRect = textureRect; }
    
    bool render(float alpha);
    
public slots:

    void cycleRenderMode();
    
private:

    enum RenderMode { MESH, POINTS, RENDER_MODE_COUNT };

    Head* _owningHead;
    RenderMode _renderMode;
    GLuint _colorTextureID;
    GLuint _depthTextureID;
    cv::Size2f _textureSize;
    cv::RotatedRect _textureRect;
    
    static ProgramObject* _program;
    static int _texCoordCornerLocation;
    static int _texCoordRightLocation;
    static int _texCoordUpLocation;
    static GLuint _vboID;
    static GLuint _iboID;
};

#endif /* defined(__interface__Face__) */
