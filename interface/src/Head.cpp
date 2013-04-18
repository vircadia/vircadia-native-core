//
//  Head.cpp
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//	adapted by Jeffrey Ventrella 
//  Copyright (c) 2012 Physical, Inc.. All rights reserved.
//

#include <glm/glm.hpp>
#include <vector>
#include <lodepng.h>
#include <SharedUtil.h>
#include "Head.h"
#include "Log.h"
#include <AgentList.h>
#include <AgentTypes.h>
#include <PacketHeaders.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/quaternion.hpp>
//#include <glm/gtx/quaternion.hpp> //looks like we might not need this

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

bool usingBigSphereCollisionTest = true;

const float DECAY = 0.1;
const float THRUST_MAG	= 10.0;
const float YAW_MAG		= 300.0;

char iris_texture_file[] = "resources/images/green_eye.png";

vector<unsigned char> iris_texture;
unsigned int iris_texture_width = 512;
unsigned int iris_texture_height = 256;

Head::Head(bool isMine) {
    
	_avatar.orientation.setToIdentity();
	_avatar.velocity    = glm::vec3( 0.0, 0.0, 0.0 );
	_avatar.thrust		= glm::vec3( 0.0, 0.0, 0.0 );
    _rotation           = glm::quat( 0.0f, 0.0f, 0.0f, 0.0f );	
	_closestOtherAvatar = 0;
	_bodyYaw            = -90.0;
	_bodyPitch          = 0.0;
	_bodyRoll           = 0.0;
	_bodyYawDelta       = 0.0;
	_triggeringAction   = false;
	_mode               = AVATAR_MODE_STANDING;
    _isMine             = isMine;

    initializeSkeleton();
    
    _TEST_bigSphereRadius = 0.3f;
    _TEST_bigSpherePosition = glm::vec3( 0.0f, _TEST_bigSphereRadius, 2.0f );
    
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) driveKeys[i] = false; 
    
    PupilSize = 0.10;
    interPupilDistance = 0.6;
    interBrowDistance = 0.75;
    NominalPupilSize = 0.10;
    _headYaw = 0.0;
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
	
	_handBeingMoved         = false;
	_previousHandBeingMoved = false;
	_movedHandOffset        = glm::vec3( 0.0, 0.0, 0.0 );
	_usingSprings           = false;
	_springForce            = 6.0f;
	_springVelocityDecay    = 16.0f;
	
	sphere = NULL;
	
    if (iris_texture.size() == 0) {
        switchToResourcesParentIfRequired();
        unsigned error = lodepng::decode(iris_texture, iris_texture_width, iris_texture_height, iris_texture_file);
        if (error != 0) {
            printLog("error %u: %s\n", error, lodepng_error_text(error));
        }
    }
	
	for (int o=0; o<NUM_OTHER_AVATARS; o++) {
		DEBUG_otherAvatarListTimer[o] = 0.0f;
		DEBUG_otherAvatarListPosition[o] = glm::vec3( 0.0f, 0.0f, 0.0f );
	}
	
	//--------------------------------------------------
	// test... just slam them into random positions...
	//--------------------------------------------------
	DEBUG_otherAvatarListPosition[ 0 ] = glm::vec3(  0.0, 0.3,  2.0 );
	DEBUG_otherAvatarListPosition[ 1 ] = glm::vec3(  4.0, 0.3,  2.0 );
	DEBUG_otherAvatarListPosition[ 2 ] = glm::vec3(  2.0, 0.3,  2.0 );
	DEBUG_otherAvatarListPosition[ 3 ] = glm::vec3(  1.0, 0.3, -4.0 );
	DEBUG_otherAvatarListPosition[ 4 ] = glm::vec3( -2.0, 0.3, -2.0 );
}

Head::Head(const Head &otherHead) {
    
	_avatar.orientation.set( otherHead._avatar.orientation );
	_avatar.velocity	= otherHead._avatar.velocity;
	_avatar.thrust		= otherHead._avatar.thrust;
    _rotation           = otherHead._rotation;
	_closestOtherAvatar = otherHead._closestOtherAvatar;
	_bodyYaw            = otherHead._bodyYaw;
	_bodyPitch          = otherHead._bodyPitch;
	_bodyRoll           = otherHead._bodyRoll;
	_bodyYawDelta       = otherHead._bodyYawDelta;
	_triggeringAction   = otherHead._triggeringAction;
	_mode               = otherHead._mode;

    initializeSkeleton();
    
    for (int i = 0; i < MAX_DRIVE_KEYS; i++) driveKeys[i] = otherHead.driveKeys[i];

    PupilSize = otherHead.PupilSize;
    interPupilDistance = otherHead.interPupilDistance;
    interBrowDistance = otherHead.interBrowDistance;
    NominalPupilSize = otherHead.NominalPupilSize;
    _headYaw = otherHead._headYaw;
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
    
}

Head::~Head()  {
    if (sphere != NULL) {
        gluDeleteQuadric(sphere);
    }
}

Head* Head::clone() const {
    return new Head(*this);
}

void Head::reset() {
    _headPitch = _headYaw = _headRoll = 0;
    leanForward = leanSideways = 0;
}


