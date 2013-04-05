//
//  Head.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 Physical, Inc.. All rights reserved.
//

#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include <lodepng.h>
#include <fstream>
#include <sstream>
#include <SharedUtil.h>
#include "Head.h"

using namespace std;

float skinColor[] = {1.0, 0.84, 0.66};
float browColor[] = {210.0/255.0, 105.0/255.0, 30.0/255.0};
float mouthColor[] = {1, 0, 0};

float BrowRollAngle[5] = {0, 15, 30, -30, -15};
float BrowPitchAngle[3] = {-70, -60, -50};
float eyeColor[3] = {1,1,1};

float MouthWidthChoices[3] = {0.5, 0.77, 0.3};

float browWidth = 0.8;
float browThickness = 0.16;

const float DECAY = 0.1;

char iris_texture_file[] = "images/green_eye.png";

vector<unsigned char> iris_texture;
unsigned int iris_texture_width = 512;
unsigned int iris_texture_height = 256;



//---------------------------------------------------
Head::Head()
{
	initializeAvatar();

    position = glm::vec3(0,0,0);
    velocity = glm::vec3(0,0,0);
    thrust = glm::vec3(0,0,0);
    
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) driveKeys[i] = false; 
    
    PupilSize = 0.10;
    interPupilDistance = 0.6;
    interBrowDistance = 0.75;
    NominalPupilSize = 0.10;
    Yaw = 0.0;
    EyebrowPitch[0] = EyebrowPitch[1] = -30;
    EyebrowRoll[0] = 20;
    EyebrowRoll[1] = -20;
    MouthPitch = 0;
    MouthYaw = 0;
    MouthWidth = 1.0;
    MouthHeight = 0.2;
    EyeballPitch[0] = EyeballPitch[1] = 0;
    EyeballScaleX = 1.2;  EyeballScaleY = 1.5; EyeballScaleZ = 1.0;
    EyeballYaw[0] = EyeballYaw[1] = 0;
    PitchTarget = YawTarget = 0; 
    NoiseEnvelope = 1.0;
    PupilConverge = 10.0;
    leanForward = 0.0;
    leanSideways = 0.0;
    eyeContact = 1;
    eyeContactTarget = LEFT_EYE;
    scale = 1.0;
    renderYaw = 0.0;
    renderPitch = 0.0;
    audioAttack = 0.0;
    loudness = 0.0;
    averageLoudness = 0.0;
    lastLoudness = 0.0;
    browAudioLift = 0.0;
    noise = 0;
	
	handOffset.clear();
    
    sphere = NULL;
    
    hand = new Hand(glm::vec3(skinColor[0], skinColor[1], skinColor[2]));

    if (iris_texture.size() == 0) {
        switchToResourcesIfRequired();
        unsigned error = lodepng::decode(iris_texture, iris_texture_width, iris_texture_height, iris_texture_file);
        if (error != 0) {
            std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
        }
    }
}




