//
//  PerlinFace.cpp
//  interface
//
//  Created by Clément Brisset on 9/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "PerlinFace.h"
#include "Head.h"
#include "Avatar.h"
#include "Application.h"

// The following points only reference the left part of the face.
enum {
    BROW_RIGHT_TOP = 0, BROW_MID_TOP, BROW_LEFT, BROW_MID_BOTTOM, BROW_RIGHT_BOTTOM,
    EYE_RIGHT, EYE_MID_TOP, EYE_LEFT, EYE_MID_BOTTOM,
    EYEBALL_BOTTOM, EYEBALL_TOP, EYEBALL_MID,
    NOSTRIL_TOP, NOSTRIL_LEFT, NOSTRIL_BOTTOM, NOSE_LEFT, NOSE_RIGHT,
    MOUTH_TOP_OUT, MOUTH_MID_OUT, MOUTH_BOTTOM_OUT,
    MOUTH_TOP_IN, MOUTH_MID_IN, MOUTH_BOTTOM_IN,
    EAR_CENTER, EAR_BOTTOM_RIGHT, EAR_BOTTOM_LEFT, EAR_TOP_RIGHT, EAR_TOP_MID, EAR_TOP_LEFT, EAR_BACK,
    CHIN_BOTTOM, CHIN_TIP, CHIN_IN,
    CHICK_LEFT, CHICK_MID, CHICK_RIGHT,
    FOREHEAD_RIGHT, FOREHEAD_LEFT,
    JAW_BOTTOM, JAW_LEFT, JAW_BOTTOM_LEFT,
    HAIR_SIDE_1, HAIR_SIDE_2, HAIR_SIDE_3, HAIR_SIDE_4, HAIR_SIDE_5,
    HAIR_1, HAIR_2, HAIR_3, HAIR_4, HAIR_5, HAIR_6, HAIR_7, HAIR_8, HAIR_9,

    NUM_VERTICES
};

static const float VERTICES[]
= { -5,70,44,  -12,72,42,  -22,68,35,  -12,70,42,  -5,68,44.5,   // left brow
    -6,60,43.7,  -12,62,44,  -19,60,38,  -12,58,43,              // left eye
    -12,60,37,  -14.1,62.1,37,  -13.15,61.25,32,                 // left eyeball
    -2,45,52,  -5,44,47,  -1,42.5,49,  -4,47,48,  -2,48.5,52.5,  // left part nose
    -4,38,50,  -8,30.2,46,  -3,23.5,46,                          // left out mouse
    -2.1,35,48,  -5,31,47,  -1.5,29,47,                          // left in mouse
    -26.5,55,14,  -23.3,51.3,7,  -26.5,50.7,7,  -23,59,9,   -26,61,7,  -27.6,59.5,7,  -13,50,0,  // left ear
    -6.1,16.8,44.5,  -5,20.7,46.5,  -3,23,45,                    // left chin
    -22,51,37,  -15,36,39,  -13.6,26,38,                         // left chick
    -10,84,34,  -22,80,24,                                       // left forehead
    -6,22,20,  -22,35,15,  -19,31.5,10,                          // left jaw
    -14,99,0,  -27,85,0,  -30,70,0,  -30,52,-4,  -28,30,0,       // left hair side
    // left hair line
    -16,86,39,  -12,95,27,  -12,99,13,  -12,97,-5,  -15,91,-17,  -18,80,-26,  -16,60,-29,  -10,40,-23,  -15,30,-12,

    5,70,44,  12,72,42,  22,68,35,  12,70,42,  5,68,44.5,   //  0-4  right brow
    6,60,43.7,  12,62,44,  19,60,38,  12,58,43,             //  5-8  right eye
    12,60,37,  14.1,62.1,37,  13.15,61.15,32,               //  9-11 right eyeball
    2,45,52,  5,44,47,  1,42.5,49,  4,47,48,  2,48.5,52.5,  // 12-16 right part nose
    4,38,50,  8,30.2,46,  3,23.5,46,                        // 17-19 right out mouse
    2.1,35,48,  5,31,47,  1.5,29,47,                        // 20-22 right in mouse
    26.5,55,14,  23.3,51.3,7,  26.5,50.7,7,  23,59,9,   26,61,7,  27.6,59.5,7,  13,50,0,  // 23-29 right ear
    6.1,16.8,44.5,  5,20.7,46.5,  3,23,45,                  // 30-32 right chin
    22,51,37,  15,36,39,  13.6,26,38,                       // 33-35 right chick
    10,84,34,  22,80,24,                                    // 36-37 right forehead
    6,22,20,  22,35,15,  19,31.5,10,                        // 38-40 right jaw
    14,99,0,  27,85,0, 30,70,0,  30,52,-4, 28,30,0,         // 41-45 right hair side
    // 45-53 right hair line
    16,86,39,  12,95,27,  12,99,13,  12,97,-5,  15,91,-17,  18,80,-26,  16,60,-29,  10,40,-23,  15,30,-12,

    // Not used at the moment.
    7,34,44,  7,29.2,42,          // teeth
    16,36,33,  16,16,0,  40,7,0,  40,0,0,  20,-40,-23}; // shoulders, back, misc

