//
//  Head.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//	adapted by Jeffrey Ventrella, starting on April 2, 2013
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
float lightBlue[] = { 0.7, 0.8, 1.0 };
float browColor[] = {210.0/255.0, 105.0/255.0, 30.0/255.0};
float mouthColor[] = {1, 0, 0};

float BrowRollAngle[5] = {0, 15, 30, -30, -15};
float BrowPitchAngle[3] = {-70, -60, -50};
float eyeColor[3] = {1,1,1};

float MouthWidthChoices[3] = {0.5, 0.77, 0.3};

float browWidth = 0.8;
float browThickness = 0.16;

const float DECAY = 0.1;

char iris_texture_file[] = "resources/images/green_eye.png";

vector<unsigned char> iris_texture;
unsigned int iris_texture_width = 512;
unsigned int iris_texture_height = 256;



//---------------------------------------------------
Head::Head()
{
	initializeAvatar();

    //position	= glm::vec3(0,0,0);
    //velocity	= glm::vec3(0,0,0);
    thrust		= glm::vec3(0,0,0);
    
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
	
	handBeingMoved = false;
	movedHandOffset = glm::vec3( 0.0, 0.0, 0.0 );
    
    sphere = NULL;
	
	springForce				= 6.0f;
	springToBodyTightness	= 4.0f;
	springVelocityDecay		= 1.0f;
    
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
    //velocity = otherHead.velocity;
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




//this pertains to moving the head with the glasses 
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






//  Simulate the avatar over time 
//---------------------------------------------------
void Head::simulate(float deltaTime)
{
	updateAvatarSkeleton();
	
	//------------------------------------------------------------------------
	// update springy behavior:
	//------------------------------------------------------------------------
	updateAvatarSprings( deltaTime );
		
    const float THRUST_MAG	= 10.0;
    const float YAW_MAG		= 300.0;
	
	avatar.thrust = glm::vec3( 0.0, 0.0, 0.0 );
		
	//notice that the z values from avatar.orientation are flipped to accommodate different coordinate system
    if (driveKeys[FWD]) 
	{
		glm::vec3 front( avatar.orientation.getFront().x, avatar.orientation.getFront().y, avatar.orientation.getFront().z );
		avatar.thrust += front * THRUST_MAG;
    }
    if (driveKeys[BACK]) 
	{
		glm::vec3 front( avatar.orientation.getFront().x, avatar.orientation.getFront().y, avatar.orientation.getFront().z );
		avatar.thrust -= front * THRUST_MAG;
    }
    if (driveKeys[RIGHT]) 
	{
		glm::vec3 right( avatar.orientation.getRight().x, avatar.orientation.getRight().y, avatar.orientation.getRight().z );
		avatar.thrust += right * THRUST_MAG;
    }
    if (driveKeys[LEFT]) 
	{
		glm::vec3 right( avatar.orientation.getRight().x, avatar.orientation.getRight().y, avatar.orientation.getRight().z );
		avatar.thrust -= right * THRUST_MAG;
    }
    if (driveKeys[UP])
	{
		glm::vec3 up( avatar.orientation.getUp().x, avatar.orientation.getUp().y, avatar.orientation.getUp().z );
		avatar.thrust += up * THRUST_MAG;
    }
    if (driveKeys[DOWN]) 
	{
		glm::vec3 up( avatar.orientation.getUp().x, avatar.orientation.getUp().y, avatar.orientation.getUp().z );
		avatar.thrust -= up * THRUST_MAG;
    }
	
    if (driveKeys[ROT_RIGHT]) 
	{	
		avatar.yawDelta -= YAW_MAG * deltaTime;
    }
    if (driveKeys[ROT_LEFT]) 
	{	
		avatar.yawDelta += YAW_MAG * deltaTime;
    }
	
	avatar.yaw += avatar.yawDelta * deltaTime;
	
	Yaw = avatar.yaw;
	
    const float TEST_YAW_DECAY = 5.0;
    avatar.yawDelta *= ( 1.0 - TEST_YAW_DECAY * deltaTime );

	avatar.velocity += glm::dvec3( avatar.thrust * deltaTime );
		
	position += (glm::vec3)avatar.velocity * deltaTime;
	//avatar.position += (glm::vec3)avatar.velocity * deltaTime;
	//position = avatar.position;
	
	//avatar.velocity *= 0.9;

    const float LIN_VEL_DECAY = 5.0;
    avatar.velocity *= ( 1.0 - LIN_VEL_DECAY * deltaTime );
	
	
	/*
    //  Increment velocity as time
    velocity += thrust * deltaTime;

    //  Increment position as a function of velocity
    position += velocity * deltaTime;
    */
	
	
	/*
    //  Decay velocity
    const float LIN_VEL_DECAY = 5.0;
    velocity *= (1.0 - LIN_VEL_DECAY*deltaTime);
	*/
	
	
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
	glPushMatrix();
		glTranslatef( position.x, position.y, position.z );
		glScalef( 0.03, 0.03, 0.03 );
        glutSolidSphere( 1, 10, 10 );
	glPopMatrix();
	
	renderOrientationDirections(avatar.bone[ AVATAR_BONE_HEAD ].position, avatar.bone[ AVATAR_BONE_HEAD ].orientation, 0.2f );
	
	renderBody();
	renderHead( faceToFace, isMine );
}


	   
//---------------------------------------------------
void Head::renderOrientationDirections( glm::vec3 position, Orientation orientation, float size )
{
	glm::vec3 pRight	= position + orientation.getRight	() * size;
	glm::vec3 pUp		= position + orientation.getUp		() * size;
	glm::vec3 pFront	= position + orientation.getFront	() * size;
		
	glColor3f( 1.0f, 0.0f, 0.0f );
	glBegin( GL_LINE_STRIP );
	glVertex3f( avatar.bone[ AVATAR_BONE_HEAD ].position.x, avatar.bone[ AVATAR_BONE_HEAD ].position.y, avatar.bone[ AVATAR_BONE_HEAD ].position.z );
	glVertex3f( pRight.x, pRight.y, pRight.z );
	glEnd();

	glColor3f( 0.0f, 1.0f, 0.0f );
	glBegin( GL_LINE_STRIP );
	glVertex3f( avatar.bone[ AVATAR_BONE_HEAD ].position.x, avatar.bone[ AVATAR_BONE_HEAD ].position.y, avatar.bone[ AVATAR_BONE_HEAD ].position.z );
	glVertex3f( pUp.x, pUp.y, pUp.z );
	glEnd();

	glColor3f( 0.0f, 0.0f, 1.0f );
	glBegin( GL_LINE_STRIP );
	glVertex3f( avatar.bone[ AVATAR_BONE_HEAD ].position.x, avatar.bone[ AVATAR_BONE_HEAD ].position.y, avatar.bone[ AVATAR_BONE_HEAD ].position.z );
	glVertex3f( pFront.x, pFront.y, pFront.z );
	glEnd();
}

	  
	   
//---------------------------------------------------
void Head::renderHead( int faceToFace, int isMine )
{
    int side = 0;
        
    //  Always render own hand, but don't render head unless showing face2face
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
    glPushMatrix();
    
	//glScalef(scale, scale, scale);

	glTranslatef
	( 
		avatar.bone[ AVATAR_BONE_HEAD ].position.x, 
		avatar.bone[ AVATAR_BONE_HEAD ].position.y, 
		avatar.bone[ AVATAR_BONE_HEAD ].position.z 
	);

	glScalef( 0.03, 0.03, 0.03 );


	//glTranslatef(leanSideways, 0.f, leanForward);
    
	//glRotatef(Yaw, 0, 1, 0);

    glRotatef( avatar.yaw, 0, 1, 0);
    
	//hand->render(1);
    
    //  Don't render a head if it is really close to your location, because that is your own head!
	//if (!isMine || faceToFace) 
	{
        
        glRotatef(Pitch, 1, 0, 0);
        glRotatef(Roll, 0, 0, 1);
        
        // Overall scale of head
        if (faceToFace) glScalef(2.0, 2.0, 2.0);
        else glScalef(0.75, 1.0, 1.0);
		
        glColor3fv(skinColor);

        //  Head
        glutSolidSphere(1, 30, 30);
                
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
void Head::setHandMovement( glm::vec3 movement )
{
	handBeingMoved = true;
	movedHandOffset = movement;
}



//-----------------------------------------
void Head::initializeAvatar()
{
	//avatar.position		= glm::vec3( 0.0, 0.0, 0.0 );
	avatar.velocity		= glm::vec3( 0.0, 0.0, 0.0 );
	avatar.thrust		= glm::vec3( 0.0, 0.0, 0.0 );
	avatar.orientation.setToIdentity();
	
	avatar.yaw		= -90.0;
	avatar.pitch	= 0.0;
	avatar.roll		= 0.0;
	
	avatar.yawDelta = 0.0;
	
	for (int b=0; b<NUM_AVATAR_BONES; b++)
	{
		avatar.bone[b].position			= glm::vec3( 0.0, 0.0, 0.0 );
		avatar.bone[b].springyPosition	= glm::vec3( 0.0, 0.0, 0.0 );
		avatar.bone[b].springyVelocity	= glm::vec3( 0.0, 0.0, 0.0 );
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
	avatar.bone[ AVATAR_BONE_LEFT_PELVIS		].parent = AVATAR_BONE_PELVIS_SPINE;
	avatar.bone[ AVATAR_BONE_LEFT_THIGH			].parent = AVATAR_BONE_LEFT_PELVIS;
	avatar.bone[ AVATAR_BONE_LEFT_SHIN			].parent = AVATAR_BONE_LEFT_THIGH;
	avatar.bone[ AVATAR_BONE_LEFT_FOOT			].parent = AVATAR_BONE_LEFT_SHIN;

	//----------------------------------------------------------------------------
	// right pelvis and leg
	//----------------------------------------------------------------------------
	avatar.bone[ AVATAR_BONE_RIGHT_PELVIS		].parent = AVATAR_BONE_PELVIS_SPINE;
	avatar.bone[ AVATAR_BONE_RIGHT_THIGH		].parent = AVATAR_BONE_RIGHT_PELVIS;
	avatar.bone[ AVATAR_BONE_RIGHT_SHIN			].parent = AVATAR_BONE_RIGHT_THIGH;
	avatar.bone[ AVATAR_BONE_RIGHT_FOOT			].parent = AVATAR_BONE_RIGHT_SHIN;


	//----------------------------------------------------------
	// specify the default pose position
	//----------------------------------------------------------
	avatar.bone[ AVATAR_BONE_PELVIS_SPINE		].defaultPosePosition = glm::vec3(  0.0,   0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_MID_SPINE			].defaultPosePosition = glm::vec3(  0.0,   0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_CHEST_SPINE		].defaultPosePosition = glm::vec3(  0.0,   0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_NECK				].defaultPosePosition = glm::vec3(  0.0,   0.06, 0.0  );
	avatar.bone[ AVATAR_BONE_HEAD				].defaultPosePosition = glm::vec3(  0.0,   0.06, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_CHEST			].defaultPosePosition = glm::vec3( -0.06,  0.06, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_SHOULDER		].defaultPosePosition = glm::vec3( -0.03,  0.0,  0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_UPPER_ARM		].defaultPosePosition = glm::vec3(  0.0,  -0.12, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_FOREARM		].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_HAND			].defaultPosePosition = glm::vec3(  0.0,  -0.05, 0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_CHEST		].defaultPosePosition = glm::vec3(  0.06,  0.06, 0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER		].defaultPosePosition = glm::vec3(  0.03,  0.0,  0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_UPPER_ARM	].defaultPosePosition = glm::vec3(  0.0,  -0.12, 0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_FOREARM		].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_HAND			].defaultPosePosition = glm::vec3(  0.0,  -0.05, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_PELVIS		].defaultPosePosition = glm::vec3( -0.05,  0.0,  0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_THIGH			].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_SHIN			].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	avatar.bone[ AVATAR_BONE_LEFT_FOOT			].defaultPosePosition = glm::vec3(  0.0,   0.0,  0.04 );
	avatar.bone[ AVATAR_BONE_RIGHT_PELVIS		].defaultPosePosition = glm::vec3(  0.05,  0.0,  0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_THIGH		].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_SHIN			].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	avatar.bone[ AVATAR_BONE_RIGHT_FOOT			].defaultPosePosition = glm::vec3(  0.0,   0.0,  0.04 );

	//----------------------------------------------------------------------------
	// calculate bone length
	//----------------------------------------------------------------------------
	calculateBoneLengths();

	//----------------------------------------------------------------------------
	// generate world positions
	//----------------------------------------------------------------------------
	updateAvatarSkeleton();
}




