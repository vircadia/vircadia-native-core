//
//  Head.cpp
//  hifi
//
//  Created by Jeffrey on May, 10, 2013
//  Copyright (c) 2013 Physical, Inc.. All rights reserved.
//

#include "Head.h"
#include <vector>
#include <SharedUtil.h>
#include <lodepng.h>

using namespace std;

const float HEAD_MOTION_DECAY = 0.1;


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

void Head::initialize() {

    audioLoudness         = 0.0;
    skinColor             = glm::vec3(0.0f, 0.0f, 0.0f);
    position              = glm::vec3(0.0f, 0.0f, 0.0f);
    yaw                   = 0.0f;
    pitch                 = 0.0f;
    roll                  = 0.0f;
    pupilSize             = 0.10;
    interPupilDistance    = 0.6;
    interBrowDistance     = 0.75;
    nominalPupilSize      = 0.10;
    pitchRate             = 0.0;
    yawRate               = 0.0;
    rollRate              = 0.0;
    eyebrowPitch[0]       = -30;
    eyebrowPitch[1]       = -30;
    eyebrowRoll [0]       = 20;
    eyebrowRoll [1]       = -20;
    mouthPitch            = 0;
    mouthYaw              = 0;
    mouthWidth            = 1.0;
    mouthHeight           = 0.2;
    eyeballPitch[0]       = 0;
    eyeballPitch[1]       = 0;
    eyeballScaleX         = 1.2;
    eyeballScaleY         = 1.5;
    eyeballScaleZ         = 1.0;
    eyeballYaw[0]         = 0;
    eyeballYaw[1]         = 0;
    pitchTarget           = 0;
    yawTarget             = 0;
    noiseEnvelope         = 1.0;
    pupilConverge         = 10.0;
    leanForward           = 0.0;
    leanSideways          = 0.0;
    eyeContact            = 1;
    eyeContactTarget      = LEFT_EYE;
    scale                 = 1.0;
    audioAttack           = 0.0;
    averageLoudness       = 0.0;
    lastLoudness          = 0.0;
    browAudioLift         = 0.0;
    noise                 = 0;
    returnSpringScale     = 1.0;
    sphere                = NULL;
}

void Head::setPositionRotationAndScale(glm::vec3 p, glm::vec3 r, float s) {

    position = p;
    scale    = s;
    yaw      = r.x;
    pitch    = r.y;
    roll     = r.z;
}

void Head::setSkinColor(glm::vec3 c) {
    skinColor = c;
}

void Head::setAudioLoudness(float loudness) {
    audioLoudness = loudness;
}


void Head::setNewTarget(float pitch, float yaw) {
    pitchTarget = pitch;
    yawTarget   = yaw;
}