const int TRIANGLES_NUMBER = 206;
const int VERTEX_PER_TRIANGLE = 3;
const int FLOAT_PER_VERTEX = 3;

const GLfloat NO_SPECULAR_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };

glm::vec3* PerlinFace::_vertices = NULL;
GLfloat* PerlinFace::_triangles = NULL;
GLfloat* PerlinFace::_normals = NULL;
GLubyte* PerlinFace::_colors = NULL;

PerlinFace::PerlinFace(Head *owningHead)
    : _owningHead(owningHead),
      _initialized(false),
      _vertexNumber(0),
      _trianglesCount(0),
      _oldNormals(NULL),
      _newNormals(NULL),
      _trianglesPos(NULL),
      _normalsPos(NULL),
      _colorsPos(NULL),
      _vboID(0),
      _nboID(0),
      _cboID(0),
      _browsD_L(0),
      _browsD_R(0),
      _browsU_C(0),
      _browsU_L(0),
      _browsU_R(0),
      _mouthSize(0),
      _mouthSmileLeft(0),
      _mouthSmileRight(0),
      _eyeBlink_L(0),
      _eyeBlink_R(0),
      _eyeOpen_L(0),
      _eyeOpen_R(0) {
}

PerlinFace::~PerlinFace() {
    if (_initialized) {
        glDeleteBuffers(1, &_vboID);
        glDeleteBuffers(1, &_nboID);
        glDeleteBuffers(1, &_cboID);
        delete[] _oldNormals;
        delete[] _newNormals;
    }
}

void staticCleanup() {
    delete[] PerlinFace::_vertices;
    delete[] PerlinFace::_triangles;
    delete[] PerlinFace::_normals;
    delete[] PerlinFace::_colors;
}

void PerlinFace::init() {
    if (_initialized) {
        return;
    }
    glGenBuffers(1, &_vboID);
    glGenBuffers(1, &_nboID);
    glGenBuffers(1, &_cboID);

    _vertexNumber = sizeof(VERTICES) / (FLOAT_PER_VERTEX * sizeof(float));

    _oldNormals = new glm::vec3[_vertexNumber];
    _newNormals = new glm::vec3[_vertexNumber];

    if (_vertices == NULL) {
        _vertices = new glm::vec3[_vertexNumber];
        for (int i = 0; i < _vertexNumber; ++i) {
            _vertices[i] = getVec3(i);
        }

        _trianglesPos = _triangles = new GLfloat[FLOAT_PER_VERTEX * VERTEX_PER_TRIANGLE * TRIANGLES_NUMBER];
        _normalsPos = _normals = new GLfloat[FLOAT_PER_VERTEX * VERTEX_PER_TRIANGLE  * TRIANGLES_NUMBER];
        _colorsPos = _colors = new GLubyte[FLOAT_PER_VERTEX * VERTEX_PER_TRIANGLE  * TRIANGLES_NUMBER];

        atexit(staticCleanup);
    }

    _initialized = true;
}

void PerlinFace::update() {
    if (!_initialized) {
        init();
    }

    updatePositions();
    updateVertices();

    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferData(GL_ARRAY_BUFFER,
                 FLOAT_PER_VERTEX * VERTEX_PER_TRIANGLE  * _trianglesCount * sizeof(GLfloat),
                 _triangles,
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, _nboID);
    glBufferData(GL_ARRAY_BUFFER,
                 FLOAT_PER_VERTEX * VERTEX_PER_TRIANGLE  * _trianglesCount * sizeof(GLfloat),
                 _normals,
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, _cboID);
    glBufferData(GL_ARRAY_BUFFER,
                 FLOAT_PER_VERTEX * VERTEX_PER_TRIANGLE  * _trianglesCount * sizeof(GLubyte),
                 _colors,
                 GL_DYNAMIC_DRAW);
}

#include <renderer/ProgramObject.h>