//-----------------------------------------
void Head::calculateBoneLengths()
{
	for (int b=0; b<NUM_AVATAR_BONES; b++)
	{
		avatar.bone[b].length = glm::length( avatar.bone[b].defaultPosePosition );
	}

	avatar.maxArmLength
	= avatar.bone[ AVATAR_BONE_RIGHT_UPPER_ARM	].length
	+ avatar.bone[ AVATAR_BONE_RIGHT_FOREARM	].length 
	+ avatar.bone[ AVATAR_BONE_RIGHT_HAND		].length;
}


//-----------------------------------------
void Head::updateAvatarSkeleton()
{
	//------------------------------------------------------------------------
	// rotate...
	//------------------------------------------------------------------------	
	avatar.orientation.setToIdentity();
	avatar.orientation.yaw( avatar.yaw );

	//------------------------------------------------------------------------
	// calculate positions of all bones by traversing the skeleton tree:
	//------------------------------------------------------------------------
	for (int b=0; b<NUM_AVATAR_BONES; b++)
	{	
		if ( avatar.bone[b].parent == AVATAR_BONE_NULL )
		{
			avatar.bone[b].orientation.set( avatar.orientation );
			avatar.bone[b].position = position;
		}
		else
		{
			avatar.bone[b].orientation.set( avatar.bone[ avatar.bone[b].parent ].orientation );
			avatar.bone[b].position = avatar.bone[ avatar.bone[b].parent ].position;
		}
											
														
/*																				
		float xx	= glm::dot( avatar.bone[b].defaultPosePosition.x, (float)avatar.bone[b].orientation.getRight	().x )
					+ glm::dot( avatar.bone[b].defaultPosePosition.y, (float)avatar.bone[b].orientation.getRight	().y )
					+ glm::dot( avatar.bone[b].defaultPosePosition.z, (float)avatar.bone[b].orientation.getRight	().z );

		float yy	= glm::dot( avatar.bone[b].defaultPosePosition.x, (float)avatar.bone[b].orientation.getUp		().x )
					+ glm::dot( avatar.bone[b].defaultPosePosition.y, (float)avatar.bone[b].orientation.getUp		().y )
					+ glm::dot( avatar.bone[b].defaultPosePosition.z, (float)avatar.bone[b].orientation.getUp		().z );

		float zz	= glm::dot( avatar.bone[b].defaultPosePosition.x, (float)avatar.bone[b].orientation.getFront	().x )
					+ glm::dot( avatar.bone[b].defaultPosePosition.y, (float)avatar.bone[b].orientation.getFront	().y )
					+ glm::dot( avatar.bone[b].defaultPosePosition.z, (float)avatar.bone[b].orientation.getFront	().z );
*/

		float xx	= glm::dot( avatar.bone[b].defaultPosePosition.x, (float)avatar.bone[b].orientation.getRight	().x )
					+ glm::dot( avatar.bone[b].defaultPosePosition.y, (float)avatar.bone[b].orientation.getRight	().y )
					+ glm::dot( avatar.bone[b].defaultPosePosition.z, (float)avatar.bone[b].orientation.getRight	().z );

		float yy	= glm::dot( avatar.bone[b].defaultPosePosition.x, (float)avatar.bone[b].orientation.getUp		().x )
					+ glm::dot( avatar.bone[b].defaultPosePosition.y, (float)avatar.bone[b].orientation.getUp		().y )
					+ glm::dot( avatar.bone[b].defaultPosePosition.z, (float)avatar.bone[b].orientation.getUp		().z );

		float zz	= glm::dot( avatar.bone[b].defaultPosePosition.x, (float)avatar.bone[b].orientation.getFront	().x )
					+ glm::dot( avatar.bone[b].defaultPosePosition.y, (float)avatar.bone[b].orientation.getFront	().y )
					+ glm::dot( avatar.bone[b].defaultPosePosition.z, (float)avatar.bone[b].orientation.getFront	().z );



		//float xx = avatar.bone[b].defaultPosePosition.x;
		//float yy = avatar.bone[b].defaultPosePosition.y;
		//float zz = avatar.bone[b].defaultPosePosition.z;

		glm::vec3 rotatedBoneVector( xx, yy, zz );
		avatar.bone[b].position += rotatedBoneVector;
	}	
	
	//------------------------------------------------------------------------
	// reset hand and elbow position according to hand movement
	//------------------------------------------------------------------------
	if ( handBeingMoved ) 
	{
		updateHandMovement();
		handBeingMoved = false;
	}
}