void Head::simulate(float deltaTime, bool isMine) {

    //  Decay head back to center if turned on
    if (isMine && returnHeadToCenter) {
        //  Decay back toward center
        pitch *= (1.0f - HEAD_MOTION_DECAY * returnSpringScale * 2 * deltaTime);
        yaw   *= (1.0f - HEAD_MOTION_DECAY * returnSpringScale * 2 * deltaTime);
        roll  *= (1.0f - HEAD_MOTION_DECAY * returnSpringScale * 2 * deltaTime);
    }
    
    //  For invensense gyro, decay only slightly when roughly centered
    if (isMine) {
        const float RETURN_RANGE = 15.0;
        const float RETURN_STRENGTH = 2.0;
        if (fabs(pitch) < RETURN_RANGE) { pitch *= (1.0f - RETURN_STRENGTH * deltaTime); }
        if (fabs(yaw)   < RETURN_RANGE) { yaw   *= (1.0f - RETURN_STRENGTH * deltaTime); }
        if (fabs(roll)  < RETURN_RANGE) { roll  *= (1.0f - RETURN_STRENGTH * deltaTime); }
    }

    if (noise) {
        //  Move toward new target
        pitch += (pitchTarget - pitch) * 10 * deltaTime; // (1.f - DECAY*deltaTime)*Pitch + ;
        yaw   += (yawTarget   - yaw  ) * 10 * deltaTime; // (1.f - DECAY*deltaTime);
        roll *= 1.f - (HEAD_MOTION_DECAY * deltaTime);
    }
    
    leanForward  *= (1.f - HEAD_MOTION_DECAY * 30 * deltaTime);
    leanSideways *= (1.f - HEAD_MOTION_DECAY * 30 * deltaTime);
        
    //  Update where the avatar's eyes are
    //
    //  First, decide if we are making eye contact or not
    if (randFloat() < 0.005) {
        eyeContact = !eyeContact;
        eyeContact = 1;
        if (!eyeContact) {
            //  If we just stopped making eye contact,move the eyes markedly away
            eyeballPitch[0] = eyeballPitch[1] = eyeballPitch[0] + 5.0 + (randFloat() - 0.5) * 10;
            eyeballYaw  [0] = eyeballYaw  [1] = eyeballYaw  [0] + 5.0 + (randFloat() - 0.5) * 5;
        } else {
            //  If now making eye contact, turn head to look right at viewer
            setNewTarget(0,0);
        }
    }
    
    const float DEGREES_BETWEEN_VIEWER_EYES = 3;
    const float DEGREES_TO_VIEWER_MOUTH = 7;
    
    if (eyeContact) {
        //  Should we pick a new eye contact target?
        if (randFloat() < 0.01) {
            //  Choose where to look next
            if (randFloat() < 0.1) {
                eyeContactTarget = MOUTH;
            } else {
                if (randFloat() < 0.5) eyeContactTarget = LEFT_EYE; else eyeContactTarget = RIGHT_EYE;
            }
        }
        //  Set eyeball pitch and yaw to make contact
        float eye_target_yaw_adjust = 0;
        float eye_target_pitch_adjust = 0;
        if (eyeContactTarget == LEFT_EYE) eye_target_yaw_adjust = DEGREES_BETWEEN_VIEWER_EYES;
        if (eyeContactTarget == RIGHT_EYE) eye_target_yaw_adjust = -DEGREES_BETWEEN_VIEWER_EYES;
        if (eyeContactTarget == MOUTH) eye_target_pitch_adjust = DEGREES_TO_VIEWER_MOUTH;
        
        eyeballPitch[0] = eyeballPitch[1] = -pitch + eye_target_pitch_adjust;
        eyeballYaw  [0] = eyeballYaw  [1] =  yaw   + eye_target_yaw_adjust;
    }
    
    if (noise)
    {
        pitch += (randFloat() - 0.5) * 0.2 * noiseEnvelope;
        yaw += (randFloat() - 0.5) * 0.3 *noiseEnvelope;
        //PupilSize += (randFloat() - 0.5) * 0.001*NoiseEnvelope;
        
        if (randFloat() < 0.005) mouthWidth = _MouthWidthChoices[rand()%3];
        
        if (!eyeContact) {
            if (randFloat() < 0.01)  eyeballPitch[0] = eyeballPitch[1] = (randFloat() - 0.5) * 20;
            if (randFloat() < 0.01)  eyeballYaw[0] = eyeballYaw[1] = (randFloat()- 0.5) * 10;
        }
        
        if ((randFloat() < 0.005) && (fabs(pitchTarget - pitch) < 1.0) && (fabs(yawTarget - yaw) < 1.0)) {
            setNewTarget((randFloat()-0.5) * 20.0, (randFloat()-0.5) * 45.0);
        }
        
        if (0) {
            
            //  Pick new target
            pitchTarget = (randFloat() - 0.5) * 45;
            yawTarget = (randFloat() - 0.5) * 22;
        }
        if (randFloat() < 0.01)
        {
            eyebrowPitch[0] = eyebrowPitch[1] = _BrowPitchAngle[rand()%3];
            eyebrowRoll [0] = eyebrowRoll[1] = _BrowRollAngle[rand()%5];
            eyebrowRoll [1] *=-1;
        }
    }
    
    //  Update audio trailing average for rendering facial animations
    const float AUDIO_AVERAGING_SECS = 0.05;
    averageLoudness = (1.f - deltaTime / AUDIO_AVERAGING_SECS) * averageLoudness +
                            (deltaTime / AUDIO_AVERAGING_SECS) * audioLoudness;
}

