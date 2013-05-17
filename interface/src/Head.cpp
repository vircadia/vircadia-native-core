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

const float HEAD_MOTION_DECAY = 0.1;
const float MINIMUM_EYE_ROTATION = 0.7f; // based on a dot product: 1.0 is straight ahead, 0.0 is 90 degrees off

float _browColor [] = {210.0/255.0, 105.0/255.0, 30.0/255.0};
float _mouthColor[] = {1, 0, 0};

float _BrowRollAngle [5] = {0, 15, 30, -30, -15};
float _BrowPitchAngle[3] = {-70, -60, -50};
float _eyeColor      [3] = {1,1,1};

float _MouthWidthChoices[3] = {0.5, 0.77, 0.3};

float _browWidth = 0.8;
float _browThickness = 0.16;

char iris_texture_file[] = "resources/images/green_eye.png";

vector<unsigned char> iris_texture;
unsigned int iris_texture_width = 512;
unsigned int iris_texture_height = 256;

Head::Head() {
    if (iris_texture.size() == 0) {
        switchToResourcesParentIfRequired();
        unsigned error = lodepng::decode(iris_texture, iris_texture_width, iris_texture_height, iris_texture_file);
        if (error != 0) {
            printLog("error %u: %s\n", error, lodepng_error_text(error));
        }
    }
}

Head::Head(const Head &head) {
 
    yawRate      = head.yawRate;
    noise        = head.noise;
    leanForward  = head.leanForward;
    leanSideways = head.leanSideways;

    _sphere              = NULL;
    _returnHeadToCenter  = head._returnHeadToCenter;
    _audioLoudness       = head._audioLoudness;
    _skinColor           = head._skinColor;
    _position            = head._position;
    _rotation            = head._rotation;
    _lookatPosition      = head._lookatPosition;
    _leftEyePosition     = head._leftEyePosition;
    _rightEyePosition    = head._rightEyePosition;
    _yaw                 = head._yaw;
    _pitch               = head._pitch;
    _roll                = head._roll;
    _pitchRate           = head._pitchRate;
    _rollRate            = head._rollRate;
    _eyeballPitch[0]     = head._eyeballPitch[0];
    _eyeballYaw  [0]     = head._eyeballYaw  [0];
    _eyebrowPitch[0]     = head._eyebrowPitch[0];
    _eyebrowRoll [0]     = head._eyebrowRoll [0];
    _eyeballPitch[1]     = head._eyeballPitch[1];
    _eyeballYaw  [1]     = head._eyeballYaw  [1];
    _eyebrowPitch[1]     = head._eyebrowPitch[1];
    _eyebrowRoll [1]     = head._eyebrowRoll [1];
    _eyeballScaleX       = head._eyeballScaleX;
    _eyeballScaleY       = head._eyeballScaleY;
    _eyeballScaleZ       = head._eyeballScaleZ;
    _interPupilDistance  = head._interPupilDistance;
    _interBrowDistance   = head._interBrowDistance;
    _nominalPupilSize    = head._nominalPupilSize;
    _pupilSize           = head._pupilSize;
    _mouthPitch          = head._mouthPitch;
    _mouthYaw            = head._mouthYaw;
    _mouthWidth          = head._mouthWidth;
    _mouthHeight         = head._mouthHeight;
    _pitchTarget         = head._pitchTarget;
    _yawTarget           = head._yawTarget;
    _noiseEnvelope       = head._noiseEnvelope;
    _pupilConverge       = head._pupilConverge;
    _scale               = head._scale;
    _eyeContact          = head._eyeContact;
    _browAudioLift       = head._browAudioLift;
    _eyeContactTarget    = head._eyeContactTarget;
    _orientation         = head._orientation;
    _bodyYaw             = head._bodyYaw;
    _lastLoudness        = head._lastLoudness;
    _averageLoudness     = head._averageLoudness;
    _audioAttack         = head._audioAttack;
    _looking             = head._looking;
    _gravity             = head._gravity;
    _returnSpringScale   = head._returnSpringScale; 
}


        
void Head::initialize() {

    _bodyYaw              = 0.0f;
    _audioLoudness         = 0.0;
    _skinColor             = glm::vec3(0.0f, 0.0f, 0.0f);
    _position              = glm::vec3(0.0f, 0.0f, 0.0f);
    _lookatPosition        = glm::vec3(0.0f, 0.0f, 0.0f);
    _gravity              = glm::vec3(0.0f, -1.0f, 0.0f); // default
    _yaw                   = 0.0f;
    _pitch                 = 0.0f;
    _roll                  = 0.0f;
    _pupilSize             = 0.10;
    _interPupilDistance    = 0.6;
    _interBrowDistance     = 0.75;
    _nominalPupilSize      = 0.10;
    _pitchRate             = 0.0;
     yawRate               = 0.0;
    _rollRate              = 0.0;
    _eyebrowPitch[0]       = -30;
    _eyebrowPitch[1]       = -30;
    _eyebrowRoll [0]       = 20;
    _eyebrowRoll [1]       = -20;
    _mouthPitch            = 0;
    _mouthYaw              = 0;
    _mouthWidth            = 1.0;
    _mouthHeight           = 0.2;
    _eyeballPitch[0]       = 0;
    _eyeballPitch[1]       = 0;
    _eyeballScaleX         = 1.2;
    _eyeballScaleY         = 1.5;
    _eyeballScaleZ         = 1.0;
    _eyeballYaw[0]         = 0;
    _eyeballYaw[1]         = 0;
    _pitchTarget           = 0;
    _yawTarget             = 0;
    _noiseEnvelope         = 1.0;
    _pupilConverge         = 10.0;
     leanForward           = 0.0;
     leanSideways          = 0.0;
    _eyeContact            = 1;
    _eyeContactTarget      = LEFT_EYE;
    _scale                 = 1.0;
    _audioAttack           = 0.0;
    _averageLoudness       = 0.0;
    _lastLoudness          = 0.0;
    _browAudioLift         = 0.0;
     noise                 = 0;
    _returnSpringScale     = 1.0;
     _sphere               = NULL;
}