//-----------------------------------------------
void Head::updateAvatarSprings( float deltaTime )
{
	for (int b=0; b<NUM_AVATAR_BONES; b++)
	{
		glm::vec3 springVector( avatar.bone[b].springyPosition );

		if ( avatar.bone[b].parent == AVATAR_BONE_NULL )
		{
			springVector -= position;
		}
		else if ( avatar.bone[b].parent == AVATAR_BONE_PELVIS_SPINE )
		{
			springVector -= position;
		}
		else
		{
			springVector -= avatar.bone[ avatar.bone[b].parent ].springyPosition;
		}

		float length = glm::length( springVector );
		
		if ( length > 0.0f )
		{
			glm::vec3 springDirection = springVector / length;
			
			float force = ( length - avatar.bone[b].length ) * springForce * deltaTime;
			
			avatar.bone[ b						].springyVelocity -= springDirection * force;
			avatar.bone[ avatar.bone[b].parent	].springyVelocity += springDirection * force;
		}
		
		avatar.bone[b].springyVelocity += ( avatar.bone[b].position - avatar.bone[b].springyPosition ) * springToBodyTightness * deltaTime;
		avatar.bone[b].springyVelocity *= 0.8;
		
		avatar.bone[b].springyVelocity *= ( 1.0 - springVelocityDecay * deltaTime );
		
		avatar.bone[b].springyPosition += avatar.bone[b].springyVelocity;
	}
}



