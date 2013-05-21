//
//  Head.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "Head.h"
#include "Util.h"
#include <vector>
#include <SharedUtil.h>
#include <lodepng.h>

using namespace std;

const float EYE_RIGHT_OFFSET        =  0.27f;
const float EYE_UP_OFFSET           =  0.38f;
const float EYE_FRONT_OFFSET        =  0.8f;
const float EAR_RIGHT_OFFSET        =  1.0;
const float MOUTH_FRONT_OFFSET      =  1.0f;
const float MOUTH_UP_OFFSET         = -0.3f;
const float MINIMUM_EYE_ROTATION    = 0.7f; // based on a dot product: 1.0 is straight ahead, 0.0 is 90 degrees off
const float EYEBALL_RADIUS          = 0.02; 
const float EYEBALL_COLOR[3]        = { 0.9f, 0.9f, 0.8f };
const float IRIS_RADIUS             = 0.007;
const float IRIS_PROTRUSION         = 0.018f;
const char  IRIS_TEXTURE_FILENAME[] = "resources/images/iris.png";

unsigned int IRIS_TEXTURE_WIDTH  = 768;
unsigned int IRIS_TEXTURE_HEIGHT = 498;
vector<unsigned char> irisTexture;

Head::Head() :
    yawRate(0.0f),
    _audioLoudness(0.0f),
    _skinColor(0.0f, 0.0f, 0.0f),
    _position(0.0f, 0.0f, 0.0f),
    _rotation(0.0f, 0.0f, 0.0f),
    _mouthPosition(0.0f, 0.0f, 0.0f),
    _scale(1.0f),
    _browAudioLift(0.0f),
    _gravity(0.0f, -1.0f, 0.0f),
    _lastLoudness(0.0f),
    _averageLoudness(0.0f),
    _audioAttack(0.0f),
    _returnSpringScale(1.0f),
    _bodyRotation(0.0f, 0.0f, 0.0f),
    _headRotation(0.0f, 0.0f, 0.0f) {
}

void Head::reset() {
    _yaw = _pitch = _roll = 0.0f;
    _leanForward = _leanSideways = 0.0f;
}


void Head::simulate(float deltaTime, bool isMine) {

    const float HEAD_MOTION_DECAY = 0.00;
    
    //  Decay head back to center if turned on
    if (isMine && _returnHeadToCenter) {
    
        //  Decay rotation back toward center
        _pitch *= (1.0f - HEAD_MOTION_DECAY * _returnSpringScale * deltaTime);
        _yaw   *= (1.0f - HEAD_MOTION_DECAY * _returnSpringScale * deltaTime);
        _roll  *= (1.0f - HEAD_MOTION_DECAY * _returnSpringScale * deltaTime);
    }
    
    //  For invensense gyro, decay only slightly when roughly centered
    if (isMine) {
        const float RETURN_RANGE = 15.0;
        const float RETURN_STRENGTH = 2.0;
        if (fabs(_pitch) < RETURN_RANGE) { _pitch *= (1.0f - RETURN_STRENGTH * deltaTime); }
        if (fabs(_yaw  ) < RETURN_RANGE) { _yaw   *= (1.0f - RETURN_STRENGTH * deltaTime); }
        if (fabs(_roll ) < RETURN_RANGE) { _roll  *= (1.0f - RETURN_STRENGTH * deltaTime); }
    }

    // decay lean
    _leanForward  *= (1.f - HEAD_MOTION_DECAY * 30 * deltaTime);
    _leanSideways *= (1.f - HEAD_MOTION_DECAY * 30 * deltaTime);
                
    //  Update audio trailing average for rendering facial animations
    const float AUDIO_AVERAGING_SECS = 0.05;
    _averageLoudness = (1.f - deltaTime / AUDIO_AVERAGING_SECS) * _averageLoudness +
                             (deltaTime / AUDIO_AVERAGING_SECS) * _audioLoudness;
                             
                             
    //  Update audio attack data for facial animation (eyebrows and mouth)
    _audioAttack = 0.9 * _audioAttack + 0.1 * fabs(_audioLoudness - _lastLoudness);
    _lastLoudness = _audioLoudness;
    
    const float BROW_LIFT_THRESHOLD = 100;
    if (_audioAttack > BROW_LIFT_THRESHOLD)
        _browAudioLift += sqrt(_audioAttack) * 0.00005;
        
        float clamp = 0.01;
        if (_browAudioLift > clamp) { _browAudioLift = clamp; }
    
    _browAudioLift *= 0.7f;                             
}