//this pertains to moving the head with the glasses 
//---------------------------------------------------
void Head::UpdateGyros(float frametime, SerialInterface * serialInterface, int head_mirror, glm::vec3 * gravity)
//  Using serial data, update avatar/render position and angles
{
    const float PITCH_ACCEL_COUPLING = 0.5;
    const float ROLL_ACCEL_COUPLING = -1.0;
    float measured_pitch_rate = serialInterface->getRelativeValue(HEAD_PITCH_RATE);
    _headYawRate = serialInterface->getRelativeValue(HEAD_YAW_RATE);
    float measured_lateral_accel = serialInterface->getRelativeValue(ACCEL_X) -
                                ROLL_ACCEL_COUPLING*serialInterface->getRelativeValue(HEAD_ROLL_RATE);
    float measured_fwd_accel = serialInterface->getRelativeValue(ACCEL_Z) -
                                PITCH_ACCEL_COUPLING*serialInterface->getRelativeValue(HEAD_PITCH_RATE);
    float measured_roll_rate = serialInterface->getRelativeValue(HEAD_ROLL_RATE);
   
    //printLog("Pitch Rate: %d ACCEL_Z: %d\n", serialInterface->getRelativeValue(PITCH_RATE), 
    //                                         serialInterface->getRelativeValue(ACCEL_Z));
    //printLog("Pitch Rate: %d ACCEL_X: %d\n", serialInterface->getRelativeValue(PITCH_RATE), 
    //                                         serialInterface->getRelativeValue(ACCEL_Z));
    //printLog("Pitch: %f\n", Pitch);
    
    //  Update avatar head position based on measured gyro rates
    const float HEAD_ROTATION_SCALE = 0.70;
    const float HEAD_ROLL_SCALE = 0.40;
    const float HEAD_LEAN_SCALE = 0.01;
    const float MAX_PITCH = 45;
    const float MIN_PITCH = -45;
    const float MAX_YAW = 85;
    const float MIN_YAW = -85;

    if ((_headPitch < MAX_PITCH) && (_headPitch > MIN_PITCH))
        addPitch(measured_pitch_rate * -HEAD_ROTATION_SCALE * frametime);
    
    addRoll(-measured_roll_rate * HEAD_ROLL_SCALE * frametime);

    if (head_mirror) {
        if ((_headYaw < MAX_YAW) && (_headYaw > MIN_YAW))
            addYaw(-_headYawRate * HEAD_ROTATION_SCALE * frametime);
        addLean(-measured_lateral_accel * frametime * HEAD_LEAN_SCALE, -measured_fwd_accel*frametime * HEAD_LEAN_SCALE);
    } else {
        if ((_headYaw < MAX_YAW) && (_headYaw > MIN_YAW))
            addYaw(_headYawRate * -HEAD_ROTATION_SCALE * frametime);
        addLean(measured_lateral_accel * frametime * -HEAD_LEAN_SCALE, measured_fwd_accel*frametime * HEAD_LEAN_SCALE);        
    } 
}

void Head::addLean(float x, float z) {
    //  Add Body lean as impulse 
    leanSideways += x;
    leanForward  += z;
}


void Head::setLeanForward(float dist){
    leanForward = dist;
}

void Head::setLeanSideways(float dist){
    leanSideways = dist;
}

void Head::setTriggeringAction( bool d ) {
	_triggeringAction = d;
}



