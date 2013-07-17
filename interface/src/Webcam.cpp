//
//  Webcam.cpp
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QTimer>
#include <QtDebug>

#include <vpx_encoder.h>
#include <vp8cx.h>

#ifdef __APPLE__
#include <UVCCameraControl.hpp>
#endif

#include <Log.h>
#include <SharedUtil.h>

#include "Application.h"
#include "Webcam.h"

using namespace cv;
using namespace std;

#ifdef HAVE_OPENNI
using namespace xn;
#endif

// register types with Qt metatype system
int jointVectorMetaType = qRegisterMetaType<JointVector>("JointVector");
int matMetaType = qRegisterMetaType<Mat>("cv::Mat");
int rotatedRectMetaType = qRegisterMetaType<RotatedRect>("cv::RotatedRect");

Webcam::Webcam() : _enabled(false), _active(false), _colorTextureID(0), _depthTextureID(0) {
    // the grabber simply runs as fast as possible
    _grabber = new FrameGrabber();
    _grabber->moveToThread(&_grabberThread);
}

void Webcam::setEnabled(bool enabled) {
    if (_enabled == enabled) {
        return;
    }
    if ((_enabled = enabled)) {
        _grabberThread.start();
        _startTimestamp = 0;
        _frameCount = 0;
        
        // let the grabber know we're ready for the first frame
        QMetaObject::invokeMethod(_grabber, "reset");
        QMetaObject::invokeMethod(_grabber, "grabFrame");
    
    } else {
        QMetaObject::invokeMethod(_grabber, "shutdown");
        _active = false;
    }
}

void Webcam::reset() {
    _initialFaceRect = RotatedRect();
    
    if (_enabled) {
        // send a message to the grabber
        QMetaObject::invokeMethod(_grabber, "reset");
    }
}

void Webcam::renderPreview(int screenWidth, int screenHeight) {
    if (_enabled && _colorTextureID != 0) {
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        glEnable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            const int PREVIEW_HEIGHT = 200;
            int previewWidth = _textureSize.width * PREVIEW_HEIGHT / _textureSize.height;
            int top = screenHeight - 600;
            int left = screenWidth - previewWidth - 10;
            
            glTexCoord2f(0, 0);
            glVertex2f(left, top);
            glTexCoord2f(1, 0);
            glVertex2f(left + previewWidth, top);
            glTexCoord2f(1, 1);
            glVertex2f(left + previewWidth, top + PREVIEW_HEIGHT);
            glTexCoord2f(0, 1);
            glVertex2f(left, top + PREVIEW_HEIGHT);
        glEnd();
        
        if (_depthTextureID != 0) {
            glBindTexture(GL_TEXTURE_2D, _depthTextureID);
            glBegin(GL_QUADS);
                glTexCoord2f(0, 0);
                glVertex2f(left, top - PREVIEW_HEIGHT);
                glTexCoord2f(1, 0);
                glVertex2f(left + previewWidth, top - PREVIEW_HEIGHT);
                glTexCoord2f(1, 1);
                glVertex2f(left + previewWidth, top);
                glTexCoord2f(0, 1);
                glVertex2f(left, top);
            glEnd();
            
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            
            if (!_joints.isEmpty()) {
                glColor3f(1.0f, 0.0f, 0.0f);
                glPointSize(4.0f);
                glBegin(GL_POINTS);
                    float projectedScale = PREVIEW_HEIGHT / _textureSize.height;
                    foreach (const Joint& joint, _joints) {
                        if (joint.isValid) {
                            glVertex2f(left + joint.projected.x * projectedScale,
                                top - PREVIEW_HEIGHT + joint.projected.y * projectedScale);
                        }
                    }
                glEnd();
                glPointSize(1.0f);
            }
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
        }
        
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
            Point2f facePoints[4];
            _faceRect.points(facePoints);
            float xScale = previewWidth / _textureSize.width;
            float yScale = PREVIEW_HEIGHT / _textureSize.height;
            glVertex2f(left + facePoints[0].x * xScale, top + facePoints[0].y * yScale);
            glVertex2f(left + facePoints[1].x * xScale, top + facePoints[1].y * yScale);
            glVertex2f(left + facePoints[2].x * xScale, top + facePoints[2].y * yScale);
            glVertex2f(left + facePoints[3].x * xScale, top + facePoints[3].y * yScale);
        glEnd();
        
        char fps[30];
        sprintf(fps, "FPS: %d", (int)(roundf(_frameCount * 1000000.0f / (usecTimestampNow() - _startTimestamp))));
        drawtext(left, top + PREVIEW_HEIGHT + 20, 0.10, 0, 1, 0, fps);
    }
}

