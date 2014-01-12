//
//  Webcam.h
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Webcam__
#define __interface__Webcam__

#include <QMetaType>
#include <QThread>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <opencv2/opencv.hpp>

#if defined(HAVE_OPENNI) && !defined(Q_MOC_RUN)
#include <XnCppWrapper.h>
#endif

#ifdef LIBVPX
#include <vpx_codec.h>
#endif

#include "InterfaceConfig.h"

class QImage;

struct CvCapture;

class FrameGrabber;
class Joint;

typedef QVector<Joint> JointVector;
typedef std::vector<cv::KeyPoint> KeyPointVector;

/// Handles interaction with the webcam (including depth cameras such as the Kinect).
class Webcam : public QObject {
    Q_OBJECT

public:

    Webcam();
    ~Webcam();

    FrameGrabber* getGrabber() { return _grabber; }

    bool isActive() const { return _active; }

    bool isSending() const { return _sending; }

    GLuint getColorTextureID() const { return _colorTextureID; }
    GLuint getDepthTextureID() const { return _depthTextureID; }
    const cv::Size2f& getTextureSize() const { return _textureSize; }

    float getAspectRatio() const { return _aspectRatio; }

    const cv::RotatedRect& getFaceRect() const { return _faceRect; }

    const glm::vec3& getEstimatedPosition() const { return _estimatedPosition; }
    const glm::vec3& getEstimatedRotation() const { return _estimatedRotation; }
    const JointVector& getEstimatedJoints() const { return _estimatedJoints; }

    void reset();
    void renderPreview(int screenWidth, int screenHeight);

public slots:

    void setEnabled(bool enabled);
    void setFrame(const cv::Mat& color, int format, const cv::Mat& depth, float midFaceDepth, float aspectRatio,
        const cv::RotatedRect& faceRect, bool sending, const JointVector& joints, const KeyPointVector& keyPoints);
    void setSkeletonTrackingOn(bool toggle) { _skeletonTrackingOn = toggle; };

private:

    QThread _grabberThread;
    FrameGrabber* _grabber;

    bool _enabled;
    bool _active;
    bool _sending;
    GLuint _colorTextureID;
    GLuint _depthTextureID;
    cv::Size2f _textureSize;
    float _aspectRatio;
    cv::RotatedRect _faceRect;
    cv::RotatedRect _initialFaceRect;
    float _initialFaceDepth;
    JointVector _joints;
    KeyPointVector _keyPoints;

    glm::quat _initialLEDRotation;
    glm::vec3 _initialLEDPosition;
    float _initialLEDScale;

    uint64_t _startTimestamp;
    int _frameCount;

    uint64_t _lastFrameTimestamp;

    glm::vec3 _estimatedPosition;
    glm::vec3 _estimatedRotation;
    JointVector _estimatedJoints;

    bool _skeletonTrackingOn;
};

/// Acquires and processes video frames in a dedicated thread.
class FrameGrabber : public QObject {
    Q_OBJECT

public:

    FrameGrabber();
    virtual ~FrameGrabber();

public slots:

    void cycleVideoSendMode();
    void setDepthOnly(bool depthOnly);
    void setLEDTrackingOn(bool ledTrackingOn);
    void reset();
    void shutdown();
    void grabFrame();

private:

    enum VideoSendMode { NO_VIDEO, FACE_VIDEO, FULL_FRAME_VIDEO, VIDEO_SEND_MODE_COUNT };

    bool init();
    void updateHSVFrame(const cv::Mat& frame, int format);
    void destroyCodecs();
    void configureCapture();

    bool _initialized;
    VideoSendMode _videoSendMode;
    bool _depthOnly;
    bool _ledTrackingOn;
    CvCapture* _capture;
    cv::CascadeClassifier _faceCascade;
    cv::Mat _hsvFrame;
    cv::Mat _mask;
    cv::SparseMat _histogram;
    cv::Mat _backProject;
    cv::Rect _searchWindow;
    cv::Mat _grayDepthFrame;
    float _smoothedMidFaceDepth;

#ifdef LIBVPX
    vpx_codec_ctx_t _colorCodec;
    vpx_codec_ctx_t _depthCodec;
#endif
    int _frameCount;
    cv::Mat _faceColor;
    cv::Mat _faceDepth;
    cv::Mat _smoothedFaceDepth;
    QByteArray _encodedFace;
    cv::RotatedRect _smoothedFaceRect;

    cv::SimpleBlobDetector _blobDetector;
    cv::Mat _grayFrame;

#ifdef HAVE_OPENNI
    xn::Context _xnContext;
    xn::DepthGenerator _depthGenerator;
    xn::ImageGenerator _imageGenerator;
    xn::UserGenerator _userGenerator;
    xn::DepthMetaData _depthMetaData;
    xn::ImageMetaData _imageMetaData;
    XnUserID _userID;
#endif
};

/// Contains the 3D transform and 2D projected position of a tracked joint.
class Joint {
public:

    Joint(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& projected);
    Joint();

    bool isValid;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 projected;
};

Q_DECLARE_METATYPE(JointVector)
Q_DECLARE_METATYPE(KeyPointVector)
Q_DECLARE_METATYPE(cv::Mat)
Q_DECLARE_METATYPE(cv::RotatedRect)

#endif /* defined(__interface__Webcam__) */