//----------------------
float Head::getBodyYaw()
{
	return avatar.yaw;
}


//--------------------------------------
glm::vec3 Head::getHeadLookatDirection()
{
	return glm::vec3
	(
		avatar.orientation.getFront().x,
		avatar.orientation.getFront().y,
		-avatar.orientation.getFront().z
	);
}

//-------------------------------------------
glm::vec3 Head::getHeadLookatDirectionUp()
{
	return glm::vec3
	(
		avatar.orientation.getUp().x,
		avatar.orientation.getUp().y,
		-avatar.orientation.getUp().z
	);
}

//-------------------------------------------
glm::vec3 Head::getHeadLookatDirectionRight()
{
	return glm::vec3
	(
		avatar.orientation.getRight().x,
		avatar.orientation.getRight().y,
		avatar.orientation.getRight().z
	);
}


//-------------------------------
glm::vec3 Head::getHeadPosition()
{
	return glm::vec3
	(
		avatar.bone[ AVATAR_BONE_HEAD ].position.x,
		avatar.bone[ AVATAR_BONE_HEAD ].position.y,
		avatar.bone[ AVATAR_BONE_HEAD ].position.z
	);
}



//-------------------------------
glm::vec3 Head::getBodyPosition()
{
	return glm::vec3
	(
		avatar.bone[ AVATAR_BONE_PELVIS_SPINE ].position.x,
		avatar.bone[ AVATAR_BONE_PELVIS_SPINE ].position.y,
		avatar.bone[ AVATAR_BONE_PELVIS_SPINE ].position.z
	);
}