Webcam::~Webcam() {
    // stop the grabber thread
    _grabberThread.quit();
    _grabberThread.wait();
    
    delete _grabber;
}

void Webcam::setFrame(const Mat& color, int format, const Mat& depth, const RotatedRect& faceRect, const JointVector& joints) {
    IplImage colorImage = color;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, colorImage.widthStep / 3);
    if (_colorTextureID == 0) {
        glGenTextures(1, &_colorTextureID);
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _textureSize.width = colorImage.width, _textureSize.height = colorImage.height,
            0, format, GL_UNSIGNED_BYTE, colorImage.imageData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        printLog("Capturing video at %gx%g.\n", _textureSize.width, _textureSize.height);
    
    } else {
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _textureSize.width, _textureSize.height, format,
            GL_UNSIGNED_BYTE, colorImage.imageData);
    }
    
    if (!depth.empty()) {
        IplImage depthImage = depth;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, depthImage.widthStep);
        if (_depthTextureID == 0) {
            glGenTextures(1, &_depthTextureID);
            glBindTexture(GL_TEXTURE_2D, _depthTextureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, depthImage.width, depthImage.height, 0,
                GL_LUMINANCE, GL_UNSIGNED_BYTE, depthImage.imageData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            
        } else {
            glBindTexture(GL_TEXTURE_2D, _depthTextureID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _textureSize.width, _textureSize.height, GL_LUMINANCE,
                GL_UNSIGNED_BYTE, depthImage.imageData);        
        }
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // store our face rect and joints, update our frame count for fps computation
    _faceRect = faceRect;
    _joints = joints;
    _frameCount++;
    
    const int MAX_FPS = 60;
    const int MIN_FRAME_DELAY = 1000000 / MAX_FPS;
    uint64_t now = usecTimestampNow();
    int remaining = MIN_FRAME_DELAY;
    if (_startTimestamp == 0) {
        _startTimestamp = now;
    } else {
        remaining -= (now - _lastFrameTimestamp);
    }
    _lastFrameTimestamp = now;
    
    // correct for 180 degree rotations
    if (_faceRect.angle < -90.0f) {
        _faceRect.angle += 180.0f;
        
    } else if (_faceRect.angle > 90.0f) {
        _faceRect.angle -= 180.0f;
    }
    
    // compute the smoothed face rect
    if (_estimatedFaceRect.size.area() == 0) {
        _estimatedFaceRect = _faceRect;
        
    } else {
        const float FACE_RECT_SMOOTHING = 0.9f;
        _estimatedFaceRect.center.x = glm::mix(_faceRect.center.x, _estimatedFaceRect.center.x, FACE_RECT_SMOOTHING);
        _estimatedFaceRect.center.y = glm::mix(_faceRect.center.y, _estimatedFaceRect.center.y, FACE_RECT_SMOOTHING);
        _estimatedFaceRect.size.width = glm::mix(_faceRect.size.width, _estimatedFaceRect.size.width, FACE_RECT_SMOOTHING);
        _estimatedFaceRect.size.height = glm::mix(_faceRect.size.height, _estimatedFaceRect.size.height, FACE_RECT_SMOOTHING); 
        _estimatedFaceRect.angle = glm::mix(_faceRect.angle, _estimatedFaceRect.angle, FACE_RECT_SMOOTHING);
    }
    
    // see if we have joint data
    if (!_joints.isEmpty()) {
        _estimatedJoints.resize(NUM_AVATAR_JOINTS);
        glm::vec3 origin;
        if (_joints[AVATAR_JOINT_LEFT_HIP].isValid && _joints[AVATAR_JOINT_RIGHT_HIP].isValid) {
            origin = glm::mix(_joints[AVATAR_JOINT_LEFT_HIP].position, _joints[AVATAR_JOINT_RIGHT_HIP].position, 0.5f);
        
        } else if (_joints[AVATAR_JOINT_TORSO].isValid) {
            const glm::vec3 TORSO_TO_PELVIS = glm::vec3(0.0f, -0.09f, -0.01f);
            origin = _joints[AVATAR_JOINT_TORSO].position + TORSO_TO_PELVIS;
        }
        for (int i = 0; i < NUM_AVATAR_JOINTS; i++) {
            if (!_joints[i].isValid) {
                continue;
            }
            const float JOINT_SMOOTHING = 0.5f;
            _estimatedJoints[i].isValid = true;
            _estimatedJoints[i].position = glm::mix(_joints[i].position - origin,
                _estimatedJoints[i].position, JOINT_SMOOTHING);
            _estimatedJoints[i].rotation = safeMix(_joints[i].rotation,
                _estimatedJoints[i].rotation, JOINT_SMOOTHING);
        }
        _estimatedRotation = safeEulerAngles(_estimatedJoints[AVATAR_JOINT_HEAD_BASE].rotation);
        _estimatedPosition = _estimatedJoints[AVATAR_JOINT_HEAD_BASE].position;
        
    } else {
        // roll is just the angle of the face rect
        const float ROTATION_SMOOTHING = 0.95f;
        _estimatedRotation.z = glm::mix(_faceRect.angle, _estimatedRotation.z, ROTATION_SMOOTHING);
        
        // determine position based on translation and scaling of the face rect
        if (_initialFaceRect.size.area() == 0) {
            _initialFaceRect = _faceRect;
            _estimatedPosition = glm::vec3();
        
        } else {
            float proportion = sqrtf(_initialFaceRect.size.area() / (float)_faceRect.size.area());
            const float DISTANCE_TO_CAMERA = 0.333f;
            const float POSITION_SCALE = 0.5f;
            float z = DISTANCE_TO_CAMERA * proportion - DISTANCE_TO_CAMERA;
            glm::vec3 position = glm::vec3(
                (_faceRect.center.x - _initialFaceRect.center.x) * proportion * POSITION_SCALE / _textureSize.width,
                (_faceRect.center.y - _initialFaceRect.center.y) * proportion * POSITION_SCALE / _textureSize.width,
                z);
            const float POSITION_SMOOTHING = 0.95f;
            _estimatedPosition = glm::mix(position, _estimatedPosition, POSITION_SMOOTHING);
        }
    }
    
    // note that we have data
    _active = true;
    
    // let the grabber know we're ready for the next frame
    QTimer::singleShot(qMax((int)remaining / 1000, 0), _grabber, SLOT(grabFrame()));
}