/*
void Head::copyFromHead(const Head &head) {

    returnHeadToCenter  = head.returnHeadToCenter;
    audioLoudness       = head.audioLoudness;
    skinColor           = head.skinColor;
    position            = head.position;
    rotation            = head.rotation;
    lookatPosition      = head.lookatPosition;
    _leftEyePosition    = head._leftEyePosition;
    _rightEyePosition   = head._rightEyePosition;
    yaw                 = head.yaw;
    pitch               = head.pitch;
    roll                = head.roll;
    pitchRate           = head.pitchRate;
    yawRate             = head.yawRate;
    rollRate            = head.rollRate;
    noise               = head.noise;
    eyeballPitch[0]     = head.eyeballPitch[0];
    eyeballYaw  [0]     = head.eyeballYaw  [0];
    eyebrowPitch[0]     = head.eyebrowPitch[0];
    eyebrowRoll [0]     = head.eyebrowRoll [0];
    eyeballPitch[1]     = head.eyeballPitch[1];
    eyeballYaw  [1]     = head.eyeballYaw  [1];
    eyebrowPitch[1]     = head.eyebrowPitch[1];
    eyebrowRoll [1]     = head.eyebrowRoll [1];
    eyeballScaleX       = head.eyeballScaleX;
    eyeballScaleY       = head.eyeballScaleY;
    eyeballScaleZ       = head.eyeballScaleZ;
    interPupilDistance  = head.interPupilDistance;
    interBrowDistance   = head.interBrowDistance;
    nominalPupilSize    = head.nominalPupilSize;
    pupilSize           = head.pupilSize;
    mouthPitch          = head.mouthPitch;
    mouthYaw            = head.mouthYaw;
    mouthWidth          = head.mouthWidth;
    mouthHeight         = head.mouthHeight;
    leanForward         = head.leanForward;
    leanSideways        = head.leanSideways;
    pitchTarget         = head.pitchTarget;
    yawTarget           = head.yawTarget;
    noiseEnvelope       = head.noiseEnvelope;
    pupilConverge       = head.pupilConverge;
    scale               = head.scale;
    eyeContact          = head.eyeContact;
    browAudioLift       = head.browAudioLift;
    eyeContactTarget    = head.eyeContactTarget;
    _orientation        = head._orientation;
    _bodyYaw            = head._bodyYaw;
    lastLoudness        = head.lastLoudness;
    averageLoudness     = head.averageLoudness;
    audioAttack         = head.audioAttack;
    _looking            = head._looking;
    _gravity            = head._gravity;
    sphere              = head.sphere;
    returnSpringScale   = head.returnSpringScale;
}
*/