void Head::setLooking(bool looking) {

    _lookingAtSomething = looking;

    glm::vec3 averageEyePosition = _leftEyePosition + (_rightEyePosition - _leftEyePosition ) * ONE_HALF;
    glm::vec3 targetLookatAxis = glm::normalize(_lookAtPosition - averageEyePosition);
    
    float dot = glm::dot(targetLookatAxis, _orientation.getFront());
    if (dot < MINIMUM_EYE_ROTATION) {
        _lookingAtSomething = false;
    }
}




void Head::calculateGeometry(bool lookingInMirror) {
    //generate orientation directions based on Euler angles...
    
    float pitch =  _pitch;
    float yaw   = -_yaw;
    float roll  = -_roll;
    
    if (lookingInMirror) {
        yaw   =  _yaw;
        roll  =  _roll;
    }
    
    _orientation.setToPitchYawRoll
    (
        _bodyRotation.x + pitch, 
        _bodyRotation.y + yaw, 
        _bodyRotation.z + roll
    );

    //calculate the eye positions 
    _leftEyePosition  = _position 
                      - _orientation.getRight() * _scale * EYE_RIGHT_OFFSET
                      + _orientation.getUp   () * _scale * EYE_UP_OFFSET
                      + _orientation.getFront() * _scale * EYE_FRONT_OFFSET;
    _rightEyePosition = _position
                      + _orientation.getRight() * _scale * EYE_RIGHT_OFFSET 
                      + _orientation.getUp   () * _scale * EYE_UP_OFFSET 
                      + _orientation.getFront() * _scale * EYE_FRONT_OFFSET;

    //calculate the ear positions 
    _leftEarPosition  = _position - _orientation.getRight() * _scale * EAR_RIGHT_OFFSET;
    _rightEarPosition = _position + _orientation.getRight() * _scale * EAR_RIGHT_OFFSET;

    //calculate the mouth position 
    _mouthPosition    = _position + _orientation.getUp   () * _scale * MOUTH_UP_OFFSET
                                  + _orientation.getFront() * _scale * MOUTH_FRONT_OFFSET;
}


void Head::render(bool lookingInMirror) {

    calculateGeometry(lookingInMirror);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
    renderHeadSphere();
    renderEyeBalls();    
    renderEars();
    renderMouth();    
    renderEyeBrows();    
        
    /*
    if (_lookingAtSomething) {
        // Render lines originating from the eyes and converging on the lookatPosition    
        debugRenderLookatVectors(_leftEyePosition, _rightEyePosition, _lookatPosition);
    }
    */
}

void Head::renderHeadSphere() {
    glPushMatrix();
        glTranslatef(_position.x, _position.y, _position.z); //translate to head position
        glScalef(_scale, _scale, _scale); //scale to head size        
        glColor3f(_skinColor.x, _skinColor.y, _skinColor.z);
        glutSolidSphere(1, 30, 30);
    glPopMatrix();
}

void Head::renderEars() {

    glPushMatrix();
        glColor3f(_skinColor.x, _skinColor.y, _skinColor.z);
        glTranslatef(_leftEarPosition.x, _leftEarPosition.y, _leftEarPosition.z);        
        glutSolidSphere(0.02, 30, 30);
    glPopMatrix();

    glPushMatrix();
        glColor3f(_skinColor.x, _skinColor.y, _skinColor.z);
        glTranslatef(_rightEarPosition.x, _rightEarPosition.y, _rightEarPosition.z);        
        glutSolidSphere(0.02, 30, 30);
    glPopMatrix();
}