FrameGrabber::FrameGrabber() : _initialized(false), _capture(0), _searchWindow(0, 0, 0, 0), _depthOffset(0.0) {
}

FrameGrabber::~FrameGrabber() {
    if (_initialized) {
        shutdown();
    }
}

#ifdef HAVE_OPENNI
static AvatarJointID xnToAvatarJoint(XnSkeletonJoint joint) {
    switch (joint) {
        case XN_SKEL_HEAD: return AVATAR_JOINT_HEAD_TOP;
        case XN_SKEL_NECK: return AVATAR_JOINT_HEAD_BASE;
        case XN_SKEL_TORSO: return AVATAR_JOINT_CHEST;
        
        case XN_SKEL_LEFT_SHOULDER: return AVATAR_JOINT_RIGHT_ELBOW;
        case XN_SKEL_LEFT_ELBOW: return AVATAR_JOINT_RIGHT_WRIST;
        
        case XN_SKEL_RIGHT_SHOULDER: return AVATAR_JOINT_LEFT_ELBOW;
        case XN_SKEL_RIGHT_ELBOW: return AVATAR_JOINT_LEFT_WRIST;
        
        case XN_SKEL_LEFT_HIP: return AVATAR_JOINT_RIGHT_KNEE;
        case XN_SKEL_LEFT_KNEE: return AVATAR_JOINT_RIGHT_HEEL;
        case XN_SKEL_LEFT_FOOT: return AVATAR_JOINT_RIGHT_TOES;
        
        case XN_SKEL_RIGHT_HIP: return AVATAR_JOINT_LEFT_KNEE;
        case XN_SKEL_RIGHT_KNEE: return AVATAR_JOINT_LEFT_HEEL;
        case XN_SKEL_RIGHT_FOOT: return AVATAR_JOINT_LEFT_TOES;
        
        default: return AVATAR_JOINT_NULL;
    }
}