void Head::simulate(float deltaTime) {
    
    //-------------------------------------------------------------
    // if the avatar being simulated is mine, then loop through
    // all the other avatars to get information about them...
    //-------------------------------------------------------------
    if ( _isMine )
    {
        //-------------------------------------
        // DEBUG - other avatars...
        //-------------------------------------
        _closestOtherAvatar = -1;
        float closestDistance = 10000.0f;
        
        /*
        AgentList * agentList = AgentList::getInstance();

        for(std::vector<Agent>::iterator agent = agentList->getAgents().begin();
            agent != agentList->getAgents().end();
            agent++) {
            if (( agent->getLinkedData() != NULL && ( agent->getType() == AGENT_TYPE_INTERFACE ) )) {
                Head *otherAvatar = (Head *)agent->getLinkedData();
               
                // when this is working, I will grab the position here...
                //glm::vec3 otherAvatarPosition = otherAvatar->getBodyPosition();
            }
        }
        */
        
        ///for testing only (prior to having real avs working)
        for (int o=0; o<NUM_OTHER_AVATARS; o++) {
            //-------------------------------------
            // test other avs for proximity...
            //-------------------------------------
            glm::vec3 v( _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position );
            v -= DEBUG_otherAvatarListPosition[o];
            
            float distance = glm::length( v );

            if ( distance < _avatar.maxArmLength ) {
                if ( distance < closestDistance ) {
                    closestDistance = distance;
                    _closestOtherAvatar = o;
                }
            }
        }
        
        if ( usingBigSphereCollisionTest ) {
            //--------------------------------------------------------------
            // test for avatar collision response (using a big sphere :)
            //--------------------------------------------------------------
            updateBigSphereCollisionTest(deltaTime);
        }
            
    }//if ( _isMine )

	//------------------------
	// update avatar skeleton
	//------------------------ 
	updateSkeleton();
	
	//------------------------------------------------------------------------
	// reset hand and elbow position according to hand movement
	//------------------------------------------------------------------------
	if ( _handBeingMoved ){
		if (! _previousHandBeingMoved ){ 
			initializeBodySprings();
			_usingSprings = true;
			//printLog( "just started moving hand\n" );
		}
	}
	else {
		if ( _previousHandBeingMoved ){ 
			_usingSprings = false;
			//printLog( "just stopped moving hand\n" );
		}
	}
    	
	if ( _handBeingMoved ) {
		updateHandMovement();
		updateBodySprings( deltaTime );
	}

	_previousHandBeingMoved = _handBeingMoved;
	_handBeingMoved = false;
	
    if ( _isMine ) { // driving the avatar around should only apply is this is my avatar (as opposed to an avatar being driven remotely) 
        //-------------------------------------------------
        // this handles the avatar being driven around...
        //-------------------------------------------------	
        _avatar.thrust = glm::vec3( 0.0, 0.0, 0.0 );
            
        if (driveKeys[FWD]) {
            glm::vec3 front( _avatar.orientation.getFront().x, _avatar.orientation.getFront().y, _avatar.orientation.getFront().z );
            _avatar.thrust += front * THRUST_MAG;
        }
        if (driveKeys[BACK]) {
            glm::vec3 front( _avatar.orientation.getFront().x, _avatar.orientation.getFront().y, _avatar.orientation.getFront().z );
            _avatar.thrust -= front * THRUST_MAG;
        }
        if (driveKeys[RIGHT]) {
            glm::vec3 right( _avatar.orientation.getRight().x, _avatar.orientation.getRight().y, _avatar.orientation.getRight().z );
            _avatar.thrust -= right * THRUST_MAG;
        }
        if (driveKeys[LEFT]) {
            glm::vec3 right( _avatar.orientation.getRight().x, _avatar.orientation.getRight().y, _avatar.orientation.getRight().z );
            _avatar.thrust += right * THRUST_MAG;
        }
        if (driveKeys[UP]) {
            glm::vec3 up( _avatar.orientation.getUp().x, _avatar.orientation.getUp().y, _avatar.orientation.getUp().z );
            _avatar.thrust += up * THRUST_MAG;
        }
        if (driveKeys[DOWN]) {
            glm::vec3 up( _avatar.orientation.getUp().x, _avatar.orientation.getUp().y, _avatar.orientation.getUp().z );
            _avatar.thrust -= up * THRUST_MAG;
        }
        if (driveKeys[ROT_RIGHT]) {	
            _bodyYawDelta -= YAW_MAG * deltaTime;
        }
        if (driveKeys[ROT_LEFT]) {	
            _bodyYawDelta += YAW_MAG * deltaTime;
        }
	}
    
    
	//----------------------------------------------------------
	float translationalSpeed = glm::length( _avatar.velocity );
	float rotationalSpeed = fabs( _bodyYawDelta );
	if ( translationalSpeed + rotationalSpeed > 0.2 )
	{
		_mode = AVATAR_MODE_WALKING;
	}
	else
	{
		_mode = AVATAR_MODE_COMMUNICATING;
	}
		
	//----------------------------------------------------------
	// update body yaw by body yaw delta
	//----------------------------------------------------------
    if (_isMine) {
    _bodyYaw += _bodyYawDelta * deltaTime;
    }
        
	// we will be eventually getting head rotation from elsewhere. For now, just setting it to body rotation 
	_headYaw   = _bodyYaw;
	_headPitch = _bodyPitch;
	_headRoll  = _bodyRoll;
	
	//----------------------------------------------------------
	// decay body yaw delta
	//----------------------------------------------------------
    const float TEST_YAW_DECAY = 5.0;
    _bodyYawDelta *= (1.0 - TEST_YAW_DECAY * deltaTime);

	//----------------------------------------------------------
	// add thrust to velocity
	//----------------------------------------------------------
	_avatar.velocity += glm::dvec3(_avatar.thrust * deltaTime);

    //----------------------------------------------------------
    // update position by velocity
    //----------------------------------------------------------
    _bodyPosition += (glm::vec3)_avatar.velocity * deltaTime;

	//----------------------------------------------------------
	// decay velocity
	//----------------------------------------------------------
    const float LIN_VEL_DECAY = 5.0;
    _avatar.velocity *= ( 1.0 - LIN_VEL_DECAY * deltaTime );
	
    if (!noise) {
        //  Decay back toward center 
        _headPitch *= (1.0f - DECAY*2*deltaTime);
        _headYaw   *= (1.0f - DECAY*2*deltaTime);
        _headRoll  *= (1.0f - DECAY*2*deltaTime);
    }
    else {
        //  Move toward new target  
        _headPitch += (PitchTarget - _headPitch)*10*deltaTime;   // (1.f - DECAY*deltaTime)*Pitch + ;
        _headYaw += (YawTarget - _headYaw)*10*deltaTime; //  (1.f - DECAY*deltaTime);
        _headRoll *= (1.f - DECAY*deltaTime);
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
        
        EyeballPitch[0] = EyeballPitch[1] = -_headPitch + eye_target_pitch_adjust;
        EyeballYaw[0] = EyeballYaw[1] = -_headYaw + eye_target_yaw_adjust;
    }
	

    if (noise)
    {
        _headPitch += (randFloat() - 0.5)*0.2*NoiseEnvelope;
        _headYaw += (randFloat() - 0.5)*0.3*NoiseEnvelope;
        //PupilSize += (randFloat() - 0.5)*0.001*NoiseEnvelope;
        
        if (randFloat() < 0.005) MouthWidth = MouthWidthChoices[rand()%3];
        
        if (!eyeContact) {
            if (randFloat() < 0.01)  EyeballPitch[0] = EyeballPitch[1] = (randFloat() - 0.5)*20;
            if (randFloat() < 0.01)  EyeballYaw[0] = EyeballYaw[1] = (randFloat()- 0.5)*10;
        }         

        if ((randFloat() < 0.005) && (fabs(PitchTarget - _headPitch) < 1.0) && (fabs(YawTarget - _headYaw) < 1.0)) {
            SetNewHeadTarget((randFloat()-0.5)*20.0, (randFloat()-0.5)*45.0);
        }

        if (0) {

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
}
      
	  
	  
	  
//--------------------------------------------------------------------------------
// This is a workspace for testing avatar body collision detection and response
//--------------------------------------------------------------------------------
void Head::updateBigSphereCollisionTest( float deltaTime ) {

    float myBodyApproximateBoundingRadius = 1.0f;
    glm::vec3 vectorFromMyBodyToBigSphere(_bodyPosition - _TEST_bigSpherePosition);
    bool jointCollision = false;

    float distanceToBigSphere = glm::length(vectorFromMyBodyToBigSphere);
    if ( distanceToBigSphere < myBodyApproximateBoundingRadius + _TEST_bigSphereRadius)
    {
        for (int b=0; b<NUM_AVATAR_BONES; b++) 
        {
            glm::vec3 vectorFromJointToBigSphere(_bone[b].position - _TEST_bigSpherePosition);
            float distanceToBigSphereCenter = glm::length(vectorFromJointToBigSphere);
            float combinedRadius = _bone[b].radius + _TEST_bigSphereRadius;
            if ( distanceToBigSphereCenter < combinedRadius ) 
            {
                jointCollision = true;
                if (distanceToBigSphereCenter > 0.0) 
                {
                    float amp = 1.0 - (distanceToBigSphereCenter / combinedRadius); 
                    glm::vec3 collisionForce = vectorFromJointToBigSphere * amp;                        
                    _bone[b].springyVelocity += collisionForce * 8.0f * deltaTime;
                    _avatar.velocity += collisionForce * 18.0f * deltaTime;
                }
            }
        }
        
        if ( jointCollision ) {
            //----------------------------------------------------------
            // add gravity to velocity
            //----------------------------------------------------------
            _avatar.velocity += glm::dvec3( 0.0, -1.0, 0.0 ) * 0.05;
            
            //----------------------------------------------------------
            // ground collisions
            //----------------------------------------------------------
            if ( _bodyPosition.y < 0.0 ) {
                _bodyPosition.y = 0.0;
                if ( _avatar.velocity.y < 0.0 ) {
                    _avatar.velocity.y *= -0.7;
                }
            }       
        }     
    }
}
      
      
      
	   
void Head::render(int faceToFace) {

	//---------------------------------------------------
	// show avatar position
	//---------------------------------------------------
    glColor4f( 0.5f, 0.5f, 0.5f, 0.6 );
	glPushMatrix();
		glTranslatef(_bodyPosition.x, _bodyPosition.y, _bodyPosition.z);
		glScalef( 0.03, 0.03, 0.03 );
        glutSolidSphere( 1, 10, 10 );
	glPopMatrix();
    
    
    if ( usingBigSphereCollisionTest ) {
        //---------------------------------------------------
        // show TEST big sphere 
        //---------------------------------------------------
        glColor4f( 0.5f, 0.6f, 0.8f, 0.7 );
        glPushMatrix();
            glTranslatef(_TEST_bigSpherePosition.x, _TEST_bigSpherePosition.y, _TEST_bigSpherePosition.z);
            glScalef( _TEST_bigSphereRadius, _TEST_bigSphereRadius, _TEST_bigSphereRadius );
            glutSolidSphere( 1, 20, 20 );
        glPopMatrix();
     }
     
	//---------------------------------------------------
	// show avatar orientation
	//---------------------------------------------------
	renderOrientationDirections( _bone[ AVATAR_BONE_HEAD ].position, _bone[ AVATAR_BONE_HEAD ].orientation, 0.2f );
	
	//---------------------------------------------------
	// render body
	//---------------------------------------------------
	renderBody();

	//---------------------------------------------------
	// render head
	//---------------------------------------------------
	renderHead(faceToFace);
	
	//---------------------------------------------------------------------------
	// if this is my avatar, then render my interactions with the other avatars
	//---------------------------------------------------------------------------
    if ( _isMine )
    {
        //---------------------------------------------------
        // render other avatars (DEBUG TEST)
        //---------------------------------------------------
        for (int o=0; o<NUM_OTHER_AVATARS; o++) {
            glPushMatrix();
                glTranslatef( DEBUG_otherAvatarListPosition[o].x, DEBUG_otherAvatarListPosition[o].y, DEBUG_otherAvatarListPosition[o].z );
                glScalef( 0.03, 0.03, 0.03 );
                glutSolidSphere( 1, 10, 10 );
            glPopMatrix();
        }

        if ( _usingSprings ) {
            if ( _closestOtherAvatar != -1 ) {					

                glm::vec3 v1( _bone[ AVATAR_BONE_RIGHT_HAND ].position );
                glm::vec3 v2( DEBUG_otherAvatarListPosition[ _closestOtherAvatar ] );
                
                glLineWidth( 5.0 );
                glColor4f( 0.9f, 0.5f, 0.2f, 0.6 );
                glBegin( GL_LINE_STRIP );
                glVertex3f( v1.x, v1.y, v1.z );
                glVertex3f( v2.x, v2.y, v2.z );
                glEnd();
            }
        }
    }
}



//this has been moved to Utils.cpp
/*
void Head::renderOrientationDirections( glm::vec3 position, Orientation orientation, float size ) {
	glm::vec3 pRight	= position + orientation.right	* size;
	glm::vec3 pUp		= position + orientation.up		* size;
	glm::vec3 pFront	= position + orientation.front	* size;
		
	glColor3f( 1.0f, 0.0f, 0.0f );
	glBegin( GL_LINE_STRIP );
	glVertex3f( position.x, position.y, position.z );
	glVertex3f( pRight.x, pRight.y, pRight.z );
	glEnd();

	glColor3f( 0.0f, 1.0f, 0.0f );
	glBegin( GL_LINE_STRIP );
	glVertex3f( position.x, position.y, position.z );
	glVertex3f( pUp.x, pUp.y, pUp.z );
	glEnd();

	glColor3f( 0.0f, 0.0f, 1.0f );
	glBegin( GL_LINE_STRIP );
	glVertex3f( position.x, position.y, position.z );
	glVertex3f( pFront.x, pFront.y, pFront.z );
	glEnd();
}
*/

	  
	   
void Head::renderHead(int faceToFace) {
    int side = 0;
        
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
    glPushMatrix();
    
	if (_usingSprings) {
		glTranslatef
		( 
			_bone[ AVATAR_BONE_HEAD ].springyPosition.x, 
			_bone[ AVATAR_BONE_HEAD ].springyPosition.y, 
			_bone[ AVATAR_BONE_HEAD ].springyPosition.z 
		);
	}
	else {
		glTranslatef
		( 
			_bone[ AVATAR_BONE_HEAD ].position.x, 
			_bone[ AVATAR_BONE_HEAD ].position.y, 
			_bone[ AVATAR_BONE_HEAD ].position.z 
		);
	}
	
	
	glScalef( 0.03, 0.03, 0.03 );

    glRotatef(_headYaw,   0, 1, 0); 
    glRotatef(_headPitch, 1, 0, 0);
    glRotatef(_headRoll,  0, 0, 1);
    
    // Overall scale of head
    if (faceToFace) glScalef(2.0, 2.0, 2.0);
    else glScalef(0.75, 1.0, 1.0);
    

    //  Head
    if (_isMine) {
        glColor3fv(skinColor);
    }
    else {
        glColor3f(0,0,1); //  Temp:  Other people are BLUE
    }
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

    // Eyebrows
    audioAttack = 0.9*audioAttack + 0.1*fabs(loudness - lastLoudness);
    lastLoudness = loudness;

    const float BROW_LIFT_THRESHOLD = 100;
    if (audioAttack > BROW_LIFT_THRESHOLD)
        browAudioLift += sqrt(audioAttack)/1000.0;
    
    browAudioLift *= .90;
    
    glPushMatrix();
        glTranslatef(-interBrowDistance/2.0,0.4,0.45);
        for(side = 0; side < 2; side++) {
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


    glPopMatrix();
 }
 
 
 

void Head::setHandMovement( glm::vec3 movement ) {
	_handBeingMoved = true;
	_movedHandOffset = movement;
}

AvatarMode Head::getMode() {
	return _mode;
}


void Head::initializeSkeleton() {

	for (int b=0; b<NUM_AVATAR_BONES; b++) {
        _bone[b].parent              = AVATAR_BONE_NULL;
        _bone[b].position			 = glm::vec3( 0.0, 0.0, 0.0 );
        _bone[b].defaultPosePosition = glm::vec3( 0.0, 0.0, 0.0 );
        _bone[b].springyPosition     = glm::vec3( 0.0, 0.0, 0.0 );
        _bone[b].springyVelocity     = glm::vec3( 0.0, 0.0, 0.0 );
        _bone[b].rotation            = glm::quat( 0.0f, 0.0f, 0.0f, 0.0f );
        _bone[b].yaw                 = 0.0;
        _bone[b].pitch               = 0.0;
        _bone[b].roll                = 0.0;
        _bone[b].length              = 0.0;
        _bone[b].radius              = 0.02; //default
        _bone[b].springBodyTightness = 4.0;
        _bone[b].orientation.setToIdentity();
	}

	//----------------------------------------------------------------------------
	// parental hierarchy
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	// spine and head
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_PELVIS_SPINE		].parent = AVATAR_BONE_NULL;
	_bone[ AVATAR_BONE_MID_SPINE        ].parent = AVATAR_BONE_PELVIS_SPINE;
	_bone[ AVATAR_BONE_CHEST_SPINE		].parent = AVATAR_BONE_MID_SPINE;
	_bone[ AVATAR_BONE_NECK				].parent = AVATAR_BONE_CHEST_SPINE;
	_bone[ AVATAR_BONE_HEAD				].parent = AVATAR_BONE_NECK;
	
	//----------------------------------------------------------------------------
	// left chest and arm
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_LEFT_CHEST		].parent = AVATAR_BONE_MID_SPINE;
	_bone[ AVATAR_BONE_LEFT_SHOULDER    ].parent = AVATAR_BONE_LEFT_CHEST;
	_bone[ AVATAR_BONE_LEFT_UPPER_ARM	].parent = AVATAR_BONE_LEFT_SHOULDER;
	_bone[ AVATAR_BONE_LEFT_FOREARM		].parent = AVATAR_BONE_LEFT_UPPER_ARM;
	_bone[ AVATAR_BONE_LEFT_HAND		].parent = AVATAR_BONE_LEFT_FOREARM;

	//----------------------------------------------------------------------------
	// right chest and arm
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_RIGHT_CHEST		].parent = AVATAR_BONE_MID_SPINE;
	_bone[ AVATAR_BONE_RIGHT_SHOULDER	].parent = AVATAR_BONE_RIGHT_CHEST;
	_bone[ AVATAR_BONE_RIGHT_UPPER_ARM	].parent = AVATAR_BONE_RIGHT_SHOULDER;
	_bone[ AVATAR_BONE_RIGHT_FOREARM	].parent = AVATAR_BONE_RIGHT_UPPER_ARM;
	_bone[ AVATAR_BONE_RIGHT_HAND		].parent = AVATAR_BONE_RIGHT_FOREARM;
	
	//----------------------------------------------------------------------------
	// left pelvis and leg
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_LEFT_PELVIS		].parent = AVATAR_BONE_PELVIS_SPINE;
	_bone[ AVATAR_BONE_LEFT_THIGH		].parent = AVATAR_BONE_LEFT_PELVIS;
	_bone[ AVATAR_BONE_LEFT_SHIN		].parent = AVATAR_BONE_LEFT_THIGH;
	_bone[ AVATAR_BONE_LEFT_FOOT		].parent = AVATAR_BONE_LEFT_SHIN;

	//----------------------------------------------------------------------------
	// right pelvis and leg
	//----------------------------------------------------------------------------
	_bone[ AVATAR_BONE_RIGHT_PELVIS		].parent = AVATAR_BONE_PELVIS_SPINE;
	_bone[ AVATAR_BONE_RIGHT_THIGH		].parent = AVATAR_BONE_RIGHT_PELVIS;
	_bone[ AVATAR_BONE_RIGHT_SHIN		].parent = AVATAR_BONE_RIGHT_THIGH;
	_bone[ AVATAR_BONE_RIGHT_FOOT		].parent = AVATAR_BONE_RIGHT_SHIN;


	//----------------------------------------------------------
	// specify the default pose position
	//----------------------------------------------------------
	_bone[ AVATAR_BONE_PELVIS_SPINE		].defaultPosePosition = glm::vec3(  0.0,   0.3,  0.0  );
	_bone[ AVATAR_BONE_MID_SPINE		].defaultPosePosition = glm::vec3(  0.0,   0.1,  0.0  );
	_bone[ AVATAR_BONE_CHEST_SPINE		].defaultPosePosition = glm::vec3(  0.0,   0.1,  0.0  );
	_bone[ AVATAR_BONE_NECK				].defaultPosePosition = glm::vec3(  0.0,   0.06, 0.0  );
	_bone[ AVATAR_BONE_HEAD				].defaultPosePosition = glm::vec3(  0.0,   0.06, 0.0  );
	_bone[ AVATAR_BONE_LEFT_CHEST		].defaultPosePosition = glm::vec3( -0.06,  0.06, 0.0  );
	_bone[ AVATAR_BONE_LEFT_SHOULDER	].defaultPosePosition = glm::vec3( -0.03,  0.0,  0.0  );
	_bone[ AVATAR_BONE_LEFT_UPPER_ARM	].defaultPosePosition = glm::vec3(  0.0,  -0.12, 0.0  );
	_bone[ AVATAR_BONE_LEFT_FOREARM		].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
	_bone[ AVATAR_BONE_LEFT_HAND		].defaultPosePosition = glm::vec3(  0.0,  -0.05, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_CHEST		].defaultPosePosition = glm::vec3(  0.06,  0.06, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_SHOULDER	].defaultPosePosition = glm::vec3(  0.03,  0.0,  0.0  );
	_bone[ AVATAR_BONE_RIGHT_UPPER_ARM	].defaultPosePosition = glm::vec3(  0.0,  -0.12, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_FOREARM	].defaultPosePosition = glm::vec3(  0.0,  -0.1,  0.0  );
	_bone[ AVATAR_BONE_RIGHT_HAND		].defaultPosePosition = glm::vec3(  0.0,  -0.05, 0.0  );
	_bone[ AVATAR_BONE_LEFT_PELVIS		].defaultPosePosition = glm::vec3( -0.05,  0.0,  0.0  );
	_bone[ AVATAR_BONE_LEFT_THIGH		].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	_bone[ AVATAR_BONE_LEFT_SHIN		].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	_bone[ AVATAR_BONE_LEFT_FOOT		].defaultPosePosition = glm::vec3(  0.0,   0.0,  0.04 );
	_bone[ AVATAR_BONE_RIGHT_PELVIS		].defaultPosePosition = glm::vec3(  0.05,  0.0,  0.0  );
	_bone[ AVATAR_BONE_RIGHT_THIGH		].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_SHIN		].defaultPosePosition = glm::vec3(  0.0,  -0.15, 0.0  );
	_bone[ AVATAR_BONE_RIGHT_FOOT		].defaultPosePosition = glm::vec3(  0.0,   0.0,  0.04 );

	//----------------------------------------------------------------------------
	// calculate bone length
	//----------------------------------------------------------------------------
	calculateBoneLengths();

	//----------------------------------------------------------------------------
	// generate world positions
	//----------------------------------------------------------------------------
	updateSkeleton();
}