void Head::renderMouth() {

    float s = sqrt(_averageLoudness);
    float height = _scale * (0.05f + s * 0.0040f );
    float width  = _scale * (0.30f + s * 0.0014f );

    glm::vec3 leftCorner  = _mouthPosition;
    glm::vec3 rightCorner = _mouthPosition;
    glm::vec3 leftTop     = _mouthPosition;
    glm::vec3 rightTop    = _mouthPosition;
    glm::vec3 leftBottom  = _mouthPosition;
    glm::vec3 rightBottom = _mouthPosition;
    
    leftCorner  -= _orientation.getRight() * width;
    rightCorner += _orientation.getRight() * width;
    leftTop     -= _orientation.getRight() * width * 0.4f;
    rightTop    += _orientation.getRight() * width * 0.4f;
    leftBottom  -= _orientation.getRight() * width * 0.4f;
    rightBottom += _orientation.getRight() * width * 0.4f;

    leftTop     += _orientation.getUp() * height * 0.7f;
    rightTop    += _orientation.getUp() * height * 0.7f;
    leftBottom  -= _orientation.getUp() * height;
    rightBottom -= _orientation.getUp() * height;

    leftTop     += _orientation.getFront() * _scale * 0.1f;
    rightTop    += _orientation.getFront() * _scale * 0.1f;
    leftBottom  += _orientation.getFront() * _scale * 0.1f;
    rightBottom += _orientation.getFront() * _scale * 0.1f;

    glColor3f(0.2f, 0.0f, 0.0f);
    
    glBegin(GL_TRIANGLES);             

    glVertex3f(leftCorner.x, leftCorner.y, leftCorner.z); 
    glVertex3f(leftBottom.x, leftBottom.y, leftBottom.z); 
    glVertex3f(leftTop.x,    leftTop.y,    leftTop.z   ); 

    glVertex3f(leftTop.x,    leftTop.y,    leftTop.z   ); 
    glVertex3f(rightTop.x,   rightTop.y,   rightTop.z  ); 
    glVertex3f(leftBottom.x, leftBottom.y, leftBottom.z); 
    
    glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
    glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
    glVertex3f(rightBottom.x, rightBottom.y, rightBottom.z); 

    glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
    glVertex3f(rightBottom.x, rightBottom.y, rightBottom.z); 
    glVertex3f(rightCorner.x, rightCorner.y, rightCorner.z); 

    glEnd();
}



