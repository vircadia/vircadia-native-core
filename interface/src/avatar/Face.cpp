//
//  Face.cpp
//  interface
//
//  Created by Andrzej Kapolka on 7/11/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtx/quaternion.hpp>

#include <vpx_decoder.h>
#include <vp8dx.h>

#include <PacketHeaders.h>

#include "Application.h"
#include "Avatar.h"
#include "Head.h"
#include "Face.h"
#include "renderer/ProgramObject.h"

using namespace cv;

ProgramObject* Face::_videoProgram = 0;
Face::Locations Face::_videoProgramLocations;
ProgramObject* Face::_texturedProgram = 0;
Face::Locations Face::_texturedProgramLocations;
GLuint Face::_vboID;
GLuint Face::_iboID;

Face::Face(Head* owningHead) : _owningHead(owningHead), _renderMode(MESH),
        _colorTextureID(0), _depthTextureID(0), _colorCodec(), _depthCodec(), _frameCount(0) {
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
}

Face::~Face() {
    if (_colorCodec.name != 0) {
        vpx_codec_destroy(&_colorCodec);
        
        // delete our texture, since we know that we own it
        if (_colorTextureID != 0) {
            glDeleteTextures(1, &_colorTextureID);
        }
        
    }
    if (_depthCodec.name != 0) {
        vpx_codec_destroy(&_depthCodec);
        
        // delete our texture, since we know that we own it
        if (_depthTextureID != 0) {
            glDeleteTextures(1, &_depthTextureID);
        }
    }
}

void Face::setFrameFromWebcam() {
    Webcam* webcam = Application::getInstance()->getWebcam();
    if (webcam->isSending()) {
        _colorTextureID = webcam->getColorTextureID();
        _depthTextureID = webcam->getDepthTextureID();
        _textureSize = webcam->getTextureSize();
        _textureRect = webcam->getFaceRect();
        _aspectRatio = webcam->getAspectRatio();
    } else {
        clearFrame();
    }
}

void Face::clearFrame() {
    _colorTextureID = 0;
    _depthTextureID = 0;
}
    