void PerlinFace::render() {
    glPushMatrix();

    update();
//    // Jump to head position, orientation and scale
//    glm::quat orientation = _owningHead->getOrientation();
//    glm::vec3 axis = glm::axis(orientation);
//    glTranslatef(_owningHead->getPosition().x, _owningHead->getPosition().y, _owningHead->getPosition().z);
//    glScalef(_owningHead->getScale(), _owningHead->getScale(), _owningHead->getScale());
//    glRotatef(glm::angle(orientation), axis.x, axis.y, axis.z);


    glPushMatrix();
    // Correct head scale and offset from hard coded points coordinates.
    glScalef(2.0f * BODY_BALL_RADIUS_HEAD_BASE / (_vertices[HAIR_2].y - _vertices[JAW_BOTTOM].y),
             2.0f * BODY_BALL_RADIUS_HEAD_BASE / (_vertices[HAIR_2].y - _vertices[JAW_BOTTOM].y),
             2.0f * BODY_BALL_RADIUS_HEAD_BASE / (_vertices[HAIR_2].y - _vertices[JAW_BOTTOM].y));
    glTranslatef(0, -60.0f, 20.0f);

    /*/
    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glVertexPointer(FLOAT_PER_VERTEX, GL_FLOAT, 0, 0);

    glEnableClientState(GL_NORMAL_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, _nboID);
    glNormalPointer(GL_FLOAT, 0, 0);

    glEnableClientState(GL_COLOR_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, _cboID);
    glColorPointer(3, GL_UNSIGNED_BYTE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, VERTEX_PER_TRIANGLE * _trianglesCount);

    // Draw eyes
    glColor3d(0, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex3d(_vertices[EYE_LEFT].x, _vertices[EYE_LEFT].y, _vertices[EYE_LEFT].z);
    glVertex3d(_vertices[EYE_MID_TOP].x, _vertices[EYE_MID_TOP].y, _vertices[EYE_MID_TOP].z);
    glVertex3d(_vertices[EYE_RIGHT].x, _vertices[EYE_RIGHT].y, _vertices[EYE_RIGHT].z);
    glVertex3d(_vertices[EYE_MID_BOTTOM].x, _vertices[EYE_MID_BOTTOM].y, _vertices[EYE_MID_BOTTOM].z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3d(_vertices[NUM_VERTICES + EYE_LEFT].x,
               _vertices[NUM_VERTICES + EYE_LEFT].y,
               _vertices[NUM_VERTICES + EYE_LEFT].z);
    glVertex3d(_vertices[NUM_VERTICES + EYE_MID_TOP].x,
               _vertices[NUM_VERTICES + EYE_MID_TOP].y,
               _vertices[NUM_VERTICES + EYE_MID_TOP].z);
    glVertex3d(_vertices[NUM_VERTICES + EYE_RIGHT].x,
               _vertices[NUM_VERTICES + EYE_RIGHT].y,
               _vertices[NUM_VERTICES + EYE_RIGHT].z);
    glVertex3d(_vertices[NUM_VERTICES + EYE_MID_BOTTOM].x,
               _vertices[NUM_VERTICES + EYE_MID_BOTTOM].y,
               _vertices[NUM_VERTICES + EYE_MID_BOTTOM].z);
    glEnd();
    glPopMatrix();

    const float EYEBALL_RADIUS           =  0.008f;
    const float EYEBALL_COLOR[4]         =  { 0.9f, 0.9f, 0.8f, 1.0f };
    const float IRIS_RADIUS              =  0.0035;
    const float IRIS_PROTRUSION          =  0.0065f;
    // render white ball of left eyeball

    glm::vec3 eyePos = glm::vec3(0.024f, 0.0f, -0.032f);


    Head::_irisProgram.bind();
    glBindTexture(GL_TEXTURE_2D, Head::_irisTextureID);
    glEnable(GL_TEXTURE_2D);

    orientation = _owningHead->getOrientation();
    glm::vec3 front = orientation * IDENTITY_FRONT;

    // render left iris
    glm::quat leftIrisRotation;
    glPushMatrix(); {
        glTranslatef(eyePos.x, eyePos.y, eyePos.z); //translate to eyeball position

        //rotate the eyeball to aim towards the lookat position
        glm::vec3 targetLookatVector = _owningHead->_lookAtPosition - eyePos;
        leftIrisRotation = rotationBetween(front, targetLookatVector) * orientation;
        glm::vec3 rotationAxis = glm::axis(leftIrisRotation);
        glRotatef(glm::angle(leftIrisRotation), rotationAxis.x, rotationAxis.y, rotationAxis.z);
        glTranslatef(0.0f, 0.0f, IRIS_PROTRUSION);
        glScalef(IRIS_RADIUS * 2.0f,
                 IRIS_RADIUS * 2.0f,
                 IRIS_RADIUS); // flatten the iris

        // this ugliness is simply to invert the model transform and get the eye position in model space
        Head::_irisProgram.setUniform(Head::_eyePositionLocation, (glm::inverse(leftIrisRotation) *
            (Application::getInstance()->getCamera()->getPosition() - eyePos) +
                glm::vec3(0.0f, 0.0f, IRIS_PROTRUSION)) * glm::vec3(1.0f / (IRIS_RADIUS * 2.0f),
                    1.0f / (IRIS_RADIUS * 2.0f), 1.0f / IRIS_RADIUS));

        glutSolidSphere(0.5f, 15, 15);
    }
    glPopMatrix();

    eyePos.x = - eyePos.x;

    // render right iris
    glm::quat rightIrisRotation;
    glPushMatrix(); {
        glTranslatef(eyePos.x, eyePos.y, eyePos.z); //translate to eyeball position

        //rotate the eyeball to aim towards the lookat position
        glm::vec3 targetLookatVector = _owningHead->_lookAtPosition. - eyePos;
        rightIrisRotation = rotationBetween(front, targetLookatVector) * orientation;
        glm::vec3 rotationAxis = glm::axis(rightIrisRotation);
        glRotatef(glm::angle(rightIrisRotation), rotationAxis.x, rotationAxis.y, rotationAxis.z);
        glTranslatef(0.0f, 0.0f, IRIS_PROTRUSION);
        glScalef(IRIS_RADIUS * 2.0f,
                 IRIS_RADIUS * 2.0f,
                 IRIS_RADIUS); // flatten the iris

        // this ugliness is simply to invert the model transform and get the eye position in model space
        Head::_irisProgram.setUniform(Head::_eyePositionLocation, (glm::inverse(rightIrisRotation) *
            (Application::getInstance()->getCamera()->getPosition() - eyePos) +
                glm::vec3(0.0f, 0.0f, IRIS_PROTRUSION)) * glm::vec3(1.0f / (IRIS_RADIUS * 2.0f),
                    1.0f / (IRIS_RADIUS * 2.0f), 1.0f / IRIS_RADIUS));

        glutSolidSphere(0.5f, 15, 15);
    }
    glPopMatrix();

    Head::_irisProgram.release();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);



    glPushMatrix();
        glColor4fv(EYEBALL_COLOR);
        glTranslatef(eyePos.x, eyePos.y, eyePos.z);
        glutSolidSphere(EYEBALL_RADIUS, 30, 30);
    glPopMatrix();

    //render white ball of right eyeball
    glPushMatrix();
        glColor4fv(EYEBALL_COLOR);
        glTranslatef(-eyePos.x, eyePos.y, eyePos.z);
        glutSolidSphere(EYEBALL_RADIUS, 30, 30);
    glPopMatrix();

    /**/

    /*/
    // Draw points for debug.
    glColor3d(0.0f, 1.0f, 0.0f);
    glPointSize(4);
    glBegin(GL_POINTS);
    for (int i = 0; i < 2 * NUM_VERTICES; ++i) {
        if (i == NUM_VERTICES) {
            glColor3d(1.0f, 0.0f, 0.0f);
        }
        glVertex3d(VERTICES[3 * i], VERTICES[(3 * i) + 1], VERTICES[(3 * i) + 2]);
    }
    glColor3d(0.0f, 0.0f, 1.0f);
    for (int i = 2 * NUM_VERTICES; i < _vertexNumber; ++i) {
        break;
        glVertex3d(VERTICES[3 * i], VERTICES[(3 * i) + 1], VERTICES[(3 * i) + 2]);
        glVertex3d(-VERTICES[3 * i], VERTICES[(3 * i) + 1], VERTICES[(3 * i) + 2]);
    }
    glEnd();
    /**/
    glPopMatrix();
}