void Head::renderEyeBrows() {   

    float height = _scale * 0.3f + _browAudioLift;
    float length = _scale * 0.2f;
    float width  = _scale * 0.07f;

    glColor3f(0.3f, 0.25f, 0.2f);

    glm::vec3 leftCorner  = _leftEyePosition;
    glm::vec3 rightCorner = _leftEyePosition;
    glm::vec3 leftTop     = _leftEyePosition;
    glm::vec3 rightTop    = _leftEyePosition;
    glm::vec3 leftBottom  = _leftEyePosition;
    glm::vec3 rightBottom = _leftEyePosition;
     
    for (int i = 0; i < 2; i++) {
    
        if ( i == 1 ) {
            leftCorner = rightCorner = leftTop = rightTop = leftBottom = rightBottom = _rightEyePosition;
        }
       
        leftCorner  -= _orientation.getRight() * length;
        rightCorner += _orientation.getRight() * length;
        leftTop     -= _orientation.getRight() * length * 0.4f;
        rightTop    += _orientation.getRight() * length * 0.4f;
        leftBottom  -= _orientation.getRight() * length * 0.4f;
        rightBottom += _orientation.getRight() * length * 0.4f;

        leftCorner  += _orientation.getUp() * height;
        rightCorner += _orientation.getUp() * height;
        leftTop     += _orientation.getUp() * (height + width);
        rightTop    += _orientation.getUp() * (height + width);
        leftBottom  += _orientation.getUp() * height;
        rightBottom += _orientation.getUp() * height;

        leftCorner  += _orientation.getFront() * _scale * -0.1f;
        rightCorner += _orientation.getFront() * _scale * -0.1f;
        leftTop     += _orientation.getFront() * _scale * -0.1f;
        rightTop    += _orientation.getFront() * _scale * -0.1f;
        leftBottom  += _orientation.getFront() * _scale * -0.1f;
        rightBottom += _orientation.getFront() * _scale * -0.1f;
        
        
        glBegin(GL_TRIANGLES);             

        glVertex3f(leftCorner.x, leftCorner.y, leftCorner.z); 
        glVertex3f(leftBottom.x, leftBottom.y, leftBottom.z); 
        glVertex3f(leftTop.x,    leftTop.y,    leftTop.z   ); 

        glVertex3f(leftTop.x,    leftTop.y,    leftTop.z   ); 
        glVertex3f(rightTop.x,   rightTop.y,   rightTop.z  ); 
        glVertex3f(leftBottom.x, leftBottom.y, leftBottom.z); 
        
        glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
        glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
        glVertex3f(rightBottom.x, rightBottom.y, rightBottom.z); 

        glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
        glVertex3f(rightBottom.x, rightBottom.y, rightBottom.z); 
        glVertex3f(rightCorner.x, rightCorner.y, rightCorner.z); 

        glEnd();
    }
  }
  
  