void Head::calculateBoneLengths() {
	for (int b=0; b<NUM_AVATAR_BONES; b++) {
		_bone[b].length = glm::length( _bone[b].defaultPosePosition );
	}

	_avatar.maxArmLength
	= _bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].length
	+ _bone[ AVATAR_BONE_RIGHT_FOREARM	 ].length 
	+ _bone[ AVATAR_BONE_RIGHT_HAND		 ].length;
}

void Head::updateSkeleton() {
	//----------------------------------
	// rotate body...
	//----------------------------------	
	_avatar.orientation.setToIdentity();
	_avatar.orientation.yaw( _bodyYaw );
        
    //test! - make sure this does what expected: st rotation to be identity PLUS _bodyYaw
    //_rotation = glm::angleAxis( _bodyYaw, _avatar.orientation.up );
    
    //glm::quat yaw_rotation = glm::angleAxis( _bodyYaw, _avatar.orientation.up );
    
    
	//------------------------------------------------------------------------
	// calculate positions of all bones by traversing the skeleton tree:
	//------------------------------------------------------------------------
	for (int b=0; b<NUM_AVATAR_BONES; b++) {	
		if ( _bone[b].parent == AVATAR_BONE_NULL ) {
			_bone[b].orientation.set( _avatar.orientation );
			_bone[b].position = _bodyPosition;
		}
		else {
			_bone[b].orientation.set( _bone[ _bone[b].parent ].orientation );
			_bone[b].position = _bone[ _bone[b].parent ].position;
		}
        
        ///TEST! - get this working and then add a comment; JJV
        if ( ! _isMine ) {
            _bone[ AVATAR_BONE_RIGHT_HAND ].position = _handPosition;
        }
                                            
		float xx =  glm::dot( _bone[b].defaultPosePosition, _bone[b].orientation.getRight	() );
		float yy =  glm::dot( _bone[b].defaultPosePosition, _bone[b].orientation.getUp	() );
		float zz = -glm::dot( _bone[b].defaultPosePosition, _bone[b].orientation.getFront	() );

		glm::vec3 rotatedBoneVector( xx, yy, zz );
        
        //glm::vec3 myEuler ( 0.0f, 0.0f, 0.0f );
        //glm::quat myQuat ( myEuler );
        
		_bone[b].position += rotatedBoneVector;
	}	
}