static int getParentJoint(XnSkeletonJoint joint) {
    switch (joint) {
        case XN_SKEL_HEAD: return XN_SKEL_NECK;
        case XN_SKEL_TORSO: return -1;
        
        case XN_SKEL_LEFT_ELBOW: return XN_SKEL_LEFT_SHOULDER;
        case XN_SKEL_LEFT_HAND: return XN_SKEL_LEFT_ELBOW;
        
        case XN_SKEL_RIGHT_ELBOW: return XN_SKEL_RIGHT_SHOULDER;
        case XN_SKEL_RIGHT_HAND: return XN_SKEL_RIGHT_ELBOW;
        
        case XN_SKEL_LEFT_KNEE: return XN_SKEL_LEFT_HIP;
        case XN_SKEL_LEFT_FOOT: return XN_SKEL_LEFT_KNEE;
        
        case XN_SKEL_RIGHT_KNEE: return XN_SKEL_RIGHT_HIP;
        case XN_SKEL_RIGHT_FOOT: return XN_SKEL_RIGHT_KNEE;
        
        default: return XN_SKEL_TORSO;
    }
}

static glm::vec3 xnToGLM(const XnVector3D& vector, bool flip = false) {
    return glm::vec3(vector.X * (flip ? -1 : 1), vector.Y, vector.Z);
}

static glm::quat xnToGLM(const XnMatrix3X3& matrix) {
    glm::quat rotation = glm::quat_cast(glm::mat3(
        matrix.elements[0], matrix.elements[1], matrix.elements[2],
        matrix.elements[3], matrix.elements[4], matrix.elements[5],
        matrix.elements[6], matrix.elements[7], matrix.elements[8]));
    return glm::quat(rotation.w, -rotation.x, rotation.y, rotation.z);
}

static void XN_CALLBACK_TYPE newUser(UserGenerator& generator, XnUserID id, void* cookie) {
    printLog("Found user %d.\n", id);
    generator.GetSkeletonCap().RequestCalibration(id, false);
}

static void XN_CALLBACK_TYPE lostUser(UserGenerator& generator, XnUserID id, void* cookie) {
    printLog("Lost user %d.\n", id);
}

static void XN_CALLBACK_TYPE calibrationStarted(SkeletonCapability& capability, XnUserID id, void* cookie) {
    printLog("Calibration started for user %d.\n", id);
}

static void XN_CALLBACK_TYPE calibrationCompleted(SkeletonCapability& capability,
        XnUserID id, XnCalibrationStatus status, void* cookie) {
    if (status == XN_CALIBRATION_STATUS_OK) {
        printLog("Calibration completed for user %d.\n", id);
        capability.StartTracking(id);
    
    } else {
        printLog("Calibration failed to user %d.\n", id);
        capability.RequestCalibration(id, true);
    }
}
#endif

void FrameGrabber::reset() {
    _searchWindow = cv::Rect(0, 0, 0, 0);

#ifdef HAVE_OPENNI
    if (_userGenerator.IsValid() && _userGenerator.GetSkeletonCap().IsTracking(_userID)) {
        _userGenerator.GetSkeletonCap().RequestCalibration(_userID, true);
    }
#endif
}

void FrameGrabber::shutdown() {
    if (_capture != 0) {
        cvReleaseCapture(&_capture);
        _capture = 0;
    }
    _initialized = false;
    
    thread()->quit();
}

static Point clip(const Point& point, const Rect& bounds) {
    return Point(glm::clamp(point.x, bounds.x, bounds.x + bounds.width - 1),
        glm::clamp(point.y, bounds.y, bounds.y + bounds.height - 1));
}