//---------------------------------------------------
Head::Head(const Head &otherHead) 
{
	initializeAvatar();

    position = otherHead.position;
    velocity = otherHead.velocity;
    thrust = otherHead.thrust;
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) driveKeys[i] = otherHead.driveKeys[i];

    PupilSize = otherHead.PupilSize;
    interPupilDistance = otherHead.interPupilDistance;
    interBrowDistance = otherHead.interBrowDistance;
    NominalPupilSize = otherHead.NominalPupilSize;
    Yaw = otherHead.Yaw;
    EyebrowPitch[0] = otherHead.EyebrowPitch[0];
    EyebrowPitch[1] = otherHead.EyebrowPitch[1];
    EyebrowRoll[0] = otherHead.EyebrowRoll[0];
    EyebrowRoll[1] = otherHead.EyebrowRoll[1];
    MouthPitch = otherHead.MouthPitch;
    MouthYaw = otherHead.MouthYaw;
    MouthWidth = otherHead.MouthWidth;
    MouthHeight = otherHead.MouthHeight;
    EyeballPitch[0] = otherHead.EyeballPitch[0];
    EyeballPitch[1] = otherHead.EyeballPitch[1];
    EyeballScaleX = otherHead.EyeballScaleX;
    EyeballScaleY = otherHead.EyeballScaleY;
    EyeballScaleZ = otherHead.EyeballScaleZ;
    EyeballYaw[0] = otherHead.EyeballYaw[0];
    EyeballYaw[1] = otherHead.EyeballYaw[1];
    PitchTarget = otherHead.PitchTarget;
    YawTarget = otherHead.YawTarget;
    NoiseEnvelope = otherHead.NoiseEnvelope;
    PupilConverge = otherHead.PupilConverge;
    leanForward = otherHead.leanForward;
    leanSideways = otherHead.leanSideways;
    eyeContact = otherHead.eyeContact;
    eyeContactTarget = otherHead.eyeContactTarget;
    scale = otherHead.scale;
    renderYaw = otherHead.renderYaw;
    renderPitch = otherHead.renderPitch;
    audioAttack = otherHead.audioAttack;
    loudness = otherHead.loudness;
    averageLoudness = otherHead.averageLoudness;
    lastLoudness = otherHead.lastLoudness;
    browAudioLift = otherHead.browAudioLift;
    noise = otherHead.noise;
    
    sphere = NULL;
    
    Hand newHand = Hand(*otherHead.hand);
    hand = &newHand;
}




//---------------------------------------------------
Head::~Head() 
{
    if (sphere != NULL) 
	{
        gluDeleteQuadric(sphere);
    }
}



//---------------------------------------------------
Head* Head::clone() const 
{
    return new Head(*this);
}





//---------------------------------------------------
void Head::reset()
{
    Pitch = Yaw = Roll = 0;
    leanForward = leanSideways = 0;
}





//---------------------------------------------------
void Head::UpdatePos(float frametime, SerialInterface * serialInterface, int head_mirror, glm::vec3 * gravity)
//  Using serial data, update avatar/render position and angles
{
    
    const float PITCH_ACCEL_COUPLING = 0.5;
    const float ROLL_ACCEL_COUPLING = -1.0;
    float measured_pitch_rate = serialInterface->getRelativeValue(PITCH_RATE);
    YawRate = serialInterface->getRelativeValue(YAW_RATE);
    float measured_lateral_accel = serialInterface->getRelativeValue(ACCEL_X) -
                                ROLL_ACCEL_COUPLING*serialInterface->getRelativeValue(ROLL_RATE);
    float measured_fwd_accel = serialInterface->getRelativeValue(ACCEL_Z) -
                                PITCH_ACCEL_COUPLING*serialInterface->getRelativeValue(PITCH_RATE);
    float measured_roll_rate = serialInterface->getRelativeValue(ROLL_RATE);
    
    //std::cout << "Pitch Rate: " << serialInterface->getRelativeValue(PITCH_RATE) <<
    //    " fwd_accel: " << serialInterface->getRelativeValue(ACCEL_Z) << "\n";
    //std::cout << "Roll Rate: " << serialInterface->getRelativeValue(ROLL_RATE) <<
    //" ACCEL_X: " << serialInterface->getRelativeValue(ACCEL_X) << "\n";
    //std::cout << "Pitch: " << Pitch << "\n";
    
    //  Update avatar head position based on measured gyro rates
    const float HEAD_ROTATION_SCALE = 0.70;
    const float HEAD_ROLL_SCALE = 0.40;
    const float HEAD_LEAN_SCALE = 0.01;
    const float MAX_PITCH = 45;
    const float MIN_PITCH = -45;
    const float MAX_YAW = 85;
    const float MIN_YAW = -85;

    if ((Pitch < MAX_PITCH) && (Pitch > MIN_PITCH))
        addPitch(measured_pitch_rate * -HEAD_ROTATION_SCALE * frametime);
    addRoll(-measured_roll_rate * HEAD_ROLL_SCALE * frametime);

    if (head_mirror) {
        if ((Yaw < MAX_YAW) && (Yaw > MIN_YAW))
            addYaw(-YawRate * HEAD_ROTATION_SCALE * frametime);
        addLean(-measured_lateral_accel * frametime * HEAD_LEAN_SCALE, -measured_fwd_accel*frametime * HEAD_LEAN_SCALE);
    } else {
        if ((Yaw < MAX_YAW) && (Yaw > MIN_YAW))
            addYaw(YawRate * -HEAD_ROTATION_SCALE * frametime);
        addLean(measured_lateral_accel * frametime * -HEAD_LEAN_SCALE, measured_fwd_accel*frametime * HEAD_LEAN_SCALE);        
    } 
}




