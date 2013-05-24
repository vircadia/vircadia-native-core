//
//  Head.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "Head.h"
#include "Util.h"
#include <vector>
#include <lodepng.h>
#include <AgentList.h>

using namespace std;
 
const int   MOHAWK_TRIANGLES         =  50;
const float EYE_RIGHT_OFFSET         =  0.27f;
const float EYE_UP_OFFSET            =  0.36f;
const float EYE_FRONT_OFFSET         =  0.8f;
const float EAR_RIGHT_OFFSET         =  1.0;
const float MOUTH_FRONT_OFFSET       =  0.9f;
const float MOUTH_UP_OFFSET          = -0.3f;
const float HEAD_MOTION_DECAY        =  0.1;
const float MINIMUM_EYE_ROTATION_DOT =  0.5f; // based on a dot product: 1.0 is straight ahead, 0.0 is 90 degrees off
const float EYEBALL_RADIUS           =  0.017; 
const float EYEBALL_COLOR[3]         =  { 0.9f, 0.9f, 0.8f };
const float IRIS_RADIUS              =  0.007;
const float IRIS_PROTRUSION          =  0.0145f;
const char  IRIS_TEXTURE_FILENAME[]  =  "resources/images/iris.png";

unsigned int IRIS_TEXTURE_WIDTH  = 768;
unsigned int IRIS_TEXTURE_HEIGHT = 498;
vector<unsigned char> irisTexture;

