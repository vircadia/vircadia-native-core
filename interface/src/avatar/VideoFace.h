//
//  VideoFace.h
//  interface
//
//  Created by Andrzej Kapolka on 7/11/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__VideoFace__
#define __interface__VideoFace__

#include <QObject>

#include <opencv2/opencv.hpp>

#include <vpx_codec.h>

#include "InterfaceConfig.h"

class Head;
class ProgramObject;

const float FULL_FRAME_ASPECT = 0.0f;

class VideoFace : public QObject {
    Q_OBJECT
    
public:
    
    VideoFace(Head* owningHead);
    ~VideoFace();

    bool isActive() const { return _colorTextureID != 0 || _depthTextureID != 0; }
    bool isFullFrame() const { return isActive() && _aspectRatio == FULL_FRAME_ASPECT; }

    void setFrameFromWebcam();
    void clearFrame();
    
    int processVideoMessage(unsigned char* packetData, size_t dataBytes);
    
    bool render(float alpha);
    
public slots:

    void cycleRenderMode();

private slots:

    void setFrame(const cv::Mat& color, const cv::Mat& depth, float aspectRatio);    
        
private:

    enum RenderMode { MESH, POINTS, RENDER_MODE_COUNT };

    void destroyCodecs();

    Head* _owningHead;
    RenderMode _renderMode;
    GLuint _colorTextureID;
    GLuint _depthTextureID;
    cv::Size2f _textureSize;
    cv::RotatedRect _textureRect;
    float _aspectRatio;

    vpx_codec_ctx_t _colorCodec;
    vpx_codec_ctx_t _depthCodec;
    bool _lastFullFrame;
    bool _lastDepthOnly;
    
    QByteArray _arrivingFrame;
    int _frameCount;
    int _frameBytesRemaining;
    
    struct Locations {
        int texCoordCorner;
        int texCoordRight;
        int texCoordUp;
    };

    static void loadProgram(ProgramObject& progam, const QString& suffix, const char* secondTextureUniform, Locations& locations);
    
    static bool _initialized;

    static ProgramObject _videoProgram;
    static Locations _videoProgramLocations;
    
    static ProgramObject _texturedProgram;
    static Locations _texturedProgramLocations;
    
    static GLuint _vboID;
    static GLuint _iboID;
};

#endif /* defined(__interface__VideoFace__) */