void Head::render(bool lookingInMirror, float bodyYaw) {

    int side = 0;
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
        
    glPushMatrix();
    
        glTranslatef(position.x, position.y, position.z);
	
        glScalef(scale, scale, scale);
    
    if (lookingInMirror) {
        glRotatef(bodyYaw   - yaw,   0, 1, 0);
        glRotatef(pitch, 1, 0, 0);   
        glRotatef(-roll,  0, 0, 1);
    } else {
        glRotatef(bodyYaw   + yaw,   0, 1, 0);
        glRotatef(pitch, 1, 0, 0);
        glRotatef(roll,  0, 0, 1);
    }
    
    //glScalef(2.0, 2.0, 2.0);
    glColor3f(skinColor.x, skinColor.y, skinColor.z);
    
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
    audioAttack = 0.9 * audioAttack + 0.1 * fabs(audioLoudness - lastLoudness);
    lastLoudness = audioLoudness;
    
    
    const float BROW_LIFT_THRESHOLD = 100;
    if (audioAttack > BROW_LIFT_THRESHOLD)
        browAudioLift += sqrt(audioAttack) / 1000.0;
    
    browAudioLift *= .90;
    
    //  Render Eyebrows
    glPushMatrix();
    glTranslatef(-interBrowDistance / 2.0,0.4,0.45);
    for(side = 0; side < 2; side++) {
        glColor3fv(_browColor);
        glPushMatrix();
        glTranslatef(0, 0.35 + browAudioLift, 0);
        glRotatef(eyebrowPitch[side]/2.0, 1, 0, 0);
        glRotatef(eyebrowRoll[side]/2.0, 0, 0, 1);
        glScalef(_browWidth, _browThickness, 1);
        glutSolidCube(0.5);
        glPopMatrix();
        glTranslatef(interBrowDistance, 0, 0);
    }
    glPopMatrix();
    
    // Mouth
    const float MIN_LOUDNESS_SCALE_WIDTH = 0.7f;
    const float WIDTH_SENSITIVITY = 60.f;
    const float HEIGHT_SENSITIVITY = 30.f;
    const float MIN_LOUDNESS_SCALE_HEIGHT = 1.0f;
    glPushMatrix();
    glTranslatef(0,-0.35,0.75);
    glColor3f(0,0,0);
    glRotatef(mouthPitch, 1, 0, 0);
    glRotatef(mouthYaw, 0, 0, 1);
    
    if ((averageLoudness > 1.f) && (averageLoudness < 10000.f)) {
        glScalef(mouthWidth * (MIN_LOUDNESS_SCALE_WIDTH + sqrt(averageLoudness) / WIDTH_SENSITIVITY),
                 mouthHeight * (MIN_LOUDNESS_SCALE_HEIGHT + sqrt(averageLoudness) / HEIGHT_SENSITIVITY), 1);
    } else {
        glScalef(mouthWidth, mouthHeight, 1);
    }

    glutSolidCube(0.5);
    glPopMatrix();
    
    glTranslatef(0, 1.0, 0);
    
    glTranslatef(-interPupilDistance/2.0,-0.68,0.7);
    // Right Eye
    glRotatef(-10, 1, 0, 0);
    glColor3fv(_eyeColor);
    glPushMatrix();
    {
        glTranslatef(interPupilDistance/10.0, 0, 0.05);
        glRotatef(20, 0, 0, 1);
        glScalef(eyeballScaleX, eyeballScaleY, eyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    
    // Right Pupil
    if (sphere == NULL) {
        sphere = gluNewQuadric();
        gluQuadricTexture(sphere, GL_TRUE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluQuadricOrientation(sphere, GLU_OUTSIDE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iris_texture_width, iris_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &iris_texture[0]);
    }
    
    glPushMatrix();
    {   
        glRotatef(eyeballPitch[1], 1, 0, 0);
        glRotatef(eyeballYaw[1] + yaw + pupilConverge, 0, 1, 0);
        glTranslatef(0,0,.35);
        glRotatef(-75,1,0,0);
        glScalef(1.0, 0.4, 1.0);
        
        glEnable(GL_TEXTURE_2D);
        gluSphere(sphere, pupilSize, 15, 15);
        glDisable(GL_TEXTURE_2D);
    }
    
    glPopMatrix();
    // Left Eye
    glColor3fv(_eyeColor);
    glTranslatef(interPupilDistance, 0, 0);
    glPushMatrix();
    {
        glTranslatef(-interPupilDistance/10.0, 0, .05);
        glRotatef(-20, 0, 0, 1);
        glScalef(eyeballScaleX, eyeballScaleY, eyeballScaleZ);
        glutSolidSphere(0.25, 30, 30);
    }
    glPopMatrix();
    // Left Pupil
    glPushMatrix();
    {
        glRotatef(eyeballPitch[0], 1, 0, 0);
        glRotatef(eyeballYaw[0] + yaw - pupilConverge, 0, 1, 0);
        glTranslatef(0, 0, .35);
        glRotatef(-75, 1, 0, 0);
        glScalef(1.0, 0.4, 1.0);
        
        glEnable(GL_TEXTURE_2D);
        gluSphere(sphere, pupilSize, 15, 15);
        glDisable(GL_TEXTURE_2D);
    }
    glPopMatrix();
    
    glPopMatrix();
    
}