#include "Application.h"

void PerlinFace::updatePositions() {
    const float BROWS_UP_MAX = 3;
    const float BROWS_DOWN_MAX = 1;
    const float BROWS_UP_CENTER_MAX = 1;

    Faceshift* faceshift = Application::getInstance()->getFaceshift();
    if (faceshift->isActive()) {
//        _browsD_L = faceshift->getBrowDownLeft();
//        _browsD_R = faceshift->getBrowDownRight();
//        _browsU_C = faceshift->getBrowUpCenter();
//        _browsU_L = faceshift->getBrowUpLeft();
//        _browsU_R = faceshift->getBrowUpRight();
//        _mouthSize = faceshift->getMouthSize();
//        _mouthSmileLeft = faceshift->getMouthSmileLeft();
//        _mouthSmileRight = faceshift->getMouthSmileRight();
//        _eyeBlink_L = faceshift->getLeftBlink();
//        _eyeBlink_R = faceshift->getRightBlink();
//        _eyeOpen_L = faceshift->getLeftEyeOpen();
//        _eyeOpen_R = faceshift->getRightEyeOpen();
    }


    // Update left brow
    _vertices[BROW_LEFT].y = getVec3(BROW_LEFT).y
            + _browsU_L * BROWS_UP_MAX
            - _browsD_L * BROWS_DOWN_MAX
            - _browsU_C * BROWS_UP_CENTER_MAX;
    _vertices[BROW_MID_TOP].y = getVec3(BROW_MID_TOP).y
            + _browsU_L * BROWS_UP_MAX
            - _browsD_L * BROWS_DOWN_MAX;
    _vertices[BROW_MID_BOTTOM].y = getVec3(BROW_MID_BOTTOM).y
            + _browsU_L * BROWS_UP_MAX
            - _browsD_L * BROWS_DOWN_MAX;
    _vertices[BROW_RIGHT_TOP].y = getVec3(BROW_RIGHT_TOP).y
            + _browsU_L * BROWS_UP_MAX
            - _browsD_L * BROWS_DOWN_MAX
            + _browsU_C * BROWS_UP_CENTER_MAX;
    _vertices[BROW_RIGHT_BOTTOM].y = getVec3(BROW_RIGHT_BOTTOM).y
            + _browsU_L * BROWS_UP_MAX
            - _browsD_L * BROWS_DOWN_MAX
            + _browsU_C * BROWS_UP_CENTER_MAX;

    // Update right brow
    _vertices[NUM_VERTICES + BROW_LEFT].y = getVec3(NUM_VERTICES + BROW_LEFT).y
            + _browsU_R * BROWS_UP_MAX
            - _browsD_R * BROWS_DOWN_MAX
            - _browsU_C * BROWS_UP_CENTER_MAX;
    _vertices[NUM_VERTICES + BROW_MID_TOP].y = getVec3(NUM_VERTICES + BROW_MID_TOP).y
            + _browsU_R * BROWS_UP_MAX
            - _browsD_R * BROWS_DOWN_MAX;
    _vertices[NUM_VERTICES + BROW_MID_BOTTOM].y = getVec3(NUM_VERTICES + BROW_MID_BOTTOM).y
            + _browsU_R * BROWS_UP_MAX
            - _browsD_R * BROWS_DOWN_MAX;
    _vertices[NUM_VERTICES + BROW_RIGHT_TOP].y = getVec3(NUM_VERTICES + BROW_RIGHT_TOP).y
            + _browsU_R * BROWS_UP_MAX
            - _browsD_R * BROWS_DOWN_MAX
            + _browsU_C * BROWS_UP_CENTER_MAX;
    _vertices[NUM_VERTICES + BROW_RIGHT_BOTTOM].y = getVec3(NUM_VERTICES + BROW_RIGHT_BOTTOM).y
            + _browsU_R * BROWS_UP_MAX
            - _browsD_R * BROWS_DOWN_MAX
            + _browsU_C * BROWS_UP_CENTER_MAX;


    // Mouth
    _vertices[MOUTH_BOTTOM_IN].y = getVec3(MOUTH_BOTTOM_IN).y
            + (1.0 - _mouthSize) * 6.5;
    _vertices[MOUTH_BOTTOM_OUT].y = getVec3(MOUTH_BOTTOM_OUT).y
            + (1.0 - _mouthSize) * 6.5;
    _vertices[MOUTH_MID_IN] = (1.0f - (_mouthSmileLeft / 3.0f)) * (getVec3(MOUTH_MID_IN)
                                                                   + glm::vec3(0, (1.0 - _mouthSize) * 4, 0))
            + (_mouthSmileLeft / 3.0f) * getVec3(CHICK_MID);
    _vertices[MOUTH_MID_OUT] = (1.0f - (_mouthSmileLeft / 3.0f)) * (getVec3(MOUTH_MID_OUT)
                                                                    + glm::vec3(0, (1.0 - _mouthSize) * 4, 0))
            + (_mouthSmileLeft / 3.0f) * getVec3(CHICK_MID);


    _vertices[NUM_VERTICES + MOUTH_BOTTOM_IN].y = getVec3(NUM_VERTICES + MOUTH_BOTTOM_IN).y
            + (1.0 - _mouthSize) * 6.5;
    _vertices[NUM_VERTICES + MOUTH_BOTTOM_OUT].y = getVec3(NUM_VERTICES + MOUTH_BOTTOM_OUT).y
            + (1.0 - _mouthSize) * 6.5;
    _vertices[NUM_VERTICES + MOUTH_MID_IN] = (1.0f - (_mouthSmileLeft / 3.0f)) * (getVec3(NUM_VERTICES + MOUTH_MID_IN)
                                                                   + glm::vec3(0, (1.0 - _mouthSize) * 4, 0))
            + (_mouthSmileLeft / 3.0f) * getVec3(NUM_VERTICES + CHICK_MID);
    _vertices[NUM_VERTICES + MOUTH_MID_OUT] = (1.0f - (_mouthSmileLeft / 3.0f)) * (getVec3(NUM_VERTICES + MOUTH_MID_OUT)
                                                                    + glm::vec3(0, (1.0 - _mouthSize) * 4, 0))
            + (_mouthSmileLeft / 3.0f) * getVec3(NUM_VERTICES + CHICK_MID);


    
    // Jaw
    _vertices[CHIN_IN].y = getVec3(CHIN_IN).y
            + (1.0 - _mouthSize) * 4;
    _vertices[CHIN_TIP].y = getVec3(CHIN_TIP).y
            + (1.0 - _mouthSize) * 4;
    _vertices[CHIN_BOTTOM].y = getVec3(CHIN_BOTTOM).y
            + (1.0 - _mouthSize) * 4;

    _vertices[NUM_VERTICES +CHIN_IN].y = getVec3(NUM_VERTICES + CHIN_IN).y
            + (1.0 - _mouthSize) * 4;
    _vertices[NUM_VERTICES +CHIN_TIP].y = getVec3(NUM_VERTICES + CHIN_TIP).y
            + (1.0 - _mouthSize) * 4;
    _vertices[NUM_VERTICES +CHIN_BOTTOM].y = getVec3(NUM_VERTICES + CHIN_BOTTOM).y
            + (1.0 - _mouthSize) * 4;


    // Eyelids
    glm::vec3 topLeftEyelid = getVec3(EYE_MID_TOP);
    glm::vec3 bottomLeftEyelid = getVec3(EYE_MID_BOTTOM);
    glm::vec3 topRightEyelid = getVec3(NUM_VERTICES + EYE_MID_TOP);
    glm::vec3 bottomRightEyelid = getVec3(NUM_VERTICES + EYE_MID_BOTTOM);
    
    _vertices[EYE_MID_TOP] = (1.0f - (_eyeBlink_L + _eyeOpen_L / 2.0f)) * topLeftEyelid
    + (_eyeBlink_L + _eyeOpen_L / 2.0f) * (topLeftEyelid + bottomLeftEyelid) / 2.0f;
    _vertices[EYE_MID_BOTTOM] = (1.0f - (_eyeBlink_L + _eyeOpen_L / 2.0f)) * bottomLeftEyelid
    + (_eyeBlink_L + _eyeOpen_L / 2.0f) * (topLeftEyelid + bottomLeftEyelid) / 2.0f;
    
    _vertices[NUM_VERTICES + EYE_MID_TOP] = (1.0f - (_eyeBlink_R + _eyeOpen_R / 2.0f)) * topRightEyelid
    + (_eyeBlink_R + _eyeOpen_R / 2.0f) * (topRightEyelid + bottomRightEyelid) / 2.0f;
    _vertices[NUM_VERTICES + EYE_MID_BOTTOM] = (1.0f - (_eyeBlink_R + _eyeOpen_R / 2.0f)) * bottomRightEyelid
    + (_eyeBlink_R + _eyeOpen_R / 2.0f) * (topRightEyelid + bottomRightEyelid) / 2.0f;
}