void Head::setPositionRotationAndScale(glm::vec3 p, glm::vec3 r, float s) {

    _position = p;
    _scale    = s;
    _yaw      = r.x;
    _pitch    = r.y;
    _roll     = r.z;
}

void Head::setSkinColor(glm::vec3 c) {
    _skinColor = c;
}

void Head::setAudioLoudness(float loudness) {
    _audioLoudness = loudness;
}


void Head::setNewTarget(float pitch, float yaw) {
    _pitchTarget = pitch;
    _yawTarget   = yaw;
}

void Head::simulate(float deltaTime, bool isMine) {

    //generate orientation directions based on Euler angles...
    _orientation.setToPitchYawRoll( _pitch, _bodyYaw + _yaw, _roll);

    //calculate the eye positions (algorithm still being designed)
    updateEyePositions();

    //  Decay head back to center if turned on
    if (isMine && _returnHeadToCenter) {
        //  Decay back toward center
        _pitch *= (1.0f - HEAD_MOTION_DECAY * _returnSpringScale * 2 * deltaTime);
        _yaw   *= (1.0f - HEAD_MOTION_DECAY * _returnSpringScale * 2 * deltaTime);
        _roll  *= (1.0f - HEAD_MOTION_DECAY * _returnSpringScale * 2 * deltaTime);
    }
    
    //  For invensense gyro, decay only slightly when roughly centered
    if (isMine) {
        const float RETURN_RANGE = 15.0;
        const float RETURN_STRENGTH = 2.0;
        if (fabs(_pitch) < RETURN_RANGE) { _pitch *= (1.0f - RETURN_STRENGTH * deltaTime); }
        if (fabs(_yaw)   < RETURN_RANGE) { _yaw   *= (1.0f - RETURN_STRENGTH * deltaTime); }
        if (fabs(_roll)  < RETURN_RANGE) { _roll  *= (1.0f - RETURN_STRENGTH * deltaTime); }
    }

    if (noise) {
        //  Move toward new target
        _pitch += (_pitchTarget - _pitch) * 10 * deltaTime; // (1.f - DECAY*deltaTime)*Pitch + ;
        _yaw   += (_yawTarget   - _yaw  ) * 10 * deltaTime; // (1.f - DECAY*deltaTime);
        _roll *= 1.f - (HEAD_MOTION_DECAY * deltaTime);
    }
    
    leanForward  *= (1.f - HEAD_MOTION_DECAY * 30 * deltaTime);
    leanSideways *= (1.f - HEAD_MOTION_DECAY * 30 * deltaTime);
        
    //  Update where the avatar's eyes are
    //
    //  First, decide if we are making eye contact or not
    if (randFloat() < 0.005) {
        _eyeContact = !_eyeContact;
        _eyeContact = 1;
        if (!_eyeContact) {
            //  If we just stopped making eye contact,move the eyes markedly away
            _eyeballPitch[0] = _eyeballPitch[1] = _eyeballPitch[0] + 5.0 + (randFloat() - 0.5) * 10;
            _eyeballYaw  [0] = _eyeballYaw  [1] = _eyeballYaw  [0] + 5.0 + (randFloat() - 0.5) * 5;
        } else {
            //  If now making eye contact, turn head to look right at viewer
            setNewTarget(0,0);
        }
    }
    
    const float DEGREES_BETWEEN_VIEWER_EYES = 3;
    const float DEGREES_TO_VIEWER_MOUTH = 7;
    
    if (_eyeContact) {
        //  Should we pick a new eye contact target?
        if (randFloat() < 0.01) {
            //  Choose where to look next
            if (randFloat() < 0.1) {
                _eyeContactTarget = MOUTH;
            } else {
                if (randFloat() < 0.5) _eyeContactTarget = LEFT_EYE; else _eyeContactTarget = RIGHT_EYE;
            }
        }
        //  Set eyeball pitch and yaw to make contact
        float eye_target_yaw_adjust = 0;
        float eye_target_pitch_adjust = 0;
        if (_eyeContactTarget == LEFT_EYE) eye_target_yaw_adjust = DEGREES_BETWEEN_VIEWER_EYES;
        if (_eyeContactTarget == RIGHT_EYE) eye_target_yaw_adjust = -DEGREES_BETWEEN_VIEWER_EYES;
        if (_eyeContactTarget == MOUTH) eye_target_pitch_adjust = DEGREES_TO_VIEWER_MOUTH;
        
        _eyeballPitch[0] = _eyeballPitch[1] = -_pitch + eye_target_pitch_adjust;
        _eyeballYaw  [0] = _eyeballYaw  [1] =  _yaw   + eye_target_yaw_adjust;
    }
    
    if (noise)
    {
        _pitch += (randFloat() - 0.5) * 0.2 * _noiseEnvelope;
        _yaw += (randFloat() - 0.5) * 0.3 *_noiseEnvelope;
        //PupilSize += (randFloat() - 0.5) * 0.001*NoiseEnvelope;
        
        if (randFloat() < 0.005) _mouthWidth = _MouthWidthChoices[rand()%3];
        
        if (!_eyeContact) {
            if (randFloat() < 0.01)  _eyeballPitch[0] = _eyeballPitch[1] = (randFloat() - 0.5) * 20;
            if (randFloat() < 0.01)  _eyeballYaw[0] = _eyeballYaw[1] = (randFloat()- 0.5) * 10;
        }
        
        if ((randFloat() < 0.005) && (fabs(_pitchTarget - _pitch) < 1.0) && (fabs(_yawTarget - _yaw) < 1.0)) {
            setNewTarget((randFloat()-0.5) * 20.0, (randFloat()-0.5) * 45.0);
        }
        
        if (0) {
            
            //  Pick new target
            _pitchTarget = (randFloat() - 0.5) * 45;
            _yawTarget = (randFloat() - 0.5) * 22;
        }
        if (randFloat() < 0.01)
        {
            _eyebrowPitch[0] = _eyebrowPitch[1] = _BrowPitchAngle[rand()%3];
            _eyebrowRoll [0] = _eyebrowRoll[1] = _BrowRollAngle[rand()%5];
            _eyebrowRoll [1] *=-1;
        }
    }
    
    //  Update audio trailing average for rendering facial animations
    const float AUDIO_AVERAGING_SECS = 0.05;
    _averageLoudness = (1.f - deltaTime / AUDIO_AVERAGING_SECS) * _averageLoudness +
                            (deltaTime / AUDIO_AVERAGING_SECS) * _audioLoudness;
                                                        
}