void Head::renderEyeBalls() {                                 
    
    if (::irisTexture.size() == 0) {
        switchToResourcesParentIfRequired();
        unsigned error = lodepng::decode(::irisTexture, IRIS_TEXTURE_WIDTH, IRIS_TEXTURE_HEIGHT, IRIS_TEXTURE_FILENAME);
        if (error != 0) {
            printLog("error %u: %s\n", error, lodepng_error_text(error));
        }
    }
    
    // setup the texutre to be used on each iris
    GLUquadric* irisQuadric = gluNewQuadric();
    gluQuadricTexture(irisQuadric, GL_TRUE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gluQuadricOrientation(irisQuadric, GLU_OUTSIDE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, IRIS_TEXTURE_WIDTH, IRIS_TEXTURE_HEIGHT,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, &::irisTexture[0]);

    // render white ball of left eyeball
    glPushMatrix();
        glColor3fv(EYEBALL_COLOR);
        glTranslatef(_leftEyePosition.x, _leftEyePosition.y, _leftEyePosition.z);        
        gluSphere(irisQuadric, EYEBALL_RADIUS, 30, 30);
    glPopMatrix();
    
    // render left iris
    glPushMatrix(); {
        glTranslatef(_leftEyePosition.x, _leftEyePosition.y, _leftEyePosition.z); //translate to eyeball position
        
        glPushMatrix();
        
//if (_lookingAtSomething) {

                //rotate the eyeball to aim towards the lookat position
                glm::vec3 targetLookatAxis = glm::normalize(_lookAtPosition - _leftEyePosition); // the lookat direction
                glm::vec3 rotationAxis = glm::cross(targetLookatAxis, IDENTITY_UP);
                float angle = 180.0f - angleBetween(targetLookatAxis, IDENTITY_UP);            
                glRotatef(angle, rotationAxis.x, rotationAxis.y, rotationAxis.z);
                glRotatef(180.0, 0.0f, 1.0f, 0.0f); //adjust roll to correct after previous rotations
/*
            } else {

                //rotate the eyeball to aim straight ahead
                glm::vec3 rotationAxisToHeadFront = glm::cross(_orientation.getFront(), IDENTITY_UP);            
                float angleToHeadFront = 180.0f - angleBetween(_orientation.getFront(), IDENTITY_UP);            
                glRotatef(angleToHeadFront, rotationAxisToHeadFront.x, rotationAxisToHeadFront.y, rotationAxisToHeadFront.z);

                //set the amount of roll (for correction after previous rotations)
                float rollRotation = angleBetween(_orientation.getFront(), IDENTITY_FRONT);            
                float dot = glm::dot(_orientation.getFront(), -IDENTITY_RIGHT);
                if ( dot < 0.0f ) { rollRotation = -rollRotation; }
                glRotatef(rollRotation, 0.0f, 1.0f, 0.0f); //roll the iris or correct roll about the lookat vector
            }
*/
             
            glTranslatef( 0.0f, -IRIS_PROTRUSION, 0.0f);//push the iris out a bit (otherwise - inside of eyeball!) 
            glScalef( 1.0f, 0.5f, 1.0f); // flatten the iris 
            glEnable(GL_TEXTURE_2D);
            gluSphere(irisQuadric, IRIS_RADIUS, 15, 15);
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }
    glPopMatrix();

    //render white ball of right eyeball
    glPushMatrix();
        glColor3fv(EYEBALL_COLOR);
        glTranslatef(_rightEyePosition.x, _rightEyePosition.y, _rightEyePosition.z);        
        gluSphere(irisQuadric, EYEBALL_RADIUS, 30, 30);
    glPopMatrix();

    // render right iris
    glPushMatrix(); {
        glTranslatef(_rightEyePosition.x, _rightEyePosition.y, _rightEyePosition.z);  //translate to eyeball position       

        glPushMatrix();
        
//if (_lookingAtSomething) {
            
                //rotate the eyeball to aim towards the lookat position
                glm::vec3 targetLookatAxis = glm::normalize(_lookAtPosition - _rightEyePosition);
                glm::vec3 rotationAxis = glm::cross(targetLookatAxis, IDENTITY_UP);
                float angle = 180.0f - angleBetween(targetLookatAxis, IDENTITY_UP);            
                glRotatef(angle, rotationAxis.x, rotationAxis.y, rotationAxis.z);
                glRotatef(180.0f, 0.0f, 1.0f, 0.0f); //adjust roll to correct after previous rotations
/*
            } else {

                //rotate the eyeball to aim straight ahead
                glm::vec3 rotationAxisToHeadFront = glm::cross(_orientation.getFront(), IDENTITY_UP);            
                float angleToHeadFront = 180.0f - angleBetween(_orientation.getFront(), IDENTITY_UP);            
                glRotatef(angleToHeadFront, rotationAxisToHeadFront.x, rotationAxisToHeadFront.y, rotationAxisToHeadFront.z);

                //set the amount of roll (for correction after previous rotations)
                float rollRotation = angleBetween(_orientation.getFront(), IDENTITY_FRONT); 
                float dot = glm::dot(_orientation.getFront(), -IDENTITY_RIGHT);
                if ( dot < 0.0f ) { rollRotation = -rollRotation; }
                glRotatef(rollRotation, 0.0f, 1.0f, 0.0f); //roll the iris or correct roll about the lookat vector
            }
*/
            
            glTranslatef( 0.0f, -IRIS_PROTRUSION, 0.0f);//push the iris out a bit (otherwise - inside of eyeball!) 
            glScalef( 1.0f, 0.5f, 1.0f); // flatten the iris 
            glEnable(GL_TEXTURE_2D);
            gluSphere(irisQuadric, IRIS_RADIUS, 15, 15);
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }
    
    // delete the iris quadric now that we're done with it
    gluDeleteQuadric(irisQuadric);
    glPopMatrix();
}

void Head::debugRenderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition) {

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(3.0);
    glBegin(GL_LINE_STRIP);
    glVertex3f(leftEyePosition.x, leftEyePosition.y, leftEyePosition.z);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(rightEyePosition.x, rightEyePosition.y, rightEyePosition.z);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glEnd();
}