void PerlinFace::updateVertices() {
    const GLubyte SKIN_COLOR[] = {255, 214, 163};
    const GLubyte MOUSE_COLOR[] = {200, 159, 108};
    const GLubyte HAIR_COLOR[] = {0, 0, 0};

    _trianglesCount = 0;
    _trianglesPos = _triangles;
    _normalsPos = _normals;
    _colorsPos = _colors;

    std::swap(_oldNormals, _newNormals);

    // Brows
    addTriangles(BROW_LEFT, BROW_MID_TOP, BROW_MID_BOTTOM, 0, 0, 0);
    addTriangles(BROW_MID_BOTTOM, BROW_MID_TOP, BROW_RIGHT_TOP, 0, 0, 0);
    addTriangles(BROW_MID_BOTTOM, BROW_RIGHT_TOP, BROW_RIGHT_BOTTOM, 0, 0, 0);

    // Forehead
    addTriangles(FOREHEAD_LEFT, FOREHEAD_RIGHT, BROW_MID_TOP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(FOREHEAD_LEFT, BROW_MID_TOP, BROW_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(FOREHEAD_RIGHT, BROW_RIGHT_TOP, BROW_MID_TOP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addJunction(FOREHEAD_RIGHT, BROW_RIGHT_TOP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addJunction(BROW_RIGHT_TOP, BROW_RIGHT_BOTTOM, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);

    // Eyes
    addTriangles(BROW_LEFT, BROW_MID_BOTTOM, EYE_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EYE_LEFT, BROW_MID_BOTTOM, EYE_MID_TOP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(BROW_MID_BOTTOM, BROW_RIGHT_BOTTOM, EYE_MID_TOP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EYE_MID_TOP, BROW_RIGHT_BOTTOM, EYE_RIGHT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addJunction(BROW_RIGHT_BOTTOM, EYE_RIGHT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);

    // Temp eye draw
    addTriangles(EYE_LEFT, EYE_MID_TOP, EYEBALL_MID, 255, 255, 255);
    addTriangles(EYE_MID_TOP, EYE_RIGHT, EYEBALL_MID, 255, 255, 255);
    addTriangles(EYE_RIGHT, EYE_MID_BOTTOM, EYEBALL_MID, 255, 255, 255);
    addTriangles(EYE_MID_BOTTOM, EYE_LEFT, EYEBALL_MID, 255, 255, 255);

    // Chick
    addTriangles(EAR_CENTER, FOREHEAD_LEFT, BROW_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(BROW_LEFT, CHICK_LEFT, EAR_CENTER, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(BROW_LEFT, EYE_LEFT, CHICK_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EYE_LEFT, EYE_MID_BOTTOM, CHICK_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EYE_MID_BOTTOM, EYE_RIGHT, NOSTRIL_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(CHICK_LEFT, EYE_MID_BOTTOM, NOSTRIL_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(CHICK_LEFT, NOSTRIL_LEFT, CHICK_MID, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(CHICK_MID, NOSTRIL_LEFT, MOUTH_TOP_OUT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(CHICK_MID, MOUTH_TOP_OUT, MOUTH_MID_OUT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(CHICK_MID, MOUTH_MID_OUT, CHIN_TIP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(CHICK_MID, CHIN_TIP, CHICK_RIGHT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(CHICK_RIGHT, CHIN_TIP, CHIN_BOTTOM, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(CHICK_LEFT, CHICK_MID, CHICK_RIGHT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EAR_CENTER, CHICK_LEFT, JAW_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(JAW_LEFT, CHICK_LEFT, CHICK_RIGHT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);


    // Nose
    addTriangles(EYE_RIGHT, NOSE_LEFT, NOSTRIL_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EYE_RIGHT, NOSE_RIGHT, NOSE_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(NOSE_LEFT, NOSE_RIGHT, NOSTRIL_TOP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addJunction(EYE_RIGHT, NOSE_RIGHT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addJunction(NOSE_RIGHT, NOSTRIL_TOP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);

    addTriangles(NOSTRIL_LEFT, NOSE_LEFT, NOSTRIL_TOP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(NOSTRIL_LEFT, NOSTRIL_TOP, NOSTRIL_BOTTOM, 0, 0, 0);
    addJunction(NOSTRIL_TOP, NOSTRIL_BOTTOM, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);

    addTriangles(NOSTRIL_LEFT, NOSTRIL_BOTTOM, MOUTH_TOP_OUT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addJunction(NOSTRIL_BOTTOM, MOUTH_TOP_OUT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);

    // Mouth
    addTriangles(MOUTH_TOP_OUT, MOUTH_TOP_IN, MOUTH_MID_OUT, MOUSE_COLOR[0], MOUSE_COLOR[1], MOUSE_COLOR[2]);
    addTriangles(MOUTH_TOP_IN, MOUTH_MID_IN, MOUTH_MID_OUT, MOUSE_COLOR[0], MOUSE_COLOR[1], MOUSE_COLOR[2]);
    addTriangles(MOUTH_MID_OUT, MOUTH_MID_IN, MOUTH_BOTTOM_OUT, MOUSE_COLOR[0], MOUSE_COLOR[1], MOUSE_COLOR[2]);
    addTriangles(MOUTH_MID_IN, MOUTH_BOTTOM_IN, MOUTH_BOTTOM_OUT, MOUSE_COLOR[0], MOUSE_COLOR[1], MOUSE_COLOR[2]);
    addJunction(MOUTH_TOP_OUT, MOUTH_TOP_IN, MOUSE_COLOR[0], MOUSE_COLOR[1], MOUSE_COLOR[2]);
    addJunction(MOUTH_BOTTOM_OUT, MOUTH_BOTTOM_IN, MOUSE_COLOR[0], MOUSE_COLOR[1], MOUSE_COLOR[2]);

    addTriangles(MOUTH_MID_IN, MOUTH_TOP_IN, MOUTH_BOTTOM_IN, 0, 0, 0);
    addJunction(MOUTH_TOP_IN, MOUTH_BOTTOM_IN, 0, 0, 0);


    // Chin
    addTriangles(MOUTH_MID_OUT, MOUTH_BOTTOM_OUT, CHIN_IN, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(MOUTH_MID_OUT, CHIN_IN, CHIN_TIP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);

    addJunction(MOUTH_BOTTOM_OUT, CHIN_IN, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addJunction(CHIN_IN, CHIN_TIP, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addJunction(CHIN_TIP, CHIN_BOTTOM, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);

    // Jaw
    addTriangles(JAW_BOTTOM_LEFT, JAW_LEFT, CHICK_RIGHT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(JAW_LEFT, JAW_BOTTOM_LEFT, EAR_CENTER, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(JAW_BOTTOM_LEFT, CHICK_RIGHT, CHIN_BOTTOM, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(JAW_BOTTOM_LEFT, CHIN_BOTTOM, JAW_BOTTOM, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addJunction(CHIN_BOTTOM, JAW_BOTTOM, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);

    // Ear
    addTriangles(FOREHEAD_LEFT, EAR_CENTER, EAR_TOP_RIGHT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(JAW_BOTTOM_LEFT, EAR_BOTTOM_RIGHT, EAR_CENTER, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EAR_BOTTOM_RIGHT, EAR_TOP_RIGHT, EAR_CENTER, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EAR_BOTTOM_RIGHT, EAR_BOTTOM_LEFT, EAR_TOP_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EAR_TOP_LEFT, EAR_TOP_MID, EAR_TOP_RIGHT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EAR_TOP_RIGHT, EAR_BOTTOM_RIGHT, EAR_TOP_LEFT, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EAR_TOP_RIGHT, EAR_TOP_MID, EAR_BACK, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EAR_TOP_LEFT, EAR_TOP_MID, EAR_BACK, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EAR_BOTTOM_LEFT, EAR_TOP_LEFT, EAR_BACK, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);
    addTriangles(EAR_BOTTOM_LEFT, EAR_BOTTOM_RIGHT, EAR_BACK, SKIN_COLOR[0], SKIN_COLOR[1], SKIN_COLOR[2]);


    // hair
    addTriangles(FOREHEAD_RIGHT, FOREHEAD_LEFT, HAIR_1, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(FOREHEAD_LEFT, HAIR_2, HAIR_1, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(FOREHEAD_LEFT, HAIR_3, HAIR_2, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(FOREHEAD_LEFT, HAIR_SIDE_1, HAIR_3, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(FOREHEAD_LEFT, HAIR_SIDE_2, HAIR_SIDE_1, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(FOREHEAD_LEFT, HAIR_SIDE_3, HAIR_SIDE_2, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(FOREHEAD_LEFT, EAR_TOP_RIGHT, HAIR_SIDE_3, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(EAR_TOP_RIGHT, HAIR_SIDE_4, HAIR_SIDE_3, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(EAR_TOP_RIGHT, EAR_BOTTOM_RIGHT, HAIR_SIDE_4, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(EAR_BOTTOM_RIGHT, HAIR_SIDE_5, HAIR_SIDE_4, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(EAR_BOTTOM_RIGHT, JAW_BOTTOM_LEFT, HAIR_SIDE_5, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);

    addTriangles(HAIR_3, HAIR_SIDE_1, HAIR_4, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_1, HAIR_SIDE_2, HAIR_4, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_2, HAIR_5, HAIR_4, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_2, HAIR_6, HAIR_5, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_2, HAIR_SIDE_3, HAIR_6, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_3, HAIR_7, HAIR_6, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_3, HAIR_SIDE_4, HAIR_7, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_4, HAIR_8, HAIR_7, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_4, HAIR_SIDE_5, HAIR_8, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_5, HAIR_9, HAIR_8, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_5, JAW_BOTTOM, HAIR_9, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addTriangles(HAIR_SIDE_5, JAW_BOTTOM, JAW_BOTTOM_LEFT, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);

    addJunction(FOREHEAD_RIGHT, HAIR_1, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addJunction(HAIR_1, HAIR_2, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addJunction(HAIR_2, HAIR_3, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addJunction(HAIR_3, HAIR_4, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addJunction(HAIR_4, HAIR_5, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addJunction(HAIR_5, HAIR_6, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addJunction(HAIR_6, HAIR_7, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addJunction(HAIR_7, HAIR_8, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addJunction(HAIR_8, HAIR_9, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);
    addJunction(HAIR_9, JAW_BOTTOM, HAIR_COLOR[0], HAIR_COLOR[1], HAIR_COLOR[2]);

    // Now that we have summed up the normals for each point, let's normalize them.
    for (int i = 0; i < _vertexNumber; ++i) {
        _newNormals[i] = glm::normalize(_newNormals[i]);
    }
}

void PerlinFace::addTriangles(int vertexID1, int vertexID2, int vertexID3, GLubyte r, GLubyte g, GLubyte b) {
    glm::vec3 normal = glm::normalize(glm::cross(_vertices[vertexID1] - _vertices[vertexID2],
                                                  _vertices[vertexID3] - _vertices[vertexID2]));

    addVertex(r, g, b, vertexID1, normal);
    addVertex(r, g, b, vertexID2, normal);
    addVertex(r, g, b, vertexID3, normal);
    ++_trianglesCount;

    vertexID1 += NUM_VERTICES;
    vertexID2 += NUM_VERTICES;
    vertexID3 += NUM_VERTICES;

    normal = glm::normalize(glm::cross(_vertices[vertexID1] - _vertices[vertexID2],
                                       _vertices[vertexID3] - _vertices[vertexID2]));

    addVertex(r, g, b, vertexID3, normal);
    addVertex(r, g, b, vertexID2, normal);
    addVertex(r, g, b, vertexID1, normal);
    ++_trianglesCount;
}

void PerlinFace::addJunction(int vertexID1, int vertexID2, GLubyte r, GLubyte g, GLubyte b) {
    glm::vec3 normal = glm::normalize(glm::cross(_vertices[vertexID1] - _vertices[NUM_VERTICES + vertexID1],
                                                 _vertices[vertexID2] - _vertices[NUM_VERTICES + vertexID1]));

    addVertex(r, g, b, vertexID1, normal);
    addVertex(r, g, b, NUM_VERTICES + vertexID1, normal);
    addVertex(r, g, b, vertexID2, normal);
    ++_trianglesCount;

    normal = glm::normalize(glm::cross(_vertices[vertexID2] - _vertices[NUM_VERTICES + vertexID1],
                                       _vertices[NUM_VERTICES + vertexID2] - _vertices[NUM_VERTICES + vertexID1]));

    addVertex(r, g, b, vertexID2, normal);
    addVertex(r, g, b, NUM_VERTICES + vertexID1, normal);
    addVertex(r, g, b, NUM_VERTICES + vertexID2, normal);
    ++_trianglesCount;
}

void PerlinFace::addVertex(GLubyte r, GLubyte g, GLubyte b, int vertexID, glm::vec3 normal) {
    *_trianglesPos++ = _vertices[vertexID].x;
    *_trianglesPos++ = _vertices[vertexID].y;
    *_trianglesPos++ = _vertices[vertexID].z;

    // use the normals of the previous frame for lighting
    *_normalsPos++ = _oldNormals[vertexID].x;
    *_normalsPos++ = _oldNormals[vertexID].y;
    *_normalsPos++ = _oldNormals[vertexID].z;

    // store all the normals associated to each point for next frame
    _newNormals[vertexID].x += normal.x;
    _newNormals[vertexID].y += normal.y;
    _newNormals[vertexID].z += normal.z;

    *_colorsPos++ = r;
    *_colorsPos++ = g;
    *_colorsPos++ = b;
}

glm::vec3 PerlinFace::getVec3(int vertexID) {
    return glm::vec3(VERTICES[FLOAT_PER_VERTEX * vertexID],
                     VERTICES[FLOAT_PER_VERTEX * vertexID + 1],
                     -VERTICES[FLOAT_PER_VERTEX * vertexID + 2]);   // Inverse Z axis to face the right direction
}