void Head::updateEyePositions() {
    float rightShift = _scale * 0.27f;
    float upShift    = _scale * 0.38f;
    float frontShift = _scale * 0.8f;
    
    _leftEyePosition  = _position + _orientation.getRight() * rightShift 
                                 + _orientation.getUp   () * upShift 
                                 + _orientation.getFront() * frontShift;
    _rightEyePosition = _position - _orientation.getRight() * rightShift 
                                 + _orientation.getUp   () * upShift 
                                 + _orientation.getFront() * frontShift;
}

void Head::setLooking(bool looking) {

    _looking = looking;

    glm::vec3 averagEyePosition = _leftEyePosition + (_rightEyePosition - _leftEyePosition ) * ONE_HALF;
    glm::vec3 targetLookatAxis = glm::normalize(_lookatPosition - averagEyePosition);
    
    float dot = glm::dot(targetLookatAxis, _orientation.getFront());
    if (dot < MINIMUM_EYE_ROTATION) {
        _looking = false;
    }
}

void Head::setLookatPosition(glm::vec3 l) {
    _lookatPosition = l;
}

void Head::setGravity(glm::vec3 gravity) {
    _gravity = gravity;
}

glm::vec3 Head::getApproximateEyePosition() {
    return _leftEyePosition + (_rightEyePosition - _leftEyePosition) * ONE_HALF;
}