void FrameGrabber::grabFrame() {
    if (!(_initialized || init())) {
        return;
    }
    int format = GL_BGR;
    Mat color, depth;
    JointVector joints;
    
#ifdef HAVE_OPENNI
    if (_depthGenerator.IsValid()) {
        _xnContext.WaitAnyUpdateAll();
        color = Mat(_imageMetaData.YRes(), _imageMetaData.XRes(), CV_8UC3, (void*)_imageGenerator.GetImageMap());
        format = GL_RGB;
        
        depth = Mat(_depthMetaData.YRes(), _depthMetaData.XRes(), CV_16UC1, (void*)_depthGenerator.GetDepthMap());
        
        _userID = 0;
        XnUInt16 userCount = 1; 
        _userGenerator.GetUsers(&_userID, userCount);
        if (userCount > 0 && _userGenerator.GetSkeletonCap().IsTracking(_userID)) {
            joints.resize(NUM_AVATAR_JOINTS);
            const int MAX_ACTIVE_JOINTS = 16;
            XnSkeletonJoint activeJoints[MAX_ACTIVE_JOINTS];
            XnUInt16 activeJointCount = MAX_ACTIVE_JOINTS;
            _userGenerator.GetSkeletonCap().EnumerateActiveJoints(activeJoints, activeJointCount);
            XnSkeletonJointTransformation transform;
            for (int i = 0; i < activeJointCount; i++) {
                AvatarJointID avatarJoint = xnToAvatarJoint(activeJoints[i]);
                if (avatarJoint == AVATAR_JOINT_NULL) {
                    continue;
                }
                _userGenerator.GetSkeletonCap().GetSkeletonJoint(_userID, activeJoints[i], transform);
                XnVector3D projected;
                _depthGenerator.ConvertRealWorldToProjective(1, &transform.position.position, &projected);
                glm::quat rotation = xnToGLM(transform.orientation.orientation);
                int parentJoint = getParentJoint(activeJoints[i]);
                if (parentJoint != -1) {
                    XnSkeletonJointOrientation parentOrientation;
                    _userGenerator.GetSkeletonCap().GetSkeletonJointOrientation(
                        _userID, (XnSkeletonJoint)parentJoint, parentOrientation);
                    rotation = glm::inverse(xnToGLM(parentOrientation.orientation)) * rotation;
                }
                const float METERS_PER_MM = 1.0f / 1000.0f;
                joints[avatarJoint] = Joint(xnToGLM(transform.position.position, true) * METERS_PER_MM,
                    rotation, xnToGLM(projected));
            }
        }
    }
#endif

    if (color.empty()) {
        IplImage* image = cvQueryFrame(_capture);
        if (image == 0) {
            // try again later
            QMetaObject::invokeMethod(this, "grabFrame", Qt::QueuedConnection);
            return;
        }
        // make sure it's in the format we expect
        if (image->nChannels != 3 || image->depth != IPL_DEPTH_8U || image->dataOrder != IPL_DATA_ORDER_PIXEL ||
                image->origin != 0) {
            printLog("Invalid webcam image format.\n");
            return;
        }
        color = image;
    }
    
    // if we don't have a search window (yet), try using the face cascade
    int channels = 0;
    float ranges[] = { 0, 180 };
    const float* range = ranges;
    if (_searchWindow.area() == 0) {
        vector<Rect> faces;
        _faceCascade.detectMultiScale(color, faces, 1.1, 6);
        if (!faces.empty()) {
            _searchWindow = faces.front();
            updateHSVFrame(color, format);
        
            Mat faceHsv(_hsvFrame, _searchWindow);
            Mat faceMask(_mask, _searchWindow);
            int sizes = 30;
            calcHist(&faceHsv, 1, &channels, faceMask, _histogram, 1, &sizes, &range);
            double min, max;
            minMaxLoc(_histogram, &min, &max);
            _histogram.convertTo(_histogram, -1, (max == 0.0) ? 0.0 : 255.0 / max);
        }
    }
    RotatedRect faceRect;
    if (_searchWindow.area() > 0) {
        updateHSVFrame(color, format);
        
        calcBackProject(&_hsvFrame, 1, &channels, _histogram, _backProject, &range);
        bitwise_and(_backProject, _mask, _backProject);
        
        faceRect = CamShift(_backProject, _searchWindow, TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1));
        Rect faceBounds = faceRect.boundingRect();
        Rect imageBounds(0, 0, color.cols, color.rows);
        _searchWindow = Rect(clip(faceBounds.tl(), imageBounds), clip(faceBounds.br(), imageBounds));
    }

#ifdef HAVE_OPENNI
    if (_depthGenerator.IsValid()) {
        if (_searchWindow.area() > 0) {
            const double DEPTH_OFFSET_SMOOTHING = 0.95;
            double meanOffset = 128.0 - mean(depth(_searchWindow))[0];
            _depthOffset = (_depthOffset == 0.0) ? meanOffset : glm::mix(meanOffset, _depthOffset, DEPTH_OFFSET_SMOOTHING);
        }
        depth.convertTo(_grayDepthFrame, CV_8UC1, 1.0, _depthOffset);
    }