//---------------------------------------------------
void Head::setAvatarPosition( double x, double y, double z )
{
	avatar.position.setXYZ( x, y, z );
}


//---------------------------------------------------
void Head::addLean(float x, float z) {
    //  Add Body lean as impulse 
    leanSideways += x;
    leanForward += z;
}



//---------------------------------------------------
void Head::setLeanForward(float dist){
    leanForward = dist;
}




//---------------------------------------------------
void Head::setLeanSideways(float dist){
    leanSideways = dist;
}






//  Simulate the head over time 
//---------------------------------------------------
void Head::simulate(float deltaTime)
{
	simulateAvatar( deltaTime );
	
    glm::vec3 forward(-sinf(getRenderYaw()*PI/180),
                      sinf(getRenderPitch()*PI/180),
                      cosf(getRenderYaw()*PI/180));
    
    thrust = glm::vec3(0);
    const float THRUST_MAG = 10.0;
    const float THRUST_LATERAL_MAG = 10.0;
    const float THRUST_VERTICAL_MAG = 10.0;
    
    if (driveKeys[FWD]) {
        thrust += THRUST_MAG*forward;
    }
    if (driveKeys[BACK]) {
        thrust += -THRUST_MAG*forward;
    }
    if (driveKeys[RIGHT]) {
        thrust.x += forward.z*-THRUST_LATERAL_MAG;
        thrust.z += forward.x*THRUST_LATERAL_MAG;
    }
    if (driveKeys[LEFT]) {
        thrust.x += forward.z*THRUST_LATERAL_MAG;
        thrust.z += forward.x*-THRUST_LATERAL_MAG;
    }
    if (driveKeys[UP]) {
        thrust.y += -THRUST_VERTICAL_MAG;
    }
    if (driveKeys[DOWN]) {
        thrust.y += THRUST_VERTICAL_MAG;
    }

    //  Increment velocity as time
    velocity += thrust * deltaTime;

    //  Increment position as a function of velocity
    position += velocity * deltaTime;
    
    //  Decay velocity
    const float LIN_VEL_DECAY = 5.0;
    velocity *= (1.0 - LIN_VEL_DECAY*deltaTime);
    
    if (!noise)
    {
        //  Decay back toward center 
        Pitch *= (1.f - DECAY*2*deltaTime);
        Yaw *= (1.f - DECAY*2*deltaTime);
        Roll *= (1.f - DECAY*2*deltaTime);
    }
    else {
        //  Move toward new target  
        Pitch += (PitchTarget - Pitch)*10*deltaTime;   // (1.f - DECAY*deltaTime)*Pitch + ;
        Yaw += (YawTarget - Yaw)*10*deltaTime; //  (1.f - DECAY*deltaTime);
        Roll *= (1.f - DECAY*deltaTime);
    }
    
    leanForward *= (1.f - DECAY*30.f*deltaTime);
    leanSideways *= (1.f - DECAY*30.f*deltaTime);
    
    //  Update where the avatar's eyes are 
    //
    //  First, decide if we are making eye contact or not
    if (randFloat() < 0.005) {
        eyeContact = !eyeContact;
        eyeContact = 1;
        if (!eyeContact) {
            //  If we just stopped making eye contact,move the eyes markedly away
            EyeballPitch[0] = EyeballPitch[1] = EyeballPitch[0] + 5.0 + (randFloat() - 0.5)*10;
            EyeballYaw[0] = EyeballYaw[1] = EyeballYaw[0] + 5.0 + (randFloat()- 0.5)*5;
        } else {
            //  If now making eye contact, turn head to look right at viewer
            SetNewHeadTarget(0,0);
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
        
        EyeballPitch[0] = EyeballPitch[1] = -Pitch + eye_target_pitch_adjust;
        EyeballYaw[0] = EyeballYaw[1] = -Yaw + eye_target_yaw_adjust;
    }

    
    if (noise)
    {
        Pitch += (randFloat() - 0.5)*0.2*NoiseEnvelope;
        Yaw += (randFloat() - 0.5)*0.3*NoiseEnvelope;
        //PupilSize += (randFloat() - 0.5)*0.001*NoiseEnvelope;
        
        if (randFloat() < 0.005) MouthWidth = MouthWidthChoices[rand()%3];
        
        if (!eyeContact) {
            if (randFloat() < 0.01)  EyeballPitch[0] = EyeballPitch[1] = (randFloat() - 0.5)*20;
            if (randFloat() < 0.01)  EyeballYaw[0] = EyeballYaw[1] = (randFloat()- 0.5)*10;
        }         

        if ((randFloat() < 0.005) && (fabs(PitchTarget - Pitch) < 1.0) && (fabs(YawTarget - Yaw) < 1.0))
        {
            SetNewHeadTarget((randFloat()-0.5)*20.0, (randFloat()-0.5)*45.0);
        }

        if (0)
        {

            //  Pick new target
            PitchTarget = (randFloat() - 0.5)*45;
            YawTarget = (randFloat() - 0.5)*22;
        }
        if (randFloat() < 0.01)
        {
            EyebrowPitch[0] = EyebrowPitch[1] = BrowPitchAngle[rand()%3];
            EyebrowRoll[0] = EyebrowRoll[1] = BrowRollAngle[rand()%5];
            EyebrowRoll[1]*=-1;
        }
                         
    }
//hand->simulate(deltaTime);
}
      
	  
	  
	  
	   
//---------------------------------------------------
void Head::render(int faceToFace, int isMine)
{
	// render avatar 
	renderAvatar();

    int side = 0;
        
    //  Always render own hand, but don't render head unless showing face2face
    glEnable(GL_DEPTH_TEST);
    glPushMatrix();
    
//glScalef(scale, scale, scale);


glTranslatef
( 
	avatar.bone[ AVATAR_BONE_HEAD ].position.x, 
	avatar.bone[ AVATAR_BONE_HEAD ].position.y, 
	avatar.bone[ AVATAR_BONE_HEAD ].position.z 
);


glScalef( 0.03, 0.03, 0.03 );


	glTranslatef(leanSideways, 0.f, leanForward);
    
    glRotatef(Yaw, 0, 1, 0);
    
//hand->render(1);
    
    //  Don't render a head if it is really close to your location, because that is your own head!
//if (!isMine || faceToFace) 
	{
        
        glRotatef(Pitch, 1, 0, 0);
        glRotatef(Roll, 0, 0, 1);
        
        
        // Overall scale of head
        if (faceToFace) glScalef(1.5, 2.0, 2.0);
        else glScalef(0.75, 1.0, 1.0);
		
        glColor3fv(skinColor);


        //  Head
        glutSolidSphere(1, 30, 30);
        
        //std::cout << distanceToCamera << "\n";
        
        //  Ears
        glPushMatrix();
            glTranslatef(1.0, 0, 0);
            for(side = 0; side < 2; side++)
            {
                glPushMatrix();
                    glScalef(0.3, 0.65, .65);
                    glutSolidSphere(0.5, 30, 30);  
                glPopMatrix();
                glTranslatef(-2.0, 0, 0);
            }
        glPopMatrix();

        // Eyebrows
        audioAttack = 0.9*audioAttack + 0.1*fabs(loudness - lastLoudness);
        lastLoudness = loudness;
    
        const float BROW_LIFT_THRESHOLD = 100;
        if (audioAttack > BROW_LIFT_THRESHOLD)
            browAudioLift += sqrt(audioAttack)/1000.0;
        
        browAudioLift *= .90;
        
        glPushMatrix();
            glTranslatef(-interBrowDistance/2.0,0.4,0.45);
            for(side = 0; side < 2; side++)
            {
                glColor3fv(browColor);
                glPushMatrix();
                    glTranslatef(0, 0.35 + browAudioLift, 0);
                    glRotatef(EyebrowPitch[side]/2.0, 1, 0, 0);
                    glRotatef(EyebrowRoll[side]/2.0, 0, 0, 1);
                    glScalef(browWidth, browThickness, 1);
                    glutSolidCube(0.5);
                glPopMatrix();
                glTranslatef(interBrowDistance, 0, 0);
            }
        glPopMatrix();
        
        
        // Mouth
        
        glPushMatrix();
            glTranslatef(0,-0.35,0.75);
            glColor3f(0,0,0);
            glRotatef(MouthPitch, 1, 0, 0);
            glRotatef(MouthYaw, 0, 0, 1);
            glScalef(MouthWidth*(.7 + sqrt(averageLoudness)/60.0), MouthHeight*(1.0 + sqrt(averageLoudness)/30.0), 1);
            glutSolidCube(0.5);
        glPopMatrix();
        
        glTranslatef(0, 1.0, 0);
       
        glTranslatef(-interPupilDistance/2.0,-0.68,0.7);
        // Right Eye
        glRotatef(-10, 1, 0, 0);
        glColor3fv(eyeColor);
        glPushMatrix(); 
        {
            glTranslatef(interPupilDistance/10.0, 0, 0.05);
            glRotatef(20, 0, 0, 1);
            glScalef(EyeballScaleX, EyeballScaleY, EyeballScaleZ);
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
            glRotatef(EyeballPitch[1], 1, 0, 0);
            glRotatef(EyeballYaw[1] + PupilConverge, 0, 1, 0);
            glTranslatef(0,0,.35);
            glRotatef(-75,1,0,0);
            glScalef(1.0, 0.4, 1.0);
            
            glEnable(GL_TEXTURE_2D);
            gluSphere(sphere, PupilSize, 15, 15);
            glDisable(GL_TEXTURE_2D);
        }

        glPopMatrix();
        // Left Eye
        glColor3fv(eyeColor);
        glTranslatef(interPupilDistance, 0, 0);
        glPushMatrix(); 
        {
            glTranslatef(-interPupilDistance/10.0, 0, .05);
            glRotatef(-20, 0, 0, 1);
            glScalef(EyeballScaleX, EyeballScaleY, EyeballScaleZ);
            glutSolidSphere(0.25, 30, 30);
        }
        glPopMatrix();
        // Left Pupil
        glPushMatrix();
        {
            glRotatef(EyeballPitch[0], 1, 0, 0);
            glRotatef(EyeballYaw[0] - PupilConverge, 0, 1, 0);
            glTranslatef(0, 0, .35);
            glRotatef(-75, 1, 0, 0);
            glScalef(1.0, 0.4, 1.0);

            glEnable(GL_TEXTURE_2D);
            gluSphere(sphere, PupilSize, 15, 15);
            glDisable(GL_TEXTURE_2D);
        }
        
        glPopMatrix();

    }
    glPopMatrix();
 }
 
 
 

//---------------------------------------------------------
void Head::setHandMovement( glm::dvec3 movement )
{
	handOffset.setXYZ( movement.x, -movement.y, movement.z );
}



//-----------------------------------------
void Head::initializeAvatar()
{
	//printf( "initializeAvatar\n" );

	avatar.position.clear();
	//avatar.position.setXYZ( -3.0, 0.0, 0.0 );

	avatar.velocity.clear();
	avatar.orientation.setToIdentity();
	
	for (int b=0; b<NUM_AVATAR_BONES; b++)
	{
		avatar.bone[b].position.clear();
		avatar.bone[b].velocity.clear();
		avatar.bone[b].orientation.setToIdentity();
	}

	//----------------------------------------------------------------------------
	// parental hierarchy
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	// spine and head
	//----------------------------------------------------------------------------
	avatar.bone[ AVATAR_BONE_PELVIS_SPINE		].parent = AVATAR_BONE_NULL;
	avatar.bone[ AVATAR_BONE_MID_SPINE			].parent = AVATAR_BONE_PELVIS_SPINE;
	avatar.bone[ AVATAR_BONE_CHEST_SPINE		].parent = AVATAR_BONE_MID_SPINE;
	avatar.bone[ AVATAR_BONE_NECK				].parent = AVATAR_BONE_CHEST_SPINE;
	avatar.bone[ AVATAR_BONE_HEAD				].parent = AVATAR_BONE_NECK;
	
	//----------------------------------------------------------------------------
	// left chest and arm
	//----------------------------------------------------------------------------
	avatar.bone[ AVATAR_BONE_LEFT_CHEST			].parent = AVATAR_BONE_MID_SPINE;
	avatar.bone[ AVATAR_BONE_LEFT_SHOULDER		].parent = AVATAR_BONE_LEFT_CHEST;
	avatar.bone[ AVATAR_BONE_LEFT_UPPER_ARM		].parent = AVATAR_BONE_LEFT_SHOULDER;
	avatar.bone[ AVATAR_BONE_LEFT_FOREARM		].parent = AVATAR_BONE_LEFT_UPPER_ARM;
	avatar.bone[ AVATAR_BONE_LEFT_HAND			].parent = AVATAR_BONE_LEFT_FOREARM;

	//----------------------------------------------------------------------------
	// right chest and arm
	//----------------------------------------------------------------------------
	avatar.bone[ AVATAR_BONE_RIGHT_CHEST		].parent = AVATAR_BONE_MID_SPINE;
	avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER		].parent = AVATAR_BONE_RIGHT_CHEST;
	avatar.bone[ AVATAR_BONE_RIGHT_UPPER_ARM	].parent = AVATAR_BONE_RIGHT_SHOULDER;
	avatar.bone[ AVATAR_BONE_RIGHT_FOREARM		].parent = AVATAR_BONE_RIGHT_UPPER_ARM;
	avatar.bone[ AVATAR_BONE_RIGHT_HAND			].parent = AVATAR_BONE_RIGHT_FOREARM;
	
	//----------------------------------------------------------------------------
	// left pelvis and leg
	//----------------------------------------------------------------------------
	avatar.bone[ AVATAR_BONE_LEFT_PELVIS		].parent = AVATAR_BONE_NULL;
	avatar.bone[ AVATAR_BONE_LEFT_THIGH			].parent = AVATAR_BONE_LEFT_PELVIS;
	avatar.bone[ AVATAR_BONE_LEFT_SHIN			].parent = AVATAR_BONE_LEFT_THIGH;
	avatar.bone[ AVATAR_BONE_LEFT_FOOT			].parent = AVATAR_BONE_LEFT_SHIN;

	//----------------------------------------------------------------------------
	// right pelvis and leg
	//----------------------------------------------------------------------------
	avatar.bone[ AVATAR_BONE_RIGHT_PELVIS		].parent = AVATAR_BONE_NULL;
	avatar.bone[ AVATAR_BONE_RIGHT_THIGH		].parent = AVATAR_BONE_RIGHT_PELVIS;
	avatar.bone[ AVATAR_BONE_RIGHT_SHIN			].parent = AVATAR_BONE_RIGHT_THIGH;
	avatar.bone[ AVATAR_BONE_RIGHT_FOOT			].parent = AVATAR_BONE_RIGHT_SHIN;


	//----------------------------------------------------------
	// default pose position
	//----------------------------------------------------------
	avatar.bone[ AVATAR_BONE_PELVIS_SPINE		].defaultPosePosition.setXYZ(  0.0,   0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_MID_SPINE			].defaultPosePosition.setXYZ(  0.0,   0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_CHEST_SPINE		].defaultPosePosition.setXYZ(  0.0,   0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_NECK				].defaultPosePosition.setXYZ(  0.0,   0.06, 0.0  );
	avatar.bone[ AVATAR_BONE_HEAD				].defaultPosePosition.setXYZ(  0.0,   0.06, 0.0  );

	avatar.bone[ AVATAR_BONE_LEFT_CHEST			].defaultPosePosition.setXYZ( -0.06,  0.06, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_SHOULDER		].defaultPosePosition.setXYZ( -0.03,  0.0,  0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_UPPER_ARM		].defaultPosePosition.setXYZ(  0.0,  -0.12, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_FOREARM		].defaultPosePosition.setXYZ(  0.0,  -0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_HAND			].defaultPosePosition.setXYZ(  0.0,  -0.05, 0.0  );
	
	avatar.bone[ AVATAR_BONE_RIGHT_CHEST		].defaultPosePosition.setXYZ(  0.06,  0.06, 0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER		].defaultPosePosition.setXYZ(  0.03,  0.0,  0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_UPPER_ARM	].defaultPosePosition.setXYZ(  0.0,  -0.12, 0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_FOREARM		].defaultPosePosition.setXYZ(  0.0,  -0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_HAND			].defaultPosePosition.setXYZ(  0.0,  -0.05, 0.0  );
	
	avatar.bone[ AVATAR_BONE_LEFT_PELVIS		].defaultPosePosition.setXYZ( -0.05,  0.0,  0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_THIGH			].defaultPosePosition.setXYZ(  0.0,  -0.15, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_SHIN			].defaultPosePosition.setXYZ(  0.0,  -0.15, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_FOOT			].defaultPosePosition.setXYZ(  0.0,   0.0,  0.04 );

	avatar.bone[ AVATAR_BONE_RIGHT_PELVIS		].defaultPosePosition.setXYZ(  0.05,  0.0,  0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_THIGH		].defaultPosePosition.setXYZ(  0.0,  -0.15, 0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_SHIN			].defaultPosePosition.setXYZ(  0.0,  -0.15, 0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_FOOT			].defaultPosePosition.setXYZ(  0.0,   0.0,  0.04 );


	//----------------------------------------------------------------------------
	// length
	//----------------------------------------------------------------------------
	avatar.bone[ AVATAR_BONE_PELVIS_SPINE		].length = 0.1;
	avatar.bone[ AVATAR_BONE_MID_SPINE			].length = 0.1;
	avatar.bone[ AVATAR_BONE_CHEST_SPINE		].length = 0.1;
	avatar.bone[ AVATAR_BONE_NECK				].length = 0.1;
	avatar.bone[ AVATAR_BONE_HEAD				].length = 0.1;

	avatar.bone[ AVATAR_BONE_LEFT_CHEST			].length = 0.1;
	avatar.bone[ AVATAR_BONE_LEFT_SHOULDER		].length = 0.1;
	avatar.bone[ AVATAR_BONE_LEFT_UPPER_ARM		].length = 0.1;
	avatar.bone[ AVATAR_BONE_LEFT_FOREARM		].length = 0.1;
	avatar.bone[ AVATAR_BONE_LEFT_HAND			].length = 0.1;
	
	avatar.bone[ AVATAR_BONE_RIGHT_CHEST		].length = 0.1;
	avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER		].length = 0.1;
	avatar.bone[ AVATAR_BONE_RIGHT_UPPER_ARM	].length = 0.1;
	avatar.bone[ AVATAR_BONE_RIGHT_FOREARM		].length = 0.1;
	avatar.bone[ AVATAR_BONE_RIGHT_HAND			].length = 0.1;
	
	avatar.bone[ AVATAR_BONE_LEFT_PELVIS		].length = 0.1;
	avatar.bone[ AVATAR_BONE_LEFT_THIGH			].length = 0.1;
	avatar.bone[ AVATAR_BONE_LEFT_SHIN			].length = 0.1;
	avatar.bone[ AVATAR_BONE_LEFT_FOOT			].length = 0.1;
	
	avatar.bone[ AVATAR_BONE_RIGHT_PELVIS		].length = 0.1;
	avatar.bone[ AVATAR_BONE_RIGHT_THIGH		].length = 0.1;
	avatar.bone[ AVATAR_BONE_RIGHT_SHIN			].length = 0.1;
	avatar.bone[ AVATAR_BONE_RIGHT_FOOT			].length = 0.1;
		
	updateAvatarSkeleton();
}