//-----------------------------
void Head::updateHandMovement()
{
	glm::vec3 transformedHandMovement;

	transformedHandMovement.x
	= glm::dot( movedHandOffset.x, -(float)avatar.orientation.getRight().x )
	+ glm::dot( movedHandOffset.y, -(float)avatar.orientation.getRight().y )
	+ glm::dot( movedHandOffset.z, -(float)avatar.orientation.getRight().z );

	transformedHandMovement.y
	= glm::dot( movedHandOffset.x, -(float)avatar.orientation.getUp().x )
	+ glm::dot( movedHandOffset.y, -(float)avatar.orientation.getUp().y )
	+ glm::dot( movedHandOffset.z, -(float)avatar.orientation.getUp().z );

	transformedHandMovement.z
	= glm::dot( movedHandOffset.x, -(float)avatar.orientation.getFront().x )
	+ glm::dot( movedHandOffset.y, -(float)avatar.orientation.getFront().y )
	+ glm::dot( movedHandOffset.z, -(float)avatar.orientation.getFront().z );

	avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position += transformedHandMovement;	
	glm::vec3 armVector = avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position;
	armVector -= avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;

	//-------------------------------------------------------------------------------
	// test to see if right hand is being dragged beyond maximum arm length
	//-------------------------------------------------------------------------------
	float distance = glm::length( armVector );
	
	//-------------------------------------------------------------------------------
	// if right hand is being dragged beyond maximum arm length...
	//-------------------------------------------------------------------------------
	if ( distance > avatar.maxArmLength )
	{
		//-------------------------------------------------------------------------------
		// reset right hand to be constrained to maximum arm length
		//-------------------------------------------------------------------------------
		avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position = avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
		glm::vec3 armNormal = armVector / distance;
		armVector = armNormal * (float)avatar.maxArmLength;
		distance = avatar.maxArmLength;
		glm::vec3 constrainedPosition = avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
		constrainedPosition += armVector;
		avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position = constrainedPosition;
	}
	
	//-----------------------------------------------------------------------------
	// set elbow position 
	//-----------------------------------------------------------------------------
	glm::vec3 newElbowPosition = avatar.bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
	newElbowPosition += armVector * (float)ONE_HALF;
	glm::vec3 perpendicular = glm::vec3( -armVector.y, armVector.x, armVector.z );
	newElbowPosition += perpendicular * (float)( ( 1.0 - ( avatar.maxArmLength / distance ) ) * ONE_HALF );
	avatar.bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].position = newElbowPosition;

	//-----------------------------------------------------------------------------
	// set wrist position 
	//-----------------------------------------------------------------------------
	glm::vec3 vv( avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position );
	vv -= avatar.bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].position;
	glm::vec3 newWristPosition = avatar.bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].position;
	newWristPosition += vv * 0.7f;
	avatar.bone[ AVATAR_BONE_RIGHT_FOREARM ].position = newWristPosition;
}