void Head::render(bool lookingInMirror) {

    int side = 0;
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
        
    glPushMatrix();
    
        glTranslatef(_position.x, _position.y, _position.z);
	
        glScalef(_scale, _scale, _scale);
    
    if (lookingInMirror) {
        glRotatef(_bodyYaw  - _yaw,   0, 1, 0);
        glRotatef(_pitch, 1, 0, 0);   
        glRotatef(-_roll,  0, 0, 1);
    } else {
        glRotatef(_bodyYaw  + _yaw,   0, 1, 0);
        glRotatef(_pitch, 1, 0, 0);
        glRotatef(_roll,  0, 0, 1);
    }
    
    //glScalef(2.0, 2.0, 2.0);
    glColor3f(_skinColor.x, _skinColor.y, _skinColor.z);
    
    glutSolidSphere(1, 30, 30);
    
    //  Ears
    glPushMatrix();
    glTranslatef(1.0, 0, 0);
    for(side = 0; side < 2; side++) {
        glPushMatrix();
        glScalef(0.3, 0.65, .65);
        glutSolidSphere(0.5, 30, 30);
        glPopMatrix();
        glTranslatef(-2.0, 0, 0);
    }
    glPopMatrix();
    
    //  Update audio attack data for facial animation (eyebrows and mouth)
    _audioAttack = 0.9 * _audioAttack + 0.1 * fabs(_audioLoudness - _lastLoudness);
    _lastLoudness = _audioLoudness;
    
    const float BROW_LIFT_THRESHOLD = 100;
    if (_audioAttack > BROW_LIFT_THRESHOLD)
        _browAudioLift += sqrt(_audioAttack) / 1000.0;
    
    _browAudioLift *= .90;
    
    //  Render Eyebrows
    glPushMatrix();
    glTranslatef(-_interBrowDistance / 2.0,0.4,0.45);
    for(side = 0; side < 2; side++) {
        glColor3fv(_browColor);
        glPushMatrix();
        glTranslatef(0, 0.35 + _browAudioLift, 0);
        glRotatef(_eyebrowPitch[side]/2.0, 1, 0, 0);
        glRotatef(_eyebrowRoll[side]/2.0, 0, 0, 1);
        glScalef(_browWidth, _browThickness, 1);
        glutSolidCube(0.5);
        glPopMatrix();
        glTranslatef(_interBrowDistance, 0, 0);
    }
    glPopMatrix();
    
    // Mouth
    glPushMatrix();
        glTranslatef(0,-0.35,0.75);
        glColor3f(0,0,0);

        glRotatef(_mouthPitch, 1, 0, 0);
        glRotatef(_mouthYaw, 0, 0, 1);

        if (_averageLoudness > 1.f) {
            glScalef(_mouthWidth  * (.7f + sqrt(_averageLoudness) /60.f),
                     _mouthHeight * (1.f + sqrt(_averageLoudness) /30.f), 1);
        } else {
            glScalef(_mouthWidth, _mouthHeight, 1);
        } 

        glutSolidCube(0.5);
    glPopMatrix();

    // the original code from Philip's implementation
    //previouseRenderEyeBalls();
    
    glPopMatrix();

    //a new version of eyeballs that has the ability to look at specific targets in the world (algo still not finished yet)
    renderEyeBalls();    
    
    if (_looking) {
        // Render lines originating from the eyes and converging on the lookatPosition    
        debugRenderLookatVectors(_leftEyePosition, _rightEyePosition, _lookatPosition);
    }
}