//-----------------------------------------
void Head::simulateAvatar( float deltaTime )
{
	updateAvatarSkeleton();
}



//-----------------------------------------
void Head::updateAvatarSkeleton()
{
	//------------------------------------------------------------------------
	// calculate positions of all bones by traversing the skeleton tree:
	//------------------------------------------------------------------------
	for (int b=0; b<NUM_AVATAR_BONES; b++)
	{
		if ( avatar.bone[b].parent == AVATAR_BONE_NULL )
		{
			avatar.bone[b].position.set( avatar.position );
		}
		else
		{
			avatar.bone[b].position.set( avatar.bone[ avatar.bone[b].parent ].position );
		}
		
		avatar.bone[b].position.add( avatar.bone[b].defaultPosePosition );
	}	
	
	
	//----------------------------------------------------------------
	// adjust right hand and elbow according to hand offset
	//----------------------------------------------------------------
	avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.add( handOffset );	

	glm::dvec3 armVector = glm::dvec3( avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.x, avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.y, avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.z );

	armVector -= glm::dvec3( avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position.x, avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position.y, avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position.z );

	//-------------------------------------------------------------------------------
	// test to see if right hand is being dragged beyond maximum arm length
	//-------------------------------------------------------------------------------
	double distance = glm::length( armVector );
	double maxArmLength = 0.27;
	
	//-------------------------------------------------------------------------------
	// right hand is being dragged beyond maximum arm length...
	//-------------------------------------------------------------------------------
	if ( distance > maxArmLength )
	{
		//-------------------------------------------------------------------------------
		// reset right hand to be constrained to maximum arm length
		//-------------------------------------------------------------------------------
		avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.set( avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position );
				
		glm::dvec3 armNormal = armVector / distance;
		armVector = armNormal * maxArmLength;
		distance = maxArmLength;
		
		glm::dvec3 constrainedPosition = glm::dvec3( avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position.x, avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position.y, avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position.z );
		
		constrainedPosition += armVector;
		
		avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.setXYZ( constrainedPosition.x, constrainedPosition.y, constrainedPosition.z );
	}
	
	
	//-----------------------------------------------------------------------------
	// set elbow position
	//-----------------------------------------------------------------------------
	glm::dvec3 newElbowPosition = glm::dvec3( avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position.x, avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position.y, avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position.z );
	
	newElbowPosition += armVector * ONE_HALF;
	
	glm::dvec3 perpendicular = glm::dvec3( -armVector.y, armVector.x, armVector.z );

	newElbowPosition += perpendicular * ( 1.0 - ( maxArmLength / distance ) ) * ONE_HALF;
	
	avatar.bone[ AVATAR_BONE_RIGHT_FOREARM ].position.setXYZ( newElbowPosition.x, newElbowPosition.y, newElbowPosition.z );
}