//-----------------------------------------
void Head::renderBody()
{
	//-----------------------------------------
    //  Render bone positions as spheres
	//-----------------------------------------
	for (int b=0; b<NUM_AVATAR_BONES; b++)
	{
		glColor3fv( skinColor );
		glPushMatrix();
			glTranslatef( avatar.bone[b].position.x, avatar.bone[b].position.y, avatar.bone[b].position.z );
			glutSolidSphere( 0.02f, 10.0f, 5.0f );
		glPopMatrix();

		glColor3fv( lightBlue );
		glPushMatrix();
			glTranslatef( avatar.bone[b].springyPosition.x, avatar.bone[b].springyPosition.y, avatar.bone[b].springyPosition.z );
			glutSolidSphere( 0.01f, 10.0f, 5.0f );
		glPopMatrix();
	}

	//-----------------------------------------------------
    // Render lines connecting the bone positions
	//-----------------------------------------------------
    glColor3f(1,1,1);
    glLineWidth(3.0);

	for (int b=1; b<NUM_AVATAR_BONES; b++)
	{
		glBegin( GL_LINE_STRIP );
		glVertex3fv( &avatar.bone[ avatar.bone[ b ].parent ].position.x );
		glVertex3fv( &avatar.bone[ b ].position.x);
		glEnd();
	}

	//-----------------------------------------------------
    // Render lines connecting the springy positions
	//-----------------------------------------------------
    glColor3f( 0.2f, 0.3f, 0.4f );
    glLineWidth(3.0);

	for (int b=1; b<NUM_AVATAR_BONES; b++)
	{
		glBegin( GL_LINE_STRIP );
		glVertex3fv( &avatar.bone[ avatar.bone[ b ].parent ].springyPosition.x );
		glVertex3fv( &avatar.bone[ b ].springyPosition.x );
		glEnd();
	}
	
	/*
    glBegin(GL_LINE_STRIP);
    glVertex3fv(&avatar.bone[AVATAR_BONE_CHEST_SPINE].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_NECK].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_CHEST_SPINE].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_LEFT_SHOULDER].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_LEFT_UPPER_ARM].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_LEFT_FOREARM].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_LEFT_HAND].position.x);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3fv(&avatar.bone[AVATAR_BONE_CHEST_SPINE].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_RIGHT_SHOULDER].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_RIGHT_UPPER_ARM].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_RIGHT_FOREARM].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_RIGHT_HAND].position.x);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3fv(&avatar.bone[AVATAR_BONE_CHEST_SPINE].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_MID_SPINE].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_PELVIS_SPINE].position.x);    
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3fv(&avatar.bone[AVATAR_BONE_PELVIS_SPINE].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_LEFT_PELVIS].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_LEFT_THIGH].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_LEFT_SHIN].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_LEFT_FOOT].position.x);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3fv(&avatar.bone[AVATAR_BONE_PELVIS_SPINE].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_RIGHT_PELVIS].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_RIGHT_THIGH].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_RIGHT_SHIN].position.x);
    glVertex3fv(&avatar.bone[AVATAR_BONE_RIGHT_FOOT].position.x);
    glEnd();
	*/
}