int Face::processVideoMessage(unsigned char* packetData, size_t dataBytes) {
    unsigned char* packetPosition = packetData;

    int frameCount = *(uint32_t*)packetPosition;
    packetPosition += sizeof(uint32_t);
    
    int frameSize = *(uint32_t*)packetPosition;
    packetPosition += sizeof(uint32_t);
    
    int frameOffset = *(uint32_t*)packetPosition;
    packetPosition += sizeof(uint32_t);
    
    if (frameCount < _frameCount) { // old frame; ignore
        return dataBytes; 
    
    } else if (frameCount > _frameCount) { // new frame; reset
        _frameCount = frameCount;
        _frameBytesRemaining = frameSize;
        _arrivingFrame.resize(frameSize);
    }
    
    int payloadSize = dataBytes - (packetPosition - packetData);
    memcpy(_arrivingFrame.data() + frameOffset, packetPosition, payloadSize);
    
    if ((_frameBytesRemaining -= payloadSize) > 0) {
        return dataBytes; // wait for the rest of the frame
    }
    
    if (frameSize == 0) {
        // destroy the codecs, if we have any
        destroyCodecs();
    
        // disables video data
        QMetaObject::invokeMethod(this, "setFrame", Q_ARG(cv::Mat, Mat()),
            Q_ARG(cv::Mat, Mat()), Q_ARG(float, 0.0f));
        return dataBytes;
    }
    
    // the switch between full frame or depth only modes requires us to reinit the codecs
    float aspectRatio = *(const float*)_arrivingFrame.constData();
    size_t colorSize = *(const size_t*)(_arrivingFrame.constData() + sizeof(float));
    bool fullFrame = (aspectRatio == FULL_FRAME_ASPECT);
    bool depthOnly = (colorSize == 0);
    if (fullFrame != _lastFullFrame || depthOnly != _lastDepthOnly) {
        destroyCodecs();
        _lastFullFrame = fullFrame;
        _lastDepthOnly = depthOnly;
    }
    
    // read the color data, if non-empty
    Mat color;
    const uint8_t* colorData = (const uint8_t*)(_arrivingFrame.constData() + sizeof(float) + sizeof(size_t));
    if (colorSize > 0) {
        if (_colorCodec.name == 0) {
            // initialize decoder context
            vpx_codec_dec_init(&_colorCodec, vpx_codec_vp8_dx(), 0, 0);
        }
        vpx_codec_decode(&_colorCodec, colorData, colorSize, 0, 0);
        vpx_codec_iter_t iterator = 0;
        vpx_image_t* image;
        while ((image = vpx_codec_get_frame(&_colorCodec, &iterator)) != 0) {
            // convert from YV12 to RGB: see http://www.fourcc.org/yuv.php and
            // http://docs.opencv.org/modules/imgproc/doc/miscellaneous_transformations.html#cvtcolor
            color.create(image->d_h, image->d_w, CV_8UC3);
            uchar* yline = image->planes[0];
            uchar* vline = image->planes[1];
            uchar* uline = image->planes[2];
            const int RED_V_WEIGHT = (int)(1.403 * 256);
            const int GREEN_V_WEIGHT = (int)(0.714 * 256);
            const int GREEN_U_WEIGHT = (int)(0.344 * 256);
            const int BLUE_U_WEIGHT = (int)(1.773 * 256);
            for (int i = 0; i < image->d_h; i += 2) {
                uchar* ysrc = yline;
                uchar* vsrc = vline;
                uchar* usrc = uline;
                for (int j = 0; j < image->d_w; j += 2) {
                    uchar* tl = color.ptr(i, j);
                    uchar* tr = color.ptr(i, j + 1);
                    uchar* bl = color.ptr(i + 1, j);
                    uchar* br = color.ptr(i + 1, j + 1);
                    
                    int v = *vsrc++ - 128;
                    int u = *usrc++ - 128;
                    
                    int redOffset = (RED_V_WEIGHT * v) >> 8;
                    int greenOffset = (GREEN_V_WEIGHT * v + GREEN_U_WEIGHT * u) >> 8;
                    int blueOffset = (BLUE_U_WEIGHT * u) >> 8;
                    
                    int ytl = ysrc[0];
                    int ytr = ysrc[1];
                    int ybl = ysrc[image->w];
                    int ybr = ysrc[image->w + 1];
                    ysrc += 2;
                    
                    tl[0] = saturate_cast<uchar>(ytl + redOffset);
                    tl[1] = saturate_cast<uchar>(ytl - greenOffset);
                    tl[2] = saturate_cast<uchar>(ytl + blueOffset);
                    
                    tr[0] = saturate_cast<uchar>(ytr + redOffset);
                    tr[1] = saturate_cast<uchar>(ytr - greenOffset); 
                    tr[2] = saturate_cast<uchar>(ytr + blueOffset);
                    
                    bl[0] = saturate_cast<uchar>(ybl + redOffset);
                    bl[1] = saturate_cast<uchar>(ybl - greenOffset);
                    bl[2] = saturate_cast<uchar>(ybl + blueOffset);
                    
                    br[0] = saturate_cast<uchar>(ybr + redOffset);
                    br[1] = saturate_cast<uchar>(ybr - greenOffset);
                    br[2] = saturate_cast<uchar>(ybr + blueOffset);
                }
                yline += image->stride[0] * 2;
                vline += image->stride[1];
                uline += image->stride[2];
            }
        }
    } else if (_colorCodec.name != 0) {
        vpx_codec_destroy(&_colorCodec);
        _colorCodec.name = 0;
    }
    
    // read the depth data, if non-empty
    Mat depth;
    const uint8_t* depthData = colorData + colorSize;
    int depthSize = _arrivingFrame.size() - ((const char*)depthData - _arrivingFrame.constData());
    if (depthSize > 0) {
        if (_depthCodec.name == 0) {
            // initialize decoder context
            vpx_codec_dec_init(&_depthCodec, vpx_codec_vp8_dx(), 0, 0);
        }
        vpx_codec_decode(&_depthCodec, depthData, depthSize, 0, 0);
        vpx_codec_iter_t iterator = 0;
        vpx_image_t* image;
        while ((image = vpx_codec_get_frame(&_depthCodec, &iterator)) != 0) {
            depth.create(image->d_h, image->d_w, CV_8UC1);
            uchar* yline = image->planes[0];
            uchar* vline = image->planes[1];
            const uchar EIGHT_BIT_MAXIMUM = 255;
            const uchar MASK_THRESHOLD = 192;
            for (int i = 0; i < image->d_h; i += 2) {
                uchar* ysrc = yline;
                uchar* vsrc = vline;
                for (int j = 0; j < image->d_w; j += 2) {
                    if (*vsrc++ < MASK_THRESHOLD) {
                        *depth.ptr(i, j) = EIGHT_BIT_MAXIMUM;
                        *depth.ptr(i, j + 1) = EIGHT_BIT_MAXIMUM;
                        *depth.ptr(i + 1, j) = EIGHT_BIT_MAXIMUM;
                        *depth.ptr(i + 1, j + 1) = EIGHT_BIT_MAXIMUM;
                    
                    } else {
                        *depth.ptr(i, j) = ysrc[0];
                        *depth.ptr(i, j + 1) = ysrc[1];
                        *depth.ptr(i + 1, j) = ysrc[image->stride[0]];
                        *depth.ptr(i + 1, j + 1) = ysrc[image->stride[0] + 1];
                    }
                    ysrc += 2;
                }
                yline += image->stride[0] * 2;
                vline += image->stride[1];
            }
        }
    } else if (_depthCodec.name != 0) {
        vpx_codec_destroy(&_depthCodec);
        _depthCodec.name = 0;
    }
    QMetaObject::invokeMethod(this, "setFrame", Q_ARG(cv::Mat, color),
        Q_ARG(cv::Mat, depth), Q_ARG(float, aspectRatio));
    
    return dataBytes;
}