Head::Head() :

    yawRate(0.0f),
    _returnHeadToCenter(false),
    _audioLoudness(0.0f),
    _skinColor(0.0f, 0.0f, 0.0f),
    _position(0.0f, 0.0f, 0.0f),
    _rotation(0.0f, 0.0f, 0.0f),
    _leftEyePosition(0.0f, 0.0f, 0.0f),
    _rightEyePosition(0.0f, 0.0f, 0.0f),
    _leftEyeBrowPosition(0.0f, 0.0f, 0.0f),
    _rightEyeBrowPosition(0.0f, 0.0f, 0.0f),
    _leftEarPosition(0.0f, 0.0f, 0.0f),
    _rightEarPosition(0.0f, 0.0f, 0.0f),
    _mouthPosition(0.0f, 0.0f, 0.0f),
    _scale(1.0f),
    _browAudioLift(0.0f),
    _lookingAtSomething(false),
    _gravity(0.0f, -1.0f, 0.0f),
    _lastLoudness(0.0f),
    _averageLoudness(0.0f),
    _audioAttack(0.0f),
    _returnSpringScale(1.0f),
    _bodyRotation(0.0f, 0.0f, 0.0f),
    _renderLookatVectors(false),
    _mohawkTriangleFan(NULL),
    _mohawkColors(NULL)
{
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
    
    //  For invensense gyro, decay only slightly when near center (until we add fusion)
    if (isMine) {
        const float RETURN_RANGE = 15.0;
        const float RETURN_STRENGTH = 0.5;
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

    // based on the nature of the lookat position, determine if the eyes can look / are looking at it.  
    determineIfLookingAtSomething();                                                                         
}

void Head::determineIfLookingAtSomething() { 

    if ( fabs(_lookAtPosition.x + _lookAtPosition.y + _lookAtPosition.z) == 0.0 ) { // a lookatPosition of 0,0,0 signifies NOT looking
        _lookingAtSomething = false;
    } else {
        glm::vec3 targetLookatAxis = glm::normalize(_lookAtPosition - caclulateAverageEyePosition());
        float dot = glm::dot(targetLookatAxis, _orientation.getFront());
        if (dot < MINIMUM_EYE_ROTATION_DOT) { // too far off from center for the eyes to rotate 
            _lookingAtSomething = false;
        } else {
            _lookingAtSomething = true;
        }
    }
}

void Head::calculateGeometry(bool lookingInMirror) {
    //generate orientation directions based on Euler angles...
    
    float pitch =  _pitch;
    float yaw   = _yaw;
    float roll  = _roll;
    
    if (lookingInMirror) {
        yaw   =  -_yaw;
        roll  =  -_roll;
    }

    _orientation.setToIdentity();
    _orientation.roll (_bodyRotation.z + roll );
    _orientation.pitch(_bodyRotation.x + pitch);
    _orientation.yaw  (_bodyRotation.y + yaw  );

    //calculate the eye positions 
    _leftEyePosition  = _position 
                      - _orientation.getRight() * _scale * EYE_RIGHT_OFFSET
                      + _orientation.getUp   () * _scale * EYE_UP_OFFSET
                      + _orientation.getFront() * _scale * EYE_FRONT_OFFSET;
    _rightEyePosition = _position
                      + _orientation.getRight() * _scale * EYE_RIGHT_OFFSET 
                      + _orientation.getUp   () * _scale * EYE_UP_OFFSET 
                      + _orientation.getFront() * _scale * EYE_FRONT_OFFSET;

    //calculate the eyebrow positions 
    _leftEyeBrowPosition  = _leftEyePosition; 
    _rightEyeBrowPosition = _rightEyePosition;
    
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
    
    renderMohawk(lookingInMirror);
    renderHeadSphere();
    renderEyeBalls();    
    renderEars();
    renderMouth();    
    renderEyeBrows();    
        
    if (_renderLookatVectors && _lookingAtSomething) {
        renderLookatVectors(_leftEyePosition, _rightEyePosition, _lookAtPosition);
    }
}

void Head::createMohawk() {
//    int agentId = AgentList::getInstance()
    float height = 0.05f + randFloat() * 0.10f;
    float variance = 0.05 + randFloat() * 0.05f;
    const float RAD_PER_TRIANGLE = (2.3f + randFloat() * 0.2f) / (float)MOHAWK_TRIANGLES;
    _mohawkTriangleFan = new glm::vec3[MOHAWK_TRIANGLES];
    _mohawkColors = new glm::vec3[MOHAWK_TRIANGLES];
    _mohawkTriangleFan[0] = glm::vec3(0, 0, 0);
    glm::vec3 basicColor(randFloat(), randFloat(), randFloat());
    _mohawkColors[0] = basicColor;
    for (int i = 1; i < MOHAWK_TRIANGLES; i++) {
        _mohawkTriangleFan[i]  = glm::vec3((randFloat() - 0.5f) * variance,
                                           height * cosf(i * RAD_PER_TRIANGLE - PI / 2.f)
                                           + (randFloat()  - 0.5f) * variance,
                                           height * sinf(i * RAD_PER_TRIANGLE - PI / 2.f)
                                           + (randFloat() - 0.5f) * variance);
        _mohawkColors[i] = randFloat() * basicColor;

    }
}

void Head::renderMohawk(bool lookingInMirror) {
    if (!_mohawkTriangleFan) {
        createMohawk();
    } else {
        glPushMatrix();
        glTranslatef(_position.x, _position.y, _position.z);
        glRotatef(-(_bodyRotation.y + _yaw) - 90, 0, 1, 0);
        //glRotatef((lookingInMirror ? _roll: -roll), 0, 0, 1);
        glRotatef(-_pitch, 1, 0, 0);
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i < MOHAWK_TRIANGLES; i++) {
            glColor3f(_mohawkColors[i].x, _mohawkColors[i].y, _mohawkColors[i].z);
            glVertex3fv(&_mohawkTriangleFan[i].x);
            glNormal3fv(&_mohawkColors[i].x);
        }
        glEnd();
        glPopMatrix();
    }
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

    glm::vec3 r = _orientation.getRight() * _scale * (0.30f + s * 0.0014f );
    glm::vec3 u = _orientation.getUp   () * _scale * (0.05f + s * 0.0040f );
    glm::vec3 f = _orientation.getFront() * _scale *  0.09f;

    glm::vec3 leftCorner  = _mouthPosition - r * 1.0f;
    glm::vec3 rightCorner = _mouthPosition + r * 1.0f;
    glm::vec3 leftTop     = _mouthPosition - r * 0.4f + u * 0.7f + f;
    glm::vec3 rightTop    = _mouthPosition + r * 0.4f + u * 0.7f + f;
    glm::vec3 leftBottom  = _mouthPosition - r * 0.4f - u * 1.0f + f * 0.7f;
    glm::vec3 rightBottom = _mouthPosition + r * 0.4f - u * 1.0f + f * 0.7f;
        
    glColor3f(0.2f, 0.0f, 0.0f);
    
    glBegin(GL_TRIANGLES);             
    glVertex3f(leftCorner.x,  leftCorner.y,  leftCorner.z ); 
    glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
    glVertex3f(leftTop.x,     leftTop.y,     leftTop.z    ); 
    glVertex3f(leftTop.x,     leftTop.y,     leftTop.z    ); 
    glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
    glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
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
     
    glm::vec3 r = _orientation.getRight() * length; 
    glm::vec3 u = _orientation.getUp()    * height; 
    glm::vec3 t = _orientation.getUp()    * (height + width); 
    glm::vec3 f = _orientation.getFront() * _scale * -0.1f;
     
    for (int i = 0; i < 2; i++) {
    
        if ( i == 1 ) {
            leftCorner = rightCorner = leftTop = rightTop = leftBottom = rightBottom = _rightEyePosition;
        }
       
        leftCorner  -= r * 1.0f;
        rightCorner += r * 1.0f;
        leftTop     -= r * 0.4f;
        rightTop    += r * 0.4f;
        leftBottom  -= r * 0.4f;
        rightBottom += r * 0.4f;

        leftCorner  += u + f;
        rightCorner += u + f;
        leftTop     += t + f;
        rightTop    += t + f;
        leftBottom  += u + f;
        rightBottom += u + f;        
        
        glBegin(GL_TRIANGLES);             

        glVertex3f(leftCorner.x,  leftCorner.y,  leftCorner.z ); 
        glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
        glVertex3f(leftTop.x,     leftTop.y,     leftTop.z    ); 
        glVertex3f(leftTop.x,     leftTop.y,     leftTop.z    ); 
        glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
        glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
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
        
            if (_lookingAtSomething) {

                //rotate the eyeball to aim towards the lookat position
                glm::vec3 targetLookatAxis = glm::normalize(_lookAtPosition - _leftEyePosition); // the lookat direction
                glm::vec3 rotationAxis = glm::cross(targetLookatAxis, IDENTITY_UP);
                float angle = 180.0f - angleBetween(targetLookatAxis, IDENTITY_UP);            
                glRotatef(angle, rotationAxis.x, rotationAxis.y, rotationAxis.z);
                glRotatef(180.0, 0.0f, 1.0f, 0.0f); //adjust roll to correct after previous rotations
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
        
            if (_lookingAtSomething) {
            
                //rotate the eyeball to aim towards the lookat position
                glm::vec3 targetLookatAxis = glm::normalize(_lookAtPosition - _rightEyePosition);
                glm::vec3 rotationAxis = glm::cross(targetLookatAxis, IDENTITY_UP);
                float angle = 180.0f - angleBetween(targetLookatAxis, IDENTITY_UP);            
                glRotatef(angle, rotationAxis.x, rotationAxis.y, rotationAxis.z);
                glRotatef(180.0f, 0.0f, 1.0f, 0.0f); //adjust roll to correct after previous rotations

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

void Head::renderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition) {

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0);
    glBegin(GL_LINE_STRIP);
    glVertex3f(leftEyePosition.x, leftEyePosition.y, leftEyePosition.z);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(rightEyePosition.x, rightEyePosition.y, rightEyePosition.z);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glEnd();
}