void Head::initializeBodySprings() {
	for (int b=0; b<NUM_AVATAR_BONES; b++) {
		_bone[b].springyPosition = _bone[b].position;
		_bone[b].springyVelocity = glm::vec3( 0.0f, 0.0f, 0.0f );
	}
}


void Head::updateBodySprings( float deltaTime ) {
	for (int b=0; b<NUM_AVATAR_BONES; b++) {
		glm::vec3 springVector( _bone[b].springyPosition );

		if ( _bone[b].parent == AVATAR_BONE_NULL ) {
			springVector -= _bodyPosition;
		}
		else {
			springVector -= _bone[ _bone[b].parent ].springyPosition;
		}

		float length = glm::length( springVector );
		
		if ( length > 0.0f ) {
			glm::vec3 springDirection = springVector / length;
			
			float force = ( length - _bone[b].length ) * _springForce * deltaTime;
			
			_bone[ b						].springyVelocity -= springDirection * force;
			_bone[ _bone[b].parent	].springyVelocity += springDirection * force;
		}
		
		_bone[b].springyVelocity += ( _bone[b].position - _bone[b].springyPosition ) * _bone[b].springBodyTightness * deltaTime;

		float decay = 1.0 - _springVelocityDecay * deltaTime;
		
		if ( decay > 0.0 ) {
			_bone[b].springyVelocity *= decay;
		}
		else {
			_bone[b].springyVelocity = glm::vec3( 0.0f, 0.0f, 0.0f );		
		}

		_bone[b].springyPosition += _bone[b].springyVelocity;
	}
}