bool Face::render(float alpha) {
    if (!isActive()) {
        return false;
    }
    glPushMatrix();
    
    glTranslatef(_owningHead->getPosition().x, _owningHead->getPosition().y, _owningHead->getPosition().z);
    glm::quat orientation = _owningHead->getOrientation();
    glm::vec3 axis = glm::axis(orientation);
    glRotatef(glm::angle(orientation), axis.x, axis.y, axis.z);
        
    float aspect, xScale, zScale;
    if (_aspectRatio == FULL_FRAME_ASPECT) {
        aspect = _textureSize.width / _textureSize.height;
        const float FULL_FRAME_SCALE = 0.5f;
        xScale = FULL_FRAME_SCALE * _owningHead->getScale();
        zScale = xScale * 0.3f;
        
        glPushMatrix();
        glTranslatef(0.0f, -0.2f, 0.0f);
        glScalef(0.5f * xScale, xScale / aspect, zScale);
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        Application::getInstance()->getGeometryCache()->renderHalfCylinder(25, 20);
        glPopMatrix();
    } else {
        aspect = _aspectRatio;
        xScale = BODY_BALL_RADIUS_HEAD_BASE * _owningHead->getScale();
        zScale = xScale * 1.5f;
        glTranslatef(0.0f, -xScale * 0.75f, -xScale);
    }
    glScalef(xScale, xScale / aspect, zScale);
    
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    
    Point2f points[4];
    _textureRect.points(points);
    
    if (_depthTextureID != 0) {
        const int VERTEX_WIDTH = 100;
        const int VERTEX_HEIGHT = 100;
        const int VERTEX_COUNT = VERTEX_WIDTH * VERTEX_HEIGHT;
        const int ELEMENTS_PER_VERTEX = 2;
        const int BUFFER_ELEMENTS = VERTEX_COUNT * ELEMENTS_PER_VERTEX;
        const int QUAD_WIDTH = VERTEX_WIDTH - 1;
        const int QUAD_HEIGHT = VERTEX_HEIGHT - 1;
        const int QUAD_COUNT = QUAD_WIDTH * QUAD_HEIGHT;
        const int TRIANGLES_PER_QUAD = 2;   
        const int INDICES_PER_TRIANGLE = 3;
        const int INDEX_COUNT = QUAD_COUNT * TRIANGLES_PER_QUAD * INDICES_PER_TRIANGLE;
        
        if (_videoProgram == 0) {
            _videoProgram = loadProgram(QString(), "colorTexture", _videoProgramLocations);
            _texturedProgram = loadProgram("_textured", "permutationNormalTexture", _texturedProgramLocations);
            
            glGenBuffers(1, &_vboID);
            glBindBuffer(GL_ARRAY_BUFFER, _vboID);
            float* vertices = new float[BUFFER_ELEMENTS];
            float* vertexPosition = vertices;
            for (int i = 0; i < VERTEX_HEIGHT; i++) {
                for (int j = 0; j < VERTEX_WIDTH; j++) {
                    *vertexPosition++ = j / (float)(VERTEX_WIDTH - 1);
                    *vertexPosition++ = i / (float)(VERTEX_HEIGHT - 1);
                }
            }
            glBufferData(GL_ARRAY_BUFFER, BUFFER_ELEMENTS * sizeof(float), vertices, GL_STATIC_DRAW);
            delete[] vertices;
            
            glGenBuffers(1, &_iboID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboID);
            int* indices = new int[INDEX_COUNT];
            int* indexPosition = indices; 
            for (int i = 0; i < QUAD_HEIGHT; i++) {
                for (int j = 0; j < QUAD_WIDTH; j++) {
                    *indexPosition++ = i * VERTEX_WIDTH + j;
                    *indexPosition++ = (i + 1) * VERTEX_WIDTH + j;
                    *indexPosition++ = i * VERTEX_WIDTH + j + 1;
                    
                    *indexPosition++ = i * VERTEX_WIDTH + j + 1;
                    *indexPosition++ = (i + 1) * VERTEX_WIDTH + j;
                    *indexPosition++ = (i + 1) * VERTEX_WIDTH + j + 1;
                }
            }
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_COUNT * sizeof(int), indices, GL_STATIC_DRAW);
            delete[] indices;
            
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, _vboID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboID);
        }
        glBindTexture(GL_TEXTURE_2D, _depthTextureID);
        
        glActiveTexture(GL_TEXTURE1);
        
        ProgramObject* program = _videoProgram;
        Locations* locations = &_videoProgramLocations;
        if (_colorTextureID != 0) {
            glBindTexture(GL_TEXTURE_2D, _colorTextureID);        
        
        } else {
            glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getPermutationNormalTextureID());
            program = _texturedProgram;
            locations = &_texturedProgramLocations;
        }
        program->bind();
        program->setUniformValue(locations->texCoordCorner, 
            points[0].x / _textureSize.width, points[0].y / _textureSize.height);
        program->setUniformValue(locations->texCoordRight,
            (points[3].x - points[0].x) / _textureSize.width, (points[3].y - points[0].y) / _textureSize.height);
        program->setUniformValue(locations->texCoordUp,
            (points[1].x - points[0].x) / _textureSize.width, (points[1].y - points[0].y) / _textureSize.height);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, 0);
        
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_EQUAL, 1.0f);
        
        if (_renderMode == MESH) {
            glDrawRangeElementsEXT(GL_TRIANGLES, 0, VERTEX_COUNT - 1, INDEX_COUNT, GL_UNSIGNED_INT, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
        } else { // _renderMode == POINTS
            glPointSize(5.0f);
            glDrawArrays(GL_POINTS, 0, VERTEX_COUNT);
            glPointSize(1.0f);
        }
        
        glDisable(GL_ALPHA_TEST);
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        program->release();
        
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    
    } else {
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        glEnable(GL_TEXTURE_2D);
        
        glBegin(GL_QUADS);
        glTexCoord2f(points[0].x / _textureSize.width, points[0].y / _textureSize.height);
        glVertex3f(0.5f, -0.5f, 0.0f);    
        glTexCoord2f(points[1].x / _textureSize.width, points[1].y / _textureSize.height);
        glVertex3f(0.5f, 0.5f, 0.0f);
        glTexCoord2f(points[2].x / _textureSize.width, points[2].y / _textureSize.height);
        glVertex3f(-0.5f, 0.5f, 0.0f);
        glTexCoord2f(points[3].x / _textureSize.width, points[3].y / _textureSize.height);
        glVertex3f(-0.5f, -0.5f, 0.0f);
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glPopMatrix();
    
    return true;
}