void Head::renderEyeBalls() {                                 
            
    //make the texture for the iris...
    if (_sphere == NULL) {
        _sphere = gluNewQuadric();
        gluQuadricTexture(_sphere, GL_TRUE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluQuadricOrientation(_sphere, GLU_OUTSIDE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iris_texture_width, iris_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &iris_texture[0]);
    }
    
    // left eyeball
    glPushMatrix();
        glColor3fv(_eyeColor);
        glTranslatef(_leftEyePosition.x, _leftEyePosition.y, _leftEyePosition.z);        
        gluSphere(_sphere, 0.02, 30, 30);
    glPopMatrix();
    
    // left iris
    glPushMatrix(); {
        glTranslatef(_leftEyePosition.x, _leftEyePosition.y, _leftEyePosition.z);        
        glm::vec3 targetLookatAxis = glm::normalize(_lookatPosition - _leftEyePosition);
        
        if (!_looking) {
            targetLookatAxis = _orientation.getFront();
        }        
        
        glPushMatrix();
            glm::vec3 rotationAxis = glm::cross(targetLookatAxis, glm::vec3(0.0f, 1.0f, 0.0f));
            float angle = 180.0f - angleBetween(targetLookatAxis, glm::vec3(0.0f, 1.0f, 0.0f));            

            //glm::vec3 U_rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
            //float U_angle = angleBetween(_orientation.getFront(), glm::vec3(1.0f, 0.0f, 0.0f));            
            //glRotatef(U_angle, U_rotationAxis.x, U_rotationAxis.y, U_rotationAxis.z);
            glRotatef(angle, rotationAxis.x, rotationAxis.y, rotationAxis.z);

            glTranslatef( 0.0f, -0.018f, 0.0f);//push the iris out a bit (otherwise - inside of eyeball!) 
            glScalef( 1.0f, 0.5f, 1.0f); // flatten the iris 
            glEnable(GL_TEXTURE_2D);
            gluSphere(_sphere, 0.007, 15, 15);
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }
    glPopMatrix();

    //right eyeball
    glPushMatrix();
        glColor3fv(_eyeColor);
        glTranslatef(_rightEyePosition.x, _rightEyePosition.y, _rightEyePosition.z);        
        gluSphere(_sphere, 0.02, 30, 30);
    glPopMatrix();

    //right iris
    glPushMatrix(); {
        glTranslatef(_rightEyePosition.x, _rightEyePosition.y, _rightEyePosition.z);        
        glm::vec3 targetLookatAxis = glm::normalize(_lookatPosition - _rightEyePosition);

        if (!_looking) {
            targetLookatAxis = _orientation.getFront();
        }        

        glPushMatrix();
            glm::vec3 rotationAxis = glm::cross(targetLookatAxis, glm::vec3(0.0f, 1.0f, 0.0f));
            float angle = 180.0f - angleBetween(targetLookatAxis, glm::vec3(0.0f, 1.0f, 0.0f));
            glRotatef(angle, rotationAxis.x, rotationAxis.y, rotationAxis.z);
            glTranslatef( 0.0f, -0.018f, 0.0f);//push the iris out a bit (otherwise - inside of eyeball!) 
            glScalef( 1.0f, 0.5f, 1.0f); // flatten the iris 
            glEnable(GL_TEXTURE_2D);
            gluSphere(_sphere, 0.007, 15, 15);
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }
    glPopMatrix();
}





void Head::previouseRenderEyeBalls() {

    glTranslatef(0, 1.0, 0);

    glTranslatef(-_interPupilDistance/2.0,-0.68,0.7);
    // Right Eye
    glRotatef(-10, 1, 0, 0);
    glColor3fv(_eyeColor);
    glPushMatrix();
    {
        glTranslatef(_interPupilDistance/10.0, 0, 0.05);
        glRotatef(20, 0, 0, 1);
        glScalef(_eyeballScaleX, _eyeballScaleY, _eyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    
    // Right Pupil
    if (_sphere == NULL) {
        _sphere = gluNewQuadric();
        gluQuadricTexture(_sphere, GL_TRUE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluQuadricOrientation(_sphere, GLU_OUTSIDE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iris_texture_width, iris_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &iris_texture[0]);
    }

    glPushMatrix();
    {   
        glRotatef(_eyeballPitch[1], 1, 0, 0);
        glRotatef(_eyeballYaw[1] + _yaw + _pupilConverge, 0, 1, 0);
        glTranslatef(0,0,.35);
        glRotatef(-75,1,0,0);
        glScalef(1.0, 0.4, 1.0);
        
        glEnable(GL_TEXTURE_2D);
        gluSphere(_sphere, _pupilSize, 15, 15);
        glDisable(GL_TEXTURE_2D);
    }    
    glPopMatrix();


    // Left Eye
    glColor3fv(_eyeColor);
    glTranslatef(_interPupilDistance, 0, 0);
    glPushMatrix();
    {
        glTranslatef(-_interPupilDistance/10.0, 0, .05);
        glRotatef(-20, 0, 0, 1);
        glScalef(_eyeballScaleX, _eyeballScaleY, _eyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    
    // Left Pupil
    glPushMatrix();
    {
        glRotatef(_eyeballPitch[0], 1, 0, 0);
        glRotatef(_eyeballYaw[0] + _yaw - _pupilConverge, 0, 1, 0);
        glTranslatef(0, 0, .35);
        glRotatef(-75, 1, 0, 0);
        glScalef(1.0, 0.4, 1.0);
        
        glEnable(GL_TEXTURE_2D);
        gluSphere(_sphere, _pupilSize, 15, 15);
        glDisable(GL_TEXTURE_2D);
    }
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