glm::vec3 Head::getHeadLookatDirection() {
	return glm::vec3
	(
		_avatar.orientation.getFront().x,
		_avatar.orientation.getFront().y,
		_avatar.orientation.getFront().z
	);
}

glm::vec3 Head::getHeadLookatDirectionUp() {
	return glm::vec3
	(
		_avatar.orientation.getUp().x,
		_avatar.orientation.getUp().y,
		_avatar.orientation.getUp().z
	);
}

glm::vec3 Head::getHeadLookatDirectionRight() {
	return glm::vec3
	(
		_avatar.orientation.getRight().x,
		_avatar.orientation.getRight().y,
		_avatar.orientation.getRight().z
	);
}

glm::vec3 Head::getHeadPosition() {
	return glm::vec3
	(
		_bone[ AVATAR_BONE_HEAD ].position.x,
		_bone[ AVATAR_BONE_HEAD ].position.y,
		_bone[ AVATAR_BONE_HEAD ].position.z
	);
}



void Head::updateHandMovement() {
	glm::vec3 transformedHandMovement;
	
	transformedHandMovement 
	= _avatar.orientation.getRight()	* -_movedHandOffset.x
	+ _avatar.orientation.getUp()	* -_movedHandOffset.y * 0.5f
	+ _avatar.orientation.getFront()	* -_movedHandOffset.y;

	_bone[ AVATAR_BONE_RIGHT_HAND ].position += transformedHandMovement;
    
	//if holding hands, add a pull to the hand...
	if ( _usingSprings ) {
		if ( _closestOtherAvatar != -1 ) {	
			if ( _triggeringAction ) {
				
				/*
				glm::vec3 handShakePull( DEBUG_otherAvatarListPosition[ closestOtherAvatar ]);
				handShakePull -= _bone[ AVATAR_BONE_RIGHT_HAND ].position;
				
				handShakePull *= 1.0;
				 
				transformedHandMovement += handShakePull;
				*/
				_bone[ AVATAR_BONE_RIGHT_HAND ].position = DEBUG_otherAvatarListPosition[ _closestOtherAvatar ];				
			}
		}
	}
	

	//-------------------------------------------------------------------------------
	// determine the arm vector
	//-------------------------------------------------------------------------------
	glm::vec3 armVector = _bone[ AVATAR_BONE_RIGHT_HAND ].position;
	armVector -= _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;


	//-------------------------------------------------------------------------------
	// test to see if right hand is being dragged beyond maximum arm length
	//-------------------------------------------------------------------------------
	float distance = glm::length( armVector );
	
	//-------------------------------------------------------------------------------
	// if right hand is being dragged beyond maximum arm length...
	//-------------------------------------------------------------------------------	
	if ( distance > _avatar.maxArmLength ) {
		//-------------------------------------------------------------------------------
		// reset right hand to be constrained to maximum arm length
		//-------------------------------------------------------------------------------
		_bone[ AVATAR_BONE_RIGHT_HAND ].position = _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
		glm::vec3 armNormal = armVector / distance;
		armVector = armNormal * _avatar.maxArmLength;
		distance = _avatar.maxArmLength;
		glm::vec3 constrainedPosition = _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
		constrainedPosition += armVector;
		_bone[ AVATAR_BONE_RIGHT_HAND ].position = constrainedPosition;
	}
	
	/*
	//-------------------------------------------------------------------------------
	// keep arm from going through av body...
	//-------------------------------------------------------------------------------	
	glm::vec3 adjustedArmVector = _bone[ AVATAR_BONE_RIGHT_HAND ].position;
	adjustedArmVector -= _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
	
	float rightComponent = glm::dot( adjustedArmVector, avatar.orientation.getRight() );

	if ( rightComponent < 0.0 )
	{
		_bone[ AVATAR_BONE_RIGHT_HAND ].position -= avatar.orientation.getRight() * rightComponent;
	}	
	*/
	
	//-----------------------------------------------------------------------------
	// set elbow position 
	//-----------------------------------------------------------------------------
	glm::vec3 newElbowPosition = _bone[ AVATAR_BONE_RIGHT_SHOULDER ].position;
	newElbowPosition += armVector * ONE_HALF;
	glm::vec3 perpendicular = glm::cross( _avatar.orientation.getFront(), armVector );

	newElbowPosition += perpendicular * ( 1.0f - ( _avatar.maxArmLength / distance ) ) * ONE_HALF;
	_bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].position = newElbowPosition;

	//-----------------------------------------------------------------------------
	// set wrist position 
	//-----------------------------------------------------------------------------
	glm::vec3 vv( _bone[ AVATAR_BONE_RIGHT_HAND ].position );
	vv -= _bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].position;
	glm::vec3 newWristPosition = _bone[ AVATAR_BONE_RIGHT_UPPER_ARM ].position;
	newWristPosition += vv * 0.7f;
	_bone[ AVATAR_BONE_RIGHT_FOREARM ].position = newWristPosition;
    
    //  Set the vector we send for hand position to other people to be our right hand
    setHandPosition(_bone[ AVATAR_BONE_RIGHT_HAND ].position);

}