//-----------------------------------------
void Head::renderAvatar()
{
	glColor3fv(skinColor);

	for (int b=0; b<NUM_AVATAR_BONES; b++)
	{
		glPushMatrix();
			glTranslatef( avatar.bone[b].position.x, avatar.bone[b].position.y, avatar.bone[b].position.z );
			glScalef( 0.02, 0.02, 0.02 );
			glutSolidSphere( 1, 8, 4 );
		glPopMatrix();
	}
}





//  Transmit data to agents requesting it 
//---------------------------------------------------
int Head::getBroadcastData(char* data)
{
    // Copy data for transmission to the buffer, return length of data
    sprintf(data, "H%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
            getRenderPitch() + Pitch, -getRenderYaw() + 180 -Yaw, Roll,
            position.x + leanSideways, position.y, position.z + leanForward,
            loudness, averageLoudness,
            hand->getPos().x, hand->getPos().y, hand->getPos().z);
    return strlen(data);
}

//---------------------------------------------------
void Head::parseData(void *data, int size) {
    // parse head data for this agent
    glm::vec3 handPos(0,0,0);
    sscanf((char *)data, "H%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
           &Pitch, &Yaw, &Roll,
           &position.x, &position.y, &position.z,
           &loudness, &averageLoudness,
           &handPos.x, &handPos.y, &handPos.z);
		   
    if (glm::length(handPos) > 0.0) hand->setPos(handPos);
}

//---------------------------------------------------
void Head::SetNewHeadTarget(float pitch, float yaw)
{
    PitchTarget = pitch;
    YawTarget = yaw;
}