//  Transmit data to agents requesting it

//called on me just prior to sending data to others (continuasly called)

//---------------------------------------------------
int Head::getBroadcastData(char* data)
{
    // Copy data for transmission to the buffer, return length of data
    sprintf(data, "H%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
            getRenderPitch() + Pitch, -getRenderYaw() + 180 -Yaw, Roll,
            position.x + leanSideways, position.y, position.z + leanForward,
            loudness, averageLoudness,
            //hand->getPos().x, hand->getPos().y, hand->getPos().z);  //previous to Ventrella change
            avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.x, 
			avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.y, 
			avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.z );    // Ventrella change
    return strlen(data);
}

//called on the other agents - assigns it to my views of the others

//---------------------------------------------------
void Head::parseData(void *data, int size) 
{
	sscanf
	(
		(char *)data, "H%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
		&Pitch, &Yaw, &Roll,
		&position.x, &position.y, &position.z,
		&loudness, &averageLoudness,
		&avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.x, 
		&avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.y, 
		&avatar.bone[ AVATAR_BONE_RIGHT_HAND ].position.z
	);
	
	handBeingMoved = true;
}




//---------------------------------------------------
void Head::SetNewHeadTarget(float pitch, float yaw)
{
    PitchTarget = pitch;
    YawTarget = yaw;
}