void Head::renderBody() {
	//-----------------------------------------
    //  Render bone positions as spheres
	//-----------------------------------------
	for (int b=0; b<NUM_AVATAR_BONES; b++) {
		if ( _usingSprings ) {
			glColor3fv( lightBlue );
			glPushMatrix();
				glTranslatef( _bone[b].springyPosition.x, _bone[b].springyPosition.y, _bone[b].springyPosition.z );
				glutSolidSphere( _bone[b].radius, 10.0f, 5.0f );
			glPopMatrix();
		}
		else {
			glColor3fv( skinColor );
			glPushMatrix();
				glTranslatef( _bone[b].position.x, _bone[b].position.y, _bone[b].position.z );
				glutSolidSphere( _bone[b].radius, 10.0f, 5.0f );
			glPopMatrix();
		}
	}

	//-----------------------------------------------------
    // Render lines connecting the bone positions
	//-----------------------------------------------------
	if ( _usingSprings ) {
		glColor3f( 0.4f, 0.5f, 0.6f );
		glLineWidth(3.0);

		for (int b=1; b<NUM_AVATAR_BONES; b++) {
			glBegin( GL_LINE_STRIP );
			glVertex3fv( &_bone[ _bone[ b ].parent ].springyPosition.x );
			glVertex3fv( &_bone[ b ].springyPosition.x );
			glEnd();
		}
	}
	else {
		glColor3fv( skinColor );
		glLineWidth(3.0);

		for (int b=1; b<NUM_AVATAR_BONES; b++) {
			glBegin( GL_LINE_STRIP );
			glVertex3fv( &_bone[ _bone[ b ].parent ].position.x );
			glVertex3fv( &_bone[ b ].position.x);
			glEnd();
		}
	}
	
	
	if (( _usingSprings ) && ( _triggeringAction )) {
		glColor4f( 1.0, 1.0, 0.5, 0.5 );
		glPushMatrix();
			glTranslatef
			( 
				_bone[ AVATAR_BONE_RIGHT_HAND ].springyPosition.x, 
				_bone[ AVATAR_BONE_RIGHT_HAND ].springyPosition.y, 
				_bone[ AVATAR_BONE_RIGHT_HAND ].springyPosition.z 
			);
			glutSolidSphere( 0.03f, 10.0f, 5.0f );
		glPopMatrix();
	}
	
}