void Face::cycleRenderMode() {
    _renderMode = (RenderMode)((_renderMode + 1) % RENDER_MODE_COUNT);    
}

void Face::setFrame(const cv::Mat& color, const cv::Mat& depth, float aspectRatio) {
    Size2f textureSize = _textureSize;
    if (!color.empty()) {
        bool generate = (_colorTextureID == 0);
        if (generate) {
            glGenTextures(1, &_colorTextureID);
        }
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        if (_textureSize.width != color.cols || _textureSize.height != color.rows || generate) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, color.cols, color.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, color.ptr());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            textureSize = color.size();
            _textureRect = RotatedRect(Point2f(color.cols * 0.5f, color.rows * 0.5f), textureSize, 0.0f);
        
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, color.cols, color.rows, GL_RGB, GL_UNSIGNED_BYTE, color.ptr());
        }
    } else if (_colorTextureID != 0) {
        glDeleteTextures(1, &_colorTextureID);
        _colorTextureID = 0;
    }
    
    if (!depth.empty()) {
        bool generate = (_depthTextureID == 0);
        if (generate) {
            glGenTextures(1, &_depthTextureID);
        }
        glBindTexture(GL_TEXTURE_2D, _depthTextureID);
        if (_textureSize.width != depth.cols || _textureSize.height != depth.rows || generate) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, depth.cols, depth.rows, 0,
                GL_LUMINANCE, GL_UNSIGNED_BYTE, depth.ptr());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            textureSize = depth.size();
            _textureRect = RotatedRect(Point2f(depth.cols * 0.5f, depth.rows * 0.5f), textureSize, 0.0f);
            
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depth.cols, depth.rows, GL_LUMINANCE, GL_UNSIGNED_BYTE, depth.ptr());
        }
    } else if (_depthTextureID != 0) {
        glDeleteTextures(1, &_depthTextureID);
        _depthTextureID = 0;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    
    _aspectRatio = aspectRatio;
    _textureSize = textureSize;
}

void Face::destroyCodecs() {
    if (_colorCodec.name != 0) {
        vpx_codec_destroy(&_colorCodec);
        _colorCodec.name = 0;
    }
    if (_depthCodec.name != 0) {
        vpx_codec_destroy(&_depthCodec);
        _depthCodec.name = 0;
    }
}

ProgramObject* Face::loadProgram(const QString& suffix, const char* secondTextureUniform, Locations& locations) {
    ProgramObject* program = new ProgramObject();
    program->addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/face" + suffix + ".vert");
    program->addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/face" + suffix + ".frag");
    program->link();
    
    program->bind();
    program->setUniformValue("depthTexture", 0);
    program->setUniformValue(secondTextureUniform, 1);
    program->release();
    
    locations.texCoordCorner = program->uniformLocation("texCoordCorner");
    locations.texCoordRight = program->uniformLocation("texCoordRight");
    locations.texCoordUp = program->uniformLocation("texCoordUp");
    
    return program;
}
