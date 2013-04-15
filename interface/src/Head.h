//
//  Head.h
//  interface
//
//  Created by Philip Rosedale on 9/11/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__head__
#define __interface__head__

#include <iostream>

#include <AvatarData.h>
#include <Orientation.h>	// added by Ventrella as a utility

#include "Field.h"
#include "world.h"

#include "InterfaceConfig.h"
#include "SerialInterface.h"

enum eyeContactTargets {LEFT_EYE, RIGHT_EYE, MOUTH};

#define FWD 0
#define BACK 1 
#define LEFT 2 
#define RIGHT 3 
#define UP 4 
#define DOWN 5
#define ROT_LEFT 6 
#define ROT_RIGHT 7 
#define MAX_DRIVE_KEYS 8

#define NUM_OTHER_AVATARS 5

class Head : public AvatarData {
    public:
        Head();
        ~Head();
        Head(const Head &otherHead);
        Head* clone() const;
    
        void reset();
        void UpdatePos(float frametime, SerialInterface * serialInterface, int head_mirror, glm::vec3 * gravity);
        void setNoise (float mag) { noise = mag; }
        void setPitch(float p) {Pitch = p; }
        void setYaw(float y) {Yaw = y; }
        void setRoll(float r) {Roll = r; };
        void setScale(float s) {scale = s; };
        void setRenderYaw(float y) {renderYaw = y;}
        void setRenderPitch(float p) {renderPitch = p;}
        float getRenderYaw() {return renderYaw;}
        float getRenderPitch() {return renderPitch;}
        void setLeanForward(float dist);
        void setLeanSideways(float dist);
        void addPitch(float p) {Pitch -= p; }
        void addYaw(float y){Yaw -= y; }
        void addRoll(float r){Roll += r; }
        void addLean(float x, float z);
        float getPitch() {return Pitch;}
        float getRoll() {return Roll;}
        float getYaw() {return Yaw;}
        float getLastMeasuredYaw() {return YawRate;}
		
		float getBodyYaw();
		glm::vec3 getHeadLookatDirection();
		glm::vec3 getHeadLookatDirectionUp();
		glm::vec3 getHeadLookatDirectionRight();
		glm::vec3 getHeadPosition();
		glm::vec3 getBonePosition( AvatarBones b );
		glm::vec3 getBodyPosition();
        
        void render(int faceToFace, int isMine);
		
		void renderBody();
		void renderHead( int faceToFace, int isMine );
		void renderOrientationDirections( glm::vec3 position, Orientation orientation, float size );

        void simulate(float);
				
		void setHandMovement( glm::vec3 movement );
		void updateHandMovement();
        
        //  Send and receive network data
        int getBroadcastData(char * data);
        void parseData(void *data, int size);
        
        float getLoudness() {return loudness;};
        float getAverageLoudness() {return averageLoudness;};
        void setAverageLoudness(float al) {averageLoudness = al;};
        void setLoudness(float l) {loudness = l;};
        
        void SetNewHeadTarget(float, float);
        glm::vec3 getPos() { return position; };
        void setPos(glm::vec3 newpos) { position = newpos; };
    
        //  Set what driving keys are being pressed to control thrust levels
        void setDriveKeys(int key, bool val) { driveKeys[key] = val; };
        bool getDriveKeys(int key) { return driveKeys[key]; };
    
        //  Set/Get update the thrust that will move the avatar around
        void setThrust(glm::vec3 newThrust) { avatar.thrust = newThrust; };
        void addThrust(glm::vec3 newThrust) { avatar.thrust += newThrust; };
        glm::vec3 getThrust() { return avatar.thrust; };
    
        //
        //  Related to getting transmitter UDP data used to animate the avatar hand
        //

        void processTransmitterData(char * packetData, int numBytes);
        float getTransmitterHz() { return transmitterHz; };
    
    private:
        float noise;
        float Pitch;
        float Yaw;
        float Roll;
        float PitchRate;
        float YawRate;
        float RollRate;
        float EyeballPitch[2];
        float EyeballYaw[2];
        float EyebrowPitch[2];
        float EyebrowRoll[2];
        float EyeballScaleX, EyeballScaleY, EyeballScaleZ;
        float interPupilDistance;
        float interBrowDistance;
        float NominalPupilSize;
        float PupilSize;
        float MouthPitch;
        float MouthYaw;
        float MouthWidth;
        float MouthHeight;
        float leanForward;
        float leanSideways;
        float PitchTarget; 
        float YawTarget; 
        float NoiseEnvelope;
        float PupilConverge;
        float scale;
        
        //  Sound loudness information
        float loudness, lastLoudness;
        float averageLoudness;
        float audioAttack;
        float browAudioLift;
    
        glm::vec3 position;
		
		float bodyYaw;
		float bodyPitch;
		float bodyRoll;
		float bodyYawDelta;
		
		float		closeEnoughToInteract;
		int			closestOtherAvatar;
		glm::vec3	DEBUG_otherAvatarListPosition	[ NUM_OTHER_AVATARS ];
		float		DEBUG_otherAvatarListTimer		[ NUM_OTHER_AVATARS ];
		
		bool usingSprings;
		
		bool handBeingMoved;
		bool previousHandBeingMoved;
		glm::vec3 movedHandOffset;
		//glm::vec3 movedHandPosition;
    
        int driveKeys[MAX_DRIVE_KEYS];
		
		float springVelocityDecay;
		float springForce;
		float springToBodyTightness; // XXXBHG - this had been commented out, but build breaks without it.
        
        int eyeContact;
        eyeContactTargets eyeContactTarget;
    
        GLUquadric *sphere;
		Avatar avatar;
		
		void initializeAvatar();
		void updateAvatarSkeleton();
		void initializeAvatarSprings();
		void updateAvatarSprings( float deltaTime );
		void calculateBoneLengths();
    
        void readSensors();
        float renderYaw, renderPitch;       //   Pitch from view frustum when this is own head.
    
        //
        //  Related to getting transmitter UDP data used to animate the avatar hand
        //
        timeval transmitterTimer;
        float transmitterHz;
        int transmitterPackets;

};

#endif