#endif

    //vpx_codec_encode(&_encoderContext, img, pts, duration, 0, VPX_DL_REALTIME);

    QMetaObject::invokeMethod(Application::getInstance()->getWebcam(), "setFrame",
        Q_ARG(cv::Mat, color), Q_ARG(int, format), Q_ARG(cv::Mat, _grayDepthFrame),
        Q_ARG(cv::RotatedRect, faceRect), Q_ARG(JointVector, joints));
}

bool FrameGrabber::init() {
    _initialized = true;

    // load our face cascade
    switchToResourcesParentIfRequired();
    if (_faceCascade.empty() && !_faceCascade.load("resources/haarcascades/haarcascade_frontalface_alt.xml")) {
        printLog("Failed to load Haar cascade for face tracking.\n");
        return false;
    }

    // initialize encoder context
    vpx_codec_enc_init(&_encoderContext, vpx_codec_vp8_cx(), 0, 0);

    // first try for a Kinect
#ifdef HAVE_OPENNI
    _xnContext.Init();
    if (_depthGenerator.Create(_xnContext) == XN_STATUS_OK && _imageGenerator.Create(_xnContext) == XN_STATUS_OK &&
            _userGenerator.Create(_xnContext) == XN_STATUS_OK &&
                _userGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON)) {
        _depthGenerator.GetMetaData(_depthMetaData);
        _imageGenerator.SetPixelFormat(XN_PIXEL_FORMAT_RGB24);
        _imageGenerator.GetMetaData(_imageMetaData);
        
        XnCallbackHandle userCallbacks, calibrationStartCallback, calibrationCompleteCallback;
        _userGenerator.RegisterUserCallbacks(newUser, lostUser, 0, userCallbacks);
        _userGenerator.GetSkeletonCap().RegisterToCalibrationStart(calibrationStarted, 0, calibrationStartCallback);
        _userGenerator.GetSkeletonCap().RegisterToCalibrationComplete(calibrationCompleted, 0, calibrationCompleteCallback);
        
        _userGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_UPPER);
        
        // make the depth viewpoint match that of the video image
        if (_depthGenerator.IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT)) {
            _depthGenerator.GetAlternativeViewPointCap().SetViewPoint(_imageGenerator);
        }
        
        _xnContext.StartGeneratingAll();
        return true;
    }
#endif

    // next, an ordinary webcam
    if ((_capture = cvCaptureFromCAM(-1)) == 0) {
        printLog("Failed to open webcam.\n");
        return false;
    }
    const int IDEAL_FRAME_WIDTH = 320;
    const int IDEAL_FRAME_HEIGHT = 240;
    cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_WIDTH, IDEAL_FRAME_WIDTH);
    cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_HEIGHT, IDEAL_FRAME_HEIGHT);
    
#ifdef __APPLE__
    configureCamera(0x5ac, 0x8510, false, 0.975, 0.5, 1.0, 0.5, true, 0.5);
#else
    cvSetCaptureProperty(_capture, CV_CAP_PROP_EXPOSURE, 0.5);
    cvSetCaptureProperty(_capture, CV_CAP_PROP_CONTRAST, 0.5);
    cvSetCaptureProperty(_capture, CV_CAP_PROP_SATURATION, 0.5);
    cvSetCaptureProperty(_capture, CV_CAP_PROP_BRIGHTNESS, 0.5);
    cvSetCaptureProperty(_capture, CV_CAP_PROP_HUE, 0.5);
    cvSetCaptureProperty(_capture, CV_CAP_PROP_GAIN, 0.5);
#endif

    return true;
}

void FrameGrabber::updateHSVFrame(const Mat& frame, int format) {
    cvtColor(frame, _hsvFrame, format == GL_RGB ? CV_RGB2HSV : CV_BGR2HSV);
    inRange(_hsvFrame, Scalar(0, 55, 65), Scalar(180, 256, 256), _mask);
}

Joint::Joint(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& projected) :
    isValid(true), position(position), rotation(rotation), projected(projected) {
}

Joint::Joint() : isValid(false) {
}
