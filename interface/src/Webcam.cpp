//
//  Webcam.cpp
//  interface
//
//  Created by Andrzej Kapolka on 6/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QTimer>
#include <QtDebug>

#include <Log.h>
#include <SharedUtil.h>

#ifdef __APPLE__
#include <UVCCameraControl.hpp>
#endif

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

Webcam::Webcam() : _enabled(false), _active(false), _frameTextureID(0), _depthTextureID(0) {
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
    if (_enabled && _frameTextureID != 0) {
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glEnable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            const int PREVIEW_HEIGHT = 200;
            int previewWidth = _frameWidth * PREVIEW_HEIGHT / _frameHeight;
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
                int depthPreviewWidth = _depthWidth * PREVIEW_HEIGHT / _depthHeight;
                int depthLeft = screenWidth - depthPreviewWidth - 10;
                glTexCoord2f(0, 0);
                glVertex2f(depthLeft, top - PREVIEW_HEIGHT);
                glTexCoord2f(1, 0);
                glVertex2f(depthLeft + depthPreviewWidth, top - PREVIEW_HEIGHT);
                glTexCoord2f(1, 1);
                glVertex2f(depthLeft + depthPreviewWidth, top);
                glTexCoord2f(0, 1);
                glVertex2f(depthLeft, top);
            glEnd();
            
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            
            if (!_joints.isEmpty()) {
                glColor3f(1.0f, 0.0f, 0.0f);
                glPointSize(4.0f);
                glBegin(GL_POINTS);
                    float projectedScale = PREVIEW_HEIGHT / (float)_depthHeight;
                    foreach (const Joint& joint, _joints) {
                        if (joint.isValid) {
                            glVertex2f(depthLeft + joint.projected.x * projectedScale,
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
            float xScale = previewWidth / (float)_frameWidth;
            float yScale = PREVIEW_HEIGHT / (float)_frameHeight;
            glVertex2f(left + facePoints[0].x * xScale, top + facePoints[0].y * yScale);
            glVertex2f(left + facePoints[1].x * xScale, top + facePoints[1].y * yScale);
            glVertex2f(left + facePoints[2].x * xScale, top + facePoints[2].y * yScale);
            glVertex2f(left + facePoints[3].x * xScale, top + facePoints[3].y * yScale);
        glEnd();
        
        char fps[20];
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

void Webcam::setFrame(const Mat& frame, int format, const Mat& depth, const RotatedRect& faceRect, const JointVector& joints) {
}

FrameGrabber::FrameGrabber() : _initialized(false), _capture(0), _searchWindow(0, 0, 0, 0) {
}

FrameGrabber::~FrameGrabber() {
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
}

void FrameGrabber::shutdown() {
}

void FrameGrabber::grabFrame() {
}

bool FrameGrabber::init() {
}

void FrameGrabber::updateHSVFrame(const Mat& frame, int format) {
}

Joint::Joint(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& projected) :
    isValid(true), position(position), rotation(rotation), projected(projected) {
}

Joint::Joint() : isValid(false) {
}