void Head::SetNewHeadTarget(float pitch, float yaw) {
    PitchTarget = pitch;
    YawTarget = yaw;
}

// getting data from Android transmitte app
void Head::processTransmitterData(unsigned char* packetData, int numBytes) {
    //  Read a packet from a transmitter app, process the data
    float accX, accY, accZ,
    graX, graY, graZ,
    gyrX, gyrY, gyrZ,
    linX, linY, linZ,
    rot1, rot2, rot3, rot4;
    sscanf((char *)packetData, "tacc %f %f %f gra %f %f %f gyr %f %f %f lin %f %f %f rot %f %f %f %f",
           &accX, &accY, &accZ,
           &graX, &graY, &graZ,
           &gyrX, &gyrY, &gyrZ,
           &linX, &linY, &linZ,
           &rot1, &rot2, &rot3, &rot4);
    
    if (transmitterPackets++ == 0) {
        gettimeofday(&transmitterTimer, NULL);
    }
    const int TRANSMITTER_COUNT = 100;
    if (transmitterPackets % TRANSMITTER_COUNT == 0) {
        // Every 100 packets, record the observed Hz of the transmitter data
        timeval now;
        gettimeofday(&now, NULL);
        double msecsElapsed = diffclock(&transmitterTimer, &now);
        transmitterHz = static_cast<float>( (double)TRANSMITTER_COUNT/(msecsElapsed/1000.0) );
        transmitterTimer = now;
    }
    /*  NOTE:  PR:  Will add back in when ready to animate avatar hand

    //  Add rotational forces to the hand
    const float ANG_VEL_SENSITIVITY = 4.0;
    const float ANG_VEL_THRESHOLD = 0.0;
    float angVelScale = ANG_VEL_SENSITIVITY*(1.0f/getTransmitterHz());
    
    addAngularVelocity(fabs(gyrX*angVelScale)>ANG_VEL_THRESHOLD?gyrX*angVelScale:0,
                       fabs(gyrZ*angVelScale)>ANG_VEL_THRESHOLD?gyrZ*angVelScale:0,
                       fabs(-gyrY*angVelScale)>ANG_VEL_THRESHOLD?-gyrY*angVelScale:0);
    
    //  Add linear forces to the hand
    //const float LINEAR_VEL_SENSITIVITY = 50.0;
    const float LINEAR_VEL_SENSITIVITY = 5.0;
    float linVelScale = LINEAR_VEL_SENSITIVITY*(1.0f/getTransmitterHz());
    glm::vec3 linVel(linX*linVelScale, linZ*linVelScale, -linY*linVelScale);
    addVelocity(linVel);
    */
    
}

