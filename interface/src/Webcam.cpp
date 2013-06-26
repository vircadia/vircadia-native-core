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

// register OpenCV matrix type with Qt metatype system
int matMetaType = qRegisterMetaType<Mat>("cv::Mat");
int rotatedRectMetaType = qRegisterMetaType<RotatedRect>("cv::RotatedRect");

Webcam::Webcam() : _enabled(false), _active(false), _frameTextureID(0) {
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
        _grabberThread.quit();
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
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        
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

void Webcam::setFrame(const Mat& frame, const RotatedRect& faceRect) {
    IplImage image = frame;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image.widthStep / 3);
    if (_frameTextureID == 0) {
        glGenTextures(1, &_frameTextureID);
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _frameWidth = image.width, _frameHeight = image.height, 0, GL_BGR,
            GL_UNSIGNED_BYTE, image.imageData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        printLog("Capturing webcam at %dx%d.\n", _frameWidth, _frameHeight);
    
    } else {
        glBindTexture(GL_TEXTURE_2D, _frameTextureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _frameWidth, _frameHeight, GL_BGR, GL_UNSIGNED_BYTE, image.imageData);    
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // store our face rect, update our frame count for fps computation
    _faceRect = faceRect;
    _frameCount++;
    
    const int MAX_FPS = 60;
    const int MIN_FRAME_DELAY = 1000000 / MAX_FPS;
    long long now = usecTimestampNow();
    long long remaining = MIN_FRAME_DELAY;
    if (_startTimestamp == 0) {
        _startTimestamp = now;
    } else {
        remaining -= (now - _lastFrameTimestamp);
    }
    _lastFrameTimestamp = now;
    
    // roll is just the angle of the face rect (correcting for 180 degree rotations)
    float roll = faceRect.angle;
    if (roll < -90.0f) {
        roll += 180.0f;
        
    } else if (roll > 90.0f) {
        roll -= 180.0f;
    }
    const float ROTATION_SMOOTHING = 0.95f;
    _estimatedRotation.z = glm::mix(roll, _estimatedRotation.z, ROTATION_SMOOTHING);
    
    // determine position based on translation and scaling of the face rect
    if (_initialFaceRect.size.area() == 0) {
        _initialFaceRect = faceRect;
        _estimatedPosition = glm::vec3();
    
    } else {
        float proportion = sqrtf(_initialFaceRect.size.area() / (float)faceRect.size.area());
        const float DISTANCE_TO_CAMERA = 0.333f;
        const float POSITION_SCALE = 0.5f;
        float z = DISTANCE_TO_CAMERA * proportion - DISTANCE_TO_CAMERA;
        glm::vec3 position = glm::vec3(
            (faceRect.center.x - _initialFaceRect.center.x) * proportion * POSITION_SCALE / _frameWidth,
            (faceRect.center.y - _initialFaceRect.center.y) * proportion * POSITION_SCALE / _frameWidth,
            z);
        const float POSITION_SMOOTHING = 0.95f;
        _estimatedPosition = glm::mix(position, _estimatedPosition, POSITION_SMOOTHING);
    }
    
    // note that we have data
    _active = true;
    
    // let the grabber know we're ready for the next frame
    QTimer::singleShot(qMax((int)remaining / 1000, 0), _grabber, SLOT(grabFrame()));
}

FrameGrabber::FrameGrabber() : _capture(0), _searchWindow(0, 0, 0, 0), _freenectContext(0) {
}

FrameGrabber::~FrameGrabber() {
    if (_freenectContext != 0) {
        freenect_stop_depth(_freenectDevice);
        freenect_stop_video(_freenectDevice);
        freenect_close_device(_freenectDevice);
        freenect_shutdown(_freenectContext);
        
    } else if (_capture != 0) {
        cvReleaseCapture(&_capture);
    }
}

void FrameGrabber::reset() {
    _searchWindow = Rect(0, 0, 0, 0);
}

void FrameGrabber::grabFrame() {
    if (_capture == 0 && _freenectContext == 0 && !init()) {
        return;
    }
    if (_freenectContext != 0) {
        freenect_process_events(_freenectContext);
        return;
    }
    
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
    
    // if we don't have a search window (yet), try using the face cascade
    Mat frame = image;
    int channels = 0;
    float ranges[] = { 0, 180 };
    const float* range = ranges;
    if (_searchWindow.area() == 0) {
        vector<Rect> faces;
        _faceCascade.detectMultiScale(frame, faces, 1.1, 6);
        if (!faces.empty()) {
            _searchWindow = faces.front();
            updateHSVFrame(frame);
        
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
        updateHSVFrame(frame);
        
        calcBackProject(&_hsvFrame, 1, &channels, _histogram, _backProject, &range);
        bitwise_and(_backProject, _mask, _backProject);
        
        faceRect = CamShift(_backProject, _searchWindow, TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1));
        _searchWindow = faceRect.boundingRect();
    }   
    QMetaObject::invokeMethod(Application::getInstance()->getWebcam(), "setFrame",
        Q_ARG(cv::Mat, frame), Q_ARG(cv::RotatedRect, faceRect));
}

const char* FREENECT_LOG_LEVEL_NAMES[] = { "fatal", "error", "warning", "notice", "info", "debug", "spew", "flood" };

static void logCallback(freenect_context* freenectDevice, freenect_loglevel level, const char *msg) {
    printLog("Freenect %s: %s\n", FREENECT_LOG_LEVEL_NAMES[level], msg);
}

static void depthCallback(freenect_device* freenectDevice, void* depth, uint32_t timestamp) {
    qDebug() << "Got depth " << depth << timestamp; 
}

static void videoCallback(freenect_device* freenectDevice, void* video, uint32_t timestamp) {
    qDebug() << "Got video " << video << timestamp;
}

bool FrameGrabber::init() {
    // first try for a Kinect
    if (freenect_init(&_freenectContext, 0) >= 0) {
        freenect_set_log_level(_freenectContext, FREENECT_LOG_DEBUG);
        freenect_set_log_callback(_freenectContext, logCallback);
        freenect_select_subdevices(_freenectContext, FREENECT_DEVICE_CAMERA);
        
        if (freenect_num_devices(_freenectContext) > 0) {
            if (freenect_open_device(_freenectContext, &_freenectDevice, 0) >= 0) {
                freenect_set_depth_callback(_freenectDevice, depthCallback);
                freenect_set_video_callback(_freenectDevice, videoCallback);
                _freenectVideoMode = freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB);
                freenect_set_video_mode(_freenectDevice, _freenectVideoMode);
                _freenectDepthMode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT);
                freenect_set_depth_mode(_freenectDevice, _freenectDepthMode);
                
                freenect_start_depth(_freenectDevice);
                freenect_start_video(_freenectDevice);
                return true;
            }
        }
        freenect_shutdown(_freenectContext);
        _freenectContext = 0;
    }

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

    switchToResourcesParentIfRequired();
    if (!_faceCascade.load("resources/haarcascades/haarcascade_frontalface_alt.xml")) {
        printLog("Failed to load Haar cascade for face tracking.\n");
        return false;
    }
    return true;
}

void FrameGrabber::updateHSVFrame(const Mat& frame) {
    cvtColor(frame, _hsvFrame, CV_BGR2HSV);
    inRange(_hsvFrame, Scalar(0, 55, 65), Scalar(180, 256, 256), _mask);
}
