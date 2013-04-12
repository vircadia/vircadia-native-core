//  
//  Interface
//
//  Allows you to connect to and see/hear the shared 3D space.
//  Optionally uses serialUSB connection to get gyro data for head movement.
//  Optionally gets UDP stream from transmitter to animate controller/hand.
//  
//  Usage:  The interface client first attempts to contact a domain server to
//          discover the appropriate audio, voxel, and avatar servers to contact.
//          Right now, the default domain server is "highfidelity.below92.com"
//          You can change the domain server to use your own by editing the
//          DOMAIN_HOSTNAME or DOMAIN_IP strings in the file AgentList.cpp
//
//
//  Welcome Aboard!
//
//
//  Keyboard Commands: 
//
//  / = toggle stats display
//  spacebar = reset gyros/head position
//  h = render Head facing yourself (mirror)
//  l = show incoming gyro levels
//

#include "InterfaceConfig.h"
#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#else
#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

#include <pthread.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Field.h"
#include "world.h"
#include "Util.h"
#ifndef _WIN32
#include "Audio.h"
#endif

#include "FieldOfView.h"
#include "Stars.h"

#include "MenuRow.h"
#include "MenuColumn.h"
#include "Menu.h"
#include "Head.h"
#include "Hand.h"
#include "Camera.h"
#include "Particle.h"
#include "Texture.h"
#include "Cloud.h"
#include <AgentList.h>
#include <AgentTypes.h>
#include "VoxelSystem.h"
#include "Oscilloscope.h"
#include "UDPSocket.h"
#include "SerialInterface.h"
#include <PerfStat.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>

using namespace std;

pthread_t networkReceiveThread;
bool stopNetworkReceiveThread = false;

int packetCount = 0;
int packetsPerSecond = 0; 
int bytesPerSecond = 0;
int bytesCount = 0;

int headMirror = 1;                 //  Whether to mirror own head when viewing it

int WIDTH = 1200;                   //  Window size
int HEIGHT = 800;
int fullscreen = 0;

bool wantColorRandomizer = true;    // for addSphere and load file

Oscilloscope audioScope(256,200,true);

Head myAvatar;                       //  The rendered avatar of oneself
Camera myCamera;					//  My view onto the world (sometimes on myself :)

                                    //  Starfield information
char starFile[] = "https://s3-us-west-1.amazonaws.com/highfidelity/stars.txt";
FieldOfView fov;
Stars stars;
#ifdef STARFIELD_KEYS
int starsTiles = 20;
double starsLod = 1.0;
#endif

bool showingVoxels = true;

glm::vec3 box(WORLD_SIZE,WORLD_SIZE,WORLD_SIZE);

ParticleSystem balls(0,
                     box, 
                     false,                //  Wrap?
                     0.02f,                //  Noise
                     0.3f,                 //  Size scale 
                     0.0                   //  Gravity
                     );

Cloud cloud(0,                             //  Particles
            box,                           //  Bounding Box
            false                          //  Wrap
            );

VoxelSystem voxels;
Field field;

#ifndef _WIN32
Audio audio(&audioScope, &myAvatar);
#endif

#define IDLE_SIMULATE_MSECS 8            //  How often should call simulate and other stuff 
                                         //  in the idle loop?

float yaw = 0.f;                         //  The yaw, pitch for the avatar head
float pitch = 0.f;                            
float startYaw = 122.f;
float renderPitch = 0.f;
float renderYawRate = 0.f;
float renderPitchRate = 0.f; 

//  Where one's own agent begins in the world (needs to become a dynamic thing passed to the program)
glm::vec3 start_location(6.1f, 0, 1.4f);

int statsOn = 0;					//  Whether to show onscreen text overlay with stats
bool starsOn = false;				//  Whether to display the stars
bool paintOn = false;				//  Whether to paint voxels as you fly around
VoxelDetail paintingVoxel;			//	The voxel we're painting if we're painting
unsigned char dominantColor = 0;	//	The dominant color of the voxel we're painting
bool perfStatsOn = false;			//  Do we want to display perfStats?
bool frustumOn = false;				//  Whether or not to display the debug view frustum
bool cameraFrustum = false;			// which frustum to look at
bool wantFrustumDebugging = false;  // enable for some stdout debugging output

bool viewFrustumFromOffset=false;   //  Wether or not to offset the view of the frustum
float viewFrustumOffsetYaw = -90.0;
float viewFrustumOffsetPitch = 7.5;
float viewFrustumOffsetRoll = 0.0;

int noiseOn = 0;					//  Whether to add random noise 
float noise = 1.0;                  //  Overall magnitude scaling for random noise levels 

int displayLevels = 0;
int displayHead = 0;
int displayField = 0;

int displayHeadMouse = 1;         //  Display sample mouse pointer controlled by head movement
int headMouseX, headMouseY; 

int mouseX, mouseY;				//  Where is the mouse
int mouseStartX, mouseStartY;   //  Mouse location at start of last down click
int mousePressed = 0;				//  true if mouse has been pressed (clear when finished)

Menu menu;                          // main menu
int menuOn = 1;					//  Whether to show onscreen menu

//
//  Serial USB Variables
// 

SerialInterface serialPort;

glm::vec3 gravity;

//  Frame Rate Measurement

int frameCount = 0;                  
float FPS = 120.f;
timeval timerStart, timerEnd;
timeval lastTimeIdle;
double elapsedTime;
timeval applicationStartupTime;
bool justStarted = true;


//  Every second, check the frame rates and other stuff
void Timer(int extra)
{
    gettimeofday(&timerEnd, NULL);
    FPS = (float)frameCount / ((float)diffclock(&timerStart, &timerEnd) / 1000.f);
    packetsPerSecond = (float)packetCount / ((float)diffclock(&timerStart, &timerEnd) / 1000.f);
    bytesPerSecond = (float)bytesCount / ((float)diffclock(&timerStart, &timerEnd) / 1000.f);
   	frameCount = 0;
    packetCount = 0;
    bytesCount = 0;
    
	glutTimerFunc(1000,Timer,0);
    gettimeofday(&timerStart, NULL);
    
    // if we haven't detected gyros, check for them now
    if (!serialPort.active) {
        serialPort.pair();
    }
}

void displayStats(void)
{
    int statsVerticalOffset = 50;
    if (::menuOn == 0) {
        statsVerticalOffset = 8;
    }
	//  bitmap chars are about 10 pels high 
    char legend[] = "/ - toggle this display, Q - exit, H - show head, M - show hand, T - test audio";
    drawtext(10, statsVerticalOffset + 15, 0.10f, 0, 1.0, 0, legend);

    char legend2[] = "* - toggle stars, & - toggle paint mode, '-' - send erase all, '%' - send add scene";
    drawtext(10, statsVerticalOffset + 32, 0.10f, 0, 1.0, 0, legend2);

	glm::vec3 avatarPos = myAvatar.getPos();
    
    char stats[200];
    sprintf(stats, "FPS = %3.0f  Pkts/s = %d  Bytes/s = %d Head(x,y,z)= %4.2f, %4.2f, %4.2f ", 
            FPS, packetsPerSecond,  bytesPerSecond, avatarPos.x,avatarPos.y,avatarPos.z);
    drawtext(10, statsVerticalOffset + 49, 0.10f, 0, 1.0, 0, stats); 
    if (serialPort.active) {
        sprintf(stats, "ADC samples = %d, LED = %d", 
                serialPort.getNumSamples(), serialPort.getLED());
        drawtext(300, statsVerticalOffset + 30, 0.10f, 0, 1.0, 0, stats);
    }
    
    std::stringstream voxelStats;
    voxelStats << "Voxels Rendered: " << voxels.getVoxelsRendered();
    drawtext(10, statsVerticalOffset + 70, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());

	voxelStats.str("");
	voxelStats << "Voxels Created: " << voxels.getVoxelsCreated() << " (" << voxels.getVoxelsCreatedRunningAverage() 
		<< "/sec in last "<< COUNTETSTATS_TIME_FRAME << " seconds) ";
    drawtext(10, statsVerticalOffset + 250, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());

	voxelStats.str("");
	voxelStats << "Voxels Colored: " << voxels.getVoxelsColored() << " (" << voxels.getVoxelsColoredRunningAverage() 
		<< "/sec in last "<< COUNTETSTATS_TIME_FRAME << " seconds) ";
    drawtext(10, statsVerticalOffset + 270, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
	
	voxelStats.str("");
	voxelStats << "Voxels Bytes Read: " << voxels.getVoxelsBytesRead()  
		<< " (" << voxels.getVoxelsBytesReadRunningAverage() << "/sec in last "<< COUNTETSTATS_TIME_FRAME << " seconds) ";
    drawtext(10, statsVerticalOffset + 290,0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());

	voxelStats.str("");
	long int voxelsBytesPerColored = voxels.getVoxelsColored() ? voxels.getVoxelsBytesRead()/voxels.getVoxelsColored() : 0;
	long int voxelsBytesPerColoredAvg = voxels.getVoxelsColoredRunningAverage() ? 
		voxels.getVoxelsBytesReadRunningAverage()/voxels.getVoxelsColoredRunningAverage() : 0;

	voxelStats << "Voxels Bytes per Colored: " << voxelsBytesPerColored  
		<< " (" << voxelsBytesPerColoredAvg << "/sec in last "<< COUNTETSTATS_TIME_FRAME << " seconds) ";
    drawtext(10, statsVerticalOffset + 310, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
	

	if (::perfStatsOn) {
		// Get the PerfStats group details. We need to allocate and array of char* long enough to hold 1+groups
		char** perfStatLinesArray = new char*[PerfStat::getGroupCount()+1];
		int lines = PerfStat::DumpStats(perfStatLinesArray);
		int atZ = 150; // arbitrary place on screen that looks good
		for (int line=0; line < lines; line++) {
			drawtext(10, statsVerticalOffset + atZ, 0.10f, 0, 1.0, 0, perfStatLinesArray[line]);
			delete perfStatLinesArray[line]; // we're responsible for cleanup
			perfStatLinesArray[line]=NULL;
			atZ+=20; // height of a line
		}
		delete []perfStatLinesArray; // we're responsible for cleanup
	}
}

void initDisplay(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel (GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    
    if (fullscreen) glutFullScreen();
}

void init(void)
{
    voxels.init();
    voxels.setViewerHead(&myAvatar);
    myAvatar.setRenderYaw(startYaw);

    headMouseX = WIDTH/2;
    headMouseY = HEIGHT/2; 

    stars.readInput(starFile, 0);
 
    //  Initialize Field values
    field = Field();
 
    if (noiseOn) {   
        myAvatar.setNoise(noise);
    }
    myAvatar.setPos(start_location );
	myCamera.setPosition( start_location );
    
	
#ifdef MARKER_CAPTURE
    if(marker_capture_enabled){
        marker_capturer.position_updated(&position_updated);
        marker_capturer.frame_updated(&marker_frame_available);
        if(!marker_capturer.init_capture()){
            printf("Camera-based marker capture initialized.\n");
        }else{
            printf("Error initializing camera-based marker capture.\n");
        }
    }
#endif
    
    gettimeofday(&timerStart, NULL);
    gettimeofday(&lastTimeIdle, NULL);
}

void terminate () {
    // Close serial port
    //close(serial_fd);

    #ifndef _WIN32
    audio.terminate();
    #endif
    stopNetworkReceiveThread = true;
    pthread_join(networkReceiveThread, NULL);
    
    exit(EXIT_SUCCESS);
}

void reset_sensors()
{
    //  
    //   Reset serial I/O sensors 
    // 
    myAvatar.setRenderYaw(startYaw);
    
    yaw = renderYawRate = 0; 
    pitch = renderPitch = renderPitchRate = 0;
    myAvatar.setPos(start_location);
    headMouseX = WIDTH/2;
    headMouseY = HEIGHT/2;
    
    myAvatar.reset();
    
    if (serialPort.active) {
        serialPort.resetTrailingAverages();
    }
}

void simulateHand(float deltaTime) {
    //  If mouse is being dragged, send current force to the hand controller
    if (mousePressed == 1)
    {
        //  Add a velocity to the hand corresponding to the detected size of the drag vector
        const float MOUSE_HAND_FORCE = 1.5;
        float dx = mouseX - mouseStartX;
        float dy = mouseY - mouseStartY;
        glm::vec3 vel(dx*MOUSE_HAND_FORCE, -dy*MOUSE_HAND_FORCE*(WIDTH/HEIGHT), 0);
        myAvatar.hand->addVelocity(vel*deltaTime);
    }
}

void simulateHead(float frametime)
//  Using serial data, update avatar/render position and angles
{
//    float measured_pitch_rate = serialPort.getRelativeValue(PITCH_RATE);
//    float measured_yaw_rate = serialPort.getRelativeValue(YAW_RATE);
    
    float measured_pitch_rate = 0;
    float measured_yaw_rate = 0;
    
    //float measured_lateral_accel = serialPort.getRelativeValue(ACCEL_X);
    //float measured_fwd_accel = serialPort.getRelativeValue(ACCEL_Z);
    
    myAvatar.UpdatePos(frametime, &serialPort, headMirror, &gravity);
		
    //  Update head_mouse model 
    const float MIN_MOUSE_RATE = 30.0;
    const float MOUSE_SENSITIVITY = 0.1f;
    if (powf(measured_yaw_rate*measured_yaw_rate + 
             measured_pitch_rate*measured_pitch_rate, 0.5) > MIN_MOUSE_RATE)
    {
        headMouseX += measured_yaw_rate*MOUSE_SENSITIVITY;
        headMouseY += measured_pitch_rate*MOUSE_SENSITIVITY*(float)HEIGHT/(float)WIDTH; 
    }
    headMouseX = max(headMouseX, 0);
    headMouseX = min(headMouseX, WIDTH);
    headMouseY = max(headMouseY, 0);
    headMouseY = min(headMouseY, HEIGHT);
    
    //  Update render direction (pitch/yaw) based on measured gyro rates
    const int MIN_YAW_RATE = 100;
    const int MIN_PITCH_RATE = 100;
    
    const float YAW_SENSITIVITY = 0.02;
    const float PITCH_SENSITIVITY = 0.05;
    
    //  Update render pitch and yaw rates based on keyPositions
    const float KEY_YAW_SENSITIVITY = 2.0;
    if (myAvatar.getDriveKeys(ROT_LEFT)) renderYawRate -= KEY_YAW_SENSITIVITY*frametime;
    if (myAvatar.getDriveKeys(ROT_RIGHT)) renderYawRate += KEY_YAW_SENSITIVITY*frametime;
    
    if (fabs(measured_yaw_rate) > MIN_YAW_RATE)  
    {   
        if (measured_yaw_rate > 0)
            renderYawRate += (measured_yaw_rate - MIN_YAW_RATE) * YAW_SENSITIVITY * frametime;
        else 
            renderYawRate += (measured_yaw_rate + MIN_YAW_RATE) * YAW_SENSITIVITY * frametime;
    }
    if (fabs(measured_pitch_rate) > MIN_PITCH_RATE) 
    {
        if (measured_pitch_rate > 0)
            renderPitchRate += (measured_pitch_rate - MIN_PITCH_RATE) * PITCH_SENSITIVITY * frametime;
        else 
            renderPitchRate += (measured_pitch_rate + MIN_PITCH_RATE) * PITCH_SENSITIVITY * frametime;
    }
         
    renderPitch += renderPitchRate;
    
    // Decay renderPitch toward zero because we never look constantly up/down 
    renderPitch *= (1.f - 2.0*frametime);

    //  Decay angular rates toward zero 
    renderPitchRate *= (1.f - 5.0*frametime);
    renderYawRate *= (1.f - 7.0*frametime);
    
    //  Update own head data
    myAvatar.setRenderYaw(myAvatar.getRenderYaw() + renderYawRate);
    myAvatar.setRenderPitch(renderPitch);
    
    //  Get audio loudness data from audio input device
    float loudness, averageLoudness;
    #ifndef _WIN32
    audio.getInputLoudness(&loudness, &averageLoudness);
    myAvatar.setLoudness(loudness);
    myAvatar.setAverageLoudness(averageLoudness);
    #endif

    //  Send my stream of head/hand data to the avatar mixer and voxel server
    char broadcastString[200];
    int broadcastBytes = myAvatar.getBroadcastData(broadcastString);
    const char broadcastReceivers[2] = {AGENT_TYPE_VOXEL, AGENT_TYPE_AVATAR_MIXER};
    
    AgentList::getInstance()->broadcastToAgents(broadcastString, broadcastBytes, broadcastReceivers, 2);

    // If I'm in paint mode, send a voxel out to VOXEL server agents.
    if (::paintOn) {
    
    	glm::vec3 avatarPos = myAvatar.getPos();

		// For some reason, we don't want to flip X and Z here.
		::paintingVoxel.x = avatarPos.x/10.0;  
		::paintingVoxel.y = avatarPos.y/10.0;  
		::paintingVoxel.z = avatarPos.z/10.0;
    	
    	unsigned char* bufferOut;
    	int sizeOut;
    	
		if (::paintingVoxel.x >= 0.0 && ::paintingVoxel.x <= 1.0 &&
			::paintingVoxel.y >= 0.0 && ::paintingVoxel.y <= 1.0 &&
			::paintingVoxel.z >= 0.0 && ::paintingVoxel.z <= 1.0) {

			if (createVoxelEditMessage(PACKET_HEADER_SET_VOXEL, 0, 1, &::paintingVoxel, bufferOut, sizeOut)){
                AgentList::getInstance()->broadcastToAgents((char*)bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
				delete bufferOut;
			}
		}
    }
}



/////////////////////////////////////////////////////////////////////////////////////
// render_view_frustum()
//
// Description: this will render the view frustum bounds for EITHER the head
// 				or the "myCamera". It appears as if the orientation that comes
//				from these two sources is in different coordinate spaces (namely)
// 				their axis orientations don't match.
void render_view_frustum() {

	//printf("frustum low.x=%f, low.y=%f, low.z=%f, high.x=%f, high.y=%f, high.z=%f\n",low.x,low.y,low.z,high.x,high.y,high.z);

	// p – the camera position
	// d – a vector with the direction of the camera's view ray. In here it is assumed that this vector has been normalized
	// nearDist – the distance from the camera to the near plane
	// nearHeight – the height of the near plane
	// nearWidth – the width of the near plane
	// farDist – the distance from the camera to the far plane
	// farHeight – the height of the far plane
	// farWidth – the width of the far plane
	
	
	// Some explanation here.
	
	glm::vec3 cameraPosition   = ::myCamera.getPosition(); 
	glm::vec3 headPosition     = ::myAvatar.getHeadPosition();

	glm::vec3 cameraDirection = ::myCamera.getOrientation().getFront() * glm::vec3(1,1,-1);
	glm::vec3 headDirection    = myAvatar.getHeadLookatDirection(); // direction still backwards


	glm::vec3 cameraUp  = myCamera.getOrientation().getUp() * glm::vec3(1,1,1);
	glm::vec3 headUp    = myAvatar.getHeadLookatDirectionUp();

	glm::vec3 cameraRight = myCamera.getOrientation().getRight() * glm::vec3(1,1,-1);
	glm::vec3 headRight = myAvatar.getHeadLookatDirectionRight() * glm::vec3(-1,1,-1); // z is flipped!

	// Debug these vectors!
	if (::wantFrustumDebugging) { 
		printf("\nPosition:\n");
		printf("cameraPosition=%f, cameraPosition=%f, cameraPosition=%f\n",cameraPosition.x,cameraPosition.y,cameraPosition.z);
		printf("headPosition.x=%f, headPosition.y=%f, headPosition.z=%f\n",headPosition.x,headPosition.y,headPosition.z);
		printf("\nDirection:\n");
		printf("cameraDirection.x=%f, cameraDirection.y=%f, cameraDirection.z=%f\n",cameraDirection.x,cameraDirection.y,cameraDirection.z);
		printf("headDirection.x=%f, headDirection.y=%f, headDirection.z=%f\n",headDirection.x,headDirection.y,headDirection.z);
		printf("\nUp:\n");
		printf("cameraUp.x=%f, cameraUp.y=%f, cameraUp.z=%f\n",cameraUp.x,cameraUp.y,cameraUp.z);
		printf("headUp.x=%f, headUp.y=%f, headUp.z=%f\n",headUp.x,headUp.y,headUp.z);
		printf("\nRight:\n");
		printf("cameraRight.x=%f, cameraRight.y=%f, cameraRight.z=%f\n",cameraRight.x,cameraRight.y,cameraRight.z);
		printf("headRight.x=%f, headRight.y=%f, headRight.z=%f\n",headRight.x,headRight.y,headRight.z);
	}
	
	// We will use these below, from either the camera or head vectors calculated above	
	glm::vec3 viewFrustumPosition;
	glm::vec3 viewFrustumDirection;
	glm::vec3 up;
	glm::vec3 right;
	
	// Camera or Head?
	if (::cameraFrustum) {
		viewFrustumPosition = cameraPosition;
		viewFrustumDirection = cameraDirection;
		up = cameraUp;
		right = cameraRight;
	} else {
		viewFrustumPosition = headPosition;
		viewFrustumDirection = headDirection;
		up = headUp;
		right = headRight;
	}
	
	float nearDist = 0.1; 
	float farDist  = 1.0;
	
	float fovHalfAngle = 0.7854f; // 45 deg for half, so fov = 90 deg
	
	float screenWidth = ::WIDTH; // These values come from reshape() 
	float screenHeight = ::HEIGHT;
	float ratio      = screenWidth/screenHeight;
	
	float nearHeight = 2 * tan(fovHalfAngle) * nearDist;
	float nearWidth  = nearHeight * ratio;
	float farHeight  = 2 * tan(fovHalfAngle) * farDist;
	float farWidth   = farHeight * ratio;
	
	glm::vec3 farCenter       = viewFrustumPosition+viewFrustumDirection*farDist;
	glm::vec3 farTopLeft      = farCenter  + (up*farHeight*0.5)  - (right*farWidth*0.5); 
	glm::vec3 farTopRight     = farCenter  + (up*farHeight*0.5)  + (right*farWidth*0.5); 
	glm::vec3 farBottomLeft   = farCenter  - (up*farHeight*0.5)  - (right*farWidth*0.5); 
	glm::vec3 farBottomRight  = farCenter  - (up*farHeight*0.5)  + (right*farWidth*0.5); 

	glm::vec3 nearCenter      = viewFrustumPosition+viewFrustumDirection*nearDist;
	glm::vec3 nearTopLeft     = nearCenter + (up*nearHeight*0.5) - (right*nearWidth*0.5); 
	glm::vec3 nearTopRight    = nearCenter + (up*nearHeight*0.5) + (right*nearWidth*0.5); 
	glm::vec3 nearBottomLeft  = nearCenter - (up*nearHeight*0.5) - (right*nearWidth*0.5); 
	glm::vec3 nearBottomRight = nearCenter - (up*nearHeight*0.5) + (right*nearWidth*0.5); 
	
	// At this point we have all the corners for our frustum... we could use these to
	// calculate various things...
	
	// But, here we're just going to draw them for now

    //  Get ready to draw some lines
    glDisable(GL_LIGHTING);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glLineWidth(1.0);
    glBegin(GL_LINES);

    glm::vec3 headLookingAt      = headPosition+(headDirection*5.0);
    glm::vec3 headLookingAtUp    = headPosition+(headUp*5.0);
    glm::vec3 headLookingAtRight = headPosition+(headRight*5.0);

	// Looking At from head = white
    glColor3f(1,1,1);
    glVertex3f(headPosition.x,headPosition.y,headPosition.z);
    glVertex3f(headLookingAt.x,headLookingAt.y,headLookingAt.z);

	// up from head = purple
    glColor3f(1,0,1);
    glVertex3f(headPosition.x,headPosition.y,headPosition.z);
    glVertex3f(headLookingAtUp.x,headLookingAtUp.y,headLookingAtUp.z);

	// right from head = cyan
    glColor3f(0,1,1);
    glVertex3f(headPosition.x,headPosition.y,headPosition.z);
    glVertex3f(headLookingAtRight.x,headLookingAtRight.y,headLookingAtRight.z);

    glm::vec3 cameraLookingAt      = cameraPosition+(cameraDirection*5.0);
    glm::vec3 cameraLookingAtUp    = cameraPosition+(cameraUp*5.0);
    glm::vec3 cameraLookingAtRight = cameraPosition+(cameraRight*5.0);

	// Looking At from camera = white
    glColor3f(1,1,1);
    glVertex3f(cameraPosition.x,cameraPosition.y,cameraPosition.z);
    glVertex3f(cameraLookingAt.x,cameraLookingAt.y,cameraLookingAt.z);

	// up from camera = purple
    glColor3f(1,0,1);
    glVertex3f(cameraPosition.x,cameraPosition.y,cameraPosition.z);
    glVertex3f(cameraLookingAtUp.x,cameraLookingAtUp.y,cameraLookingAtUp.z);

	// right from camera = cyan
    glColor3f(0,1,1);
    glVertex3f(cameraPosition.x,cameraPosition.y,cameraPosition.z);
    glVertex3f(cameraLookingAtRight.x,cameraLookingAtRight.y,cameraLookingAtRight.z);

	// The remaining lines are skinny
    //glLineWidth(1.0);
	// near plane - bottom edge 
    glColor3f(1,0,0);
    glVertex3f(nearBottomLeft.x,nearBottomLeft.y,nearBottomLeft.z);
    glVertex3f(nearBottomRight.x,nearBottomRight.y,nearBottomRight.z);

	// near plane - top edge
    glVertex3f(nearTopLeft.x,nearTopLeft.y,nearTopLeft.z);
    glVertex3f(nearTopRight.x,nearTopRight.y,nearTopRight.z);

	// near plane - right edge
    glVertex3f(nearBottomRight.x,nearBottomRight.y,nearBottomRight.z);
    glVertex3f(nearTopRight.x,nearTopRight.y,nearTopRight.z);

	// near plane - left edge
    glVertex3f(nearBottomLeft.x,nearBottomLeft.y,nearBottomLeft.z);
    glVertex3f(nearTopLeft.x,nearTopLeft.y,nearTopLeft.z);

	// far plane - bottom edge 
    glColor3f(0,1,0); // GREEN!!!
    glVertex3f(farBottomLeft.x,farBottomLeft.y,farBottomLeft.z);
    glVertex3f(farBottomRight.x,farBottomRight.y,farBottomRight.z);

	// far plane - top edge
    glVertex3f(farTopLeft.x,farTopLeft.y,farTopLeft.z);
    glVertex3f(farTopRight.x,farTopRight.y,farTopRight.z);

	// far plane - right edge
    glVertex3f(farBottomRight.x,farBottomRight.y,farBottomRight.z);
    glVertex3f(farTopRight.x,farTopRight.y,farTopRight.z);

	// far plane - left edge
    glVertex3f(farBottomLeft.x,farBottomLeft.y,farBottomLeft.z);
    glVertex3f(farTopLeft.x,farTopLeft.y,farTopLeft.z);

	// RIGHT PLANE IS CYAN
	// right plane - bottom edge - near to distant 
    glColor3f(0,1,1);
    glVertex3f(nearBottomRight.x,nearBottomRight.y,nearBottomRight.z);
    glVertex3f(farBottomRight.x,farBottomRight.y,farBottomRight.z);

	// right plane - top edge - near to distant
    glVertex3f(nearTopRight.x,nearTopRight.y,nearTopRight.z);
    glVertex3f(farTopRight.x,farTopRight.y,farTopRight.z);

	// LEFT PLANE IS BLUE
	// left plane - bottom edge - near to distant
    glColor3f(0,0,1);
    glVertex3f(nearBottomLeft.x,nearBottomLeft.y,nearBottomLeft.z);
    glVertex3f(farBottomLeft.x,farBottomLeft.y,farBottomLeft.z);

	// left plane - top edge - near to distant
    glVertex3f(nearTopLeft.x,nearTopLeft.y,nearTopLeft.z);
    glVertex3f(farTopLeft.x,farTopLeft.y,farTopLeft.z);
	
    glEnd();
}


void display(void)
{
	//printf( "avatar head lookat = %f, %f, %f\n", myAvatar.getAvatarHeadLookatDirection().x, myAvatar.getAvatarHeadLookatDirection().y, myAvatar.getAvatarHeadLookatDirection().z );

	PerfStat("display");

    glEnable(GL_LINE_SMOOTH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    
    glPushMatrix();  {
        glLoadIdentity();
    
        //  Setup 3D lights
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        
        GLfloat light_position0[] = { 1.0, 1.0, 0.0, 0.0 };
        glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
        GLfloat ambient_color[] = { 0.7, 0.7, 0.8 };  //{ 0.125, 0.305, 0.5 };  
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color);
        GLfloat diffuse_color[] = { 0.8, 0.7, 0.7 };  //{ 0.5, 0.42, 0.33 }; 
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color);
        GLfloat specular_color[] = { 1.0, 1.0, 1.0, 1.0};
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular_color);
        
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular_color);
        glMateriali(GL_FRONT, GL_SHININESS, 96);

		//--------------------------------------------------------
		// camera settings
		//--------------------------------------------------------		
		myCamera.setTargetPosition( myAvatar.getPos() ); 

		if ( displayHead ) {
			//-----------------------------------------------
			// set the camera to looking at my own face
			//-----------------------------------------------		
			myCamera.setYaw		( - myAvatar.getBodyYaw() );
			myCamera.setPitch	( 0.0  );
			myCamera.setRoll	( 0.0  );
			myCamera.setUp		( 0.4  );	
			myCamera.setDistance( 0.03 );
			myCamera.update();
		} else {
			//----------------------------------------------------
			// set the camera to third-person view behind my av
			//----------------------------------------------------		
			myCamera.setYaw		( 180.0 - myAvatar.getBodyYaw() );
			myCamera.setPitch	(   0.0 );
			myCamera.setRoll	(   0.0 );
			myCamera.setUp		(   0.2 );
			myCamera.setDistance(   1.6 );	
			myCamera.setDistance(   0.5 );
			myCamera.update();
		}
		// Note: whichCamera is used to pick between the normal camera myCamera for our 
		// main camera, vs, an alternate camera. The alternate camera we support right now
		// is the viewFrustumOffsetCamera. But theoretically, we could use this same mechanism
		// to add other cameras.
		//
		// Why have two cameras? Well, one reason is that because in the case of the render_view_frustum()
		// code, we want to keep the state of "myCamera" intact, so we can render what the view frustum of
		// myCamera is. But we also want to do meaningful camera transforms on OpenGL for the offset camera
		Camera whichCamera = myCamera;
		Camera viewFrustumOffsetCamera = myCamera;
		
		if (::viewFrustumFromOffset && ::frustumOn) {
			//----------------------------------------------------
			// set the camera to third-person view but offset so we can see the frustum
			//----------------------------------------------------		
			viewFrustumOffsetCamera.setYaw		( 180.0 - myAvatar.getBodyYaw() + ::viewFrustumOffsetYaw );
			viewFrustumOffsetCamera.setPitch	(   0.0 + ::viewFrustumOffsetPitch );
			viewFrustumOffsetCamera.setRoll	(   0.0 + ::viewFrustumOffsetRoll  ); 
			viewFrustumOffsetCamera.setUp		(   0.2 + 0.2 );
			viewFrustumOffsetCamera.setDistance(   0.5 + 0.2 );
			viewFrustumOffsetCamera.update();
			
			whichCamera = viewFrustumOffsetCamera;
		}		
	
		//---------------------------------------------
		// transform view according to whichCamera
		// could be myCamera (if in normal mode)
		// or could be viewFrustumOffsetCamera if in offset mode
		//---------------------------------------------
        glRotatef	( whichCamera.getPitch(),	1, 0, 0 );
        glRotatef	( whichCamera.getYaw(),	    0, 1, 0 );
        glRotatef	( whichCamera.getRoll(),	0, 0, 1 );
        glTranslatef( -whichCamera.getPosition().x, -whichCamera.getPosition().y, -whichCamera.getPosition().z );

        if (::starsOn) {
            // should be the first rendering pass - w/o depth buffer / lighting
        	stars.render(fov);
        }

        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        
		
		// draw a red sphere  
		float sphereRadius = 0.25f;
        glColor3f(1,0,0);
		glPushMatrix();
			//glTranslatef( 0.0f, sphereRadius, 0.0f );
			glutSolidSphere( sphereRadius, 15, 15 );
		glPopMatrix();

		//draw a grid gound plane....
		drawGroundPlaneGrid( 5.0f, 9 );
		
		
        //  Draw cloud of dots
        if (!displayHead) cloud.render();
    
        //  Draw voxels
		if ( showingVoxels )
		{
			voxels.render();
		}
		
        //  Draw field vectors
        if (displayField) field.render();
            
        //  Render avatars of other agents
        AgentList *agentList = AgentList::getInstance();
        for(std::vector<Agent>::iterator agent = agentList->getAgents().begin();
            agent != agentList->getAgents().end();
            agent++) {
            if (agent->getLinkedData() != NULL) {
                Head *agentHead = (Head *)agent->getLinkedData();
                glPushMatrix();
                glm::vec3 pos = agentHead->getPos();
                glTranslatef(-pos.x, -pos.y, -pos.z);
                agentHead->render(0, 0);
                glPopMatrix();
            }
        }
    
        if ( !displayHead ) balls.render();
    
        //  Render the world box
        if (!displayHead && statsOn) render_world_box();
        
        // brad's frustum for debugging
        if (::frustumOn) render_view_frustum();
    
	
        //Render my own avatar
		myAvatar.render( true, 1 );	
    }
    
    glPopMatrix();

    //  Render 2D overlay:  I/O level bar graphs and text  
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity(); 
        gluOrtho2D(0, WIDTH, HEIGHT, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
    
        #ifndef _WIN32
        audio.render(WIDTH, HEIGHT);
        if (audioScope.getState()) audioScope.render();
        #endif

        if (displayHeadMouse && !displayHead && statsOn) {
            //  Display small target box at center or head mouse target that can also be used to measure LOD
            glColor3f(1.0, 1.0, 1.0);
            glDisable(GL_LINE_SMOOTH);
            const int PIXEL_BOX = 20;
            glBegin(GL_LINE_STRIP);
            glVertex2f(headMouseX - PIXEL_BOX/2, headMouseY - PIXEL_BOX/2);
            glVertex2f(headMouseX + PIXEL_BOX/2, headMouseY - PIXEL_BOX/2);
            glVertex2f(headMouseX + PIXEL_BOX/2, headMouseY + PIXEL_BOX/2);
            glVertex2f(headMouseX - PIXEL_BOX/2, headMouseY + PIXEL_BOX/2);
            glVertex2f(headMouseX - PIXEL_BOX/2, headMouseY - PIXEL_BOX/2);
            glEnd();            
            glEnable(GL_LINE_SMOOTH);
        }
        
    //  Show detected levels from the serial I/O ADC channel sensors
    if (displayLevels) serialPort.renderLevels(WIDTH,HEIGHT);
    
    //  Display miscellaneous text stats onscreen
    if (statsOn) {
        glLineWidth(1.0f);
        glPointSize(1.0f);
        displayStats();
    }
        
    //  Show menu
    if (::menuOn) {
        glLineWidth(1.0f);
        glPointSize(1.0f);
        menu.render(WIDTH,HEIGHT);
    }

    //  Draw number of nearby people always
    glPointSize(1.0f);
    char agents[100];
    sprintf(agents, "Agents: %ld\n", AgentList::getInstance()->getAgents().size());
    drawtext(WIDTH-100,20, 0.10, 0, 1.0, 0, agents, 1, 0, 0);
    
    if (::paintOn) {
    
		char paintMessage[100];
		sprintf(paintMessage,"Painting (%.3f,%.3f,%.3f/%.3f/%d,%d,%d)",
			::paintingVoxel.x,::paintingVoxel.y,::paintingVoxel.z,::paintingVoxel.s,
			(unsigned int)::paintingVoxel.red,(unsigned int)::paintingVoxel.green,(unsigned int)::paintingVoxel.blue);
		drawtext(WIDTH-350,50, 0.10, 0, 1.0, 0, paintMessage, 1, 1, 0);
    }
    
    glPopMatrix();
    

    glutSwapBuffers();
    frameCount++;
    
    //  If application has just started, report time from startup to now (first frame display)
    if (justStarted) {
        printf("Startup Time: %4.2f\n",
               (usecTimestampNow() - usecTimestamp(&applicationStartupTime))/1000000.0);
        justStarted = false;
    }
}

int setValue(int state, int *value) {
    if (state == -2) {
        *value = !(*value);
    } else if (state == -1) {
        return *value;
    } else {
        *value = state;
    }
    return *value;
}

int setHead(int state) {
    return setValue(state, &displayHead);
}

int setField(int state) {
    return setValue(state, &displayField);
}

int setNoise(int state) {
    int iRet = setValue(state, &noiseOn);
    if (noiseOn) {
        myAvatar.setNoise(noise);
    } else {
        myAvatar.setNoise(0);
    }
    return iRet;
}

int setStats(int state) {
    return setValue(state, &statsOn);
}

int setMenu(int state) {
    return setValue(state, &::menuOn);
}

int setMirror(int state) {
    return setValue(state, &headMirror);
}

void initMenu() {
    MenuColumn *menuColumnOptions, *menuColumnTools, *menuColumnDebug;
    //  Options
    menuColumnOptions = menu.addColumn("Options");
    menuColumnOptions->addRow("Head", setHead); 
    menuColumnOptions->addRow("Field", setField); 
    menuColumnOptions->addRow("Noise", setNoise); 
    menuColumnOptions->addRow("Mirror", setMirror);
    //  Tools
    menuColumnTools = menu.addColumn("Tools");
    menuColumnTools->addRow("Stats", setStats); 
    menuColumnTools->addRow("Menu", setMenu);
    // Debug
    menuColumnDebug = menu.addColumn("Debug");
    
}

void testPointToVoxel()
{
	float y=0;
	float z=0;
	float s=0.1;
	for (float x=0; x<=1; x+= 0.05)
	{
		std::cout << " x=" << x << " ";

		unsigned char red   = 200; //randomColorValue(65);
		unsigned char green = 200; //randomColorValue(65);
		unsigned char blue  = 200; //randomColorValue(65);
	
		unsigned char* voxelCode = pointToVoxel(x, y, z, s,red,green,blue);
		printVoxelCode(voxelCode);
		delete voxelCode;
		std::cout << std::endl;
	}
}

void sendVoxelServerEraseAll() {
	char message[100];
    sprintf(message,"%c%s",'Z',"erase all");
	int messageSize = strlen(message) + 1;
	AgentList::getInstance()->broadcastToAgents(message, messageSize, &AGENT_TYPE_VOXEL, 1);
}

void sendVoxelServerAddScene() {
	char message[100];
    sprintf(message,"%c%s",'Z',"add scene");
	int messageSize = strlen(message) + 1;
	AgentList::getInstance()->broadcastToAgents(message, messageSize, &AGENT_TYPE_VOXEL, 1);
}

void shiftPaintingColor()
{
    // About the color of the paintbrush... first determine the dominant color
    ::dominantColor = (::dominantColor+1)%3; // 0=red,1=green,2=blue
	::paintingVoxel.red   = (::dominantColor==0)?randIntInRange(200,255):randIntInRange(40,100);
	::paintingVoxel.green = (::dominantColor==1)?randIntInRange(200,255):randIntInRange(40,100);
	::paintingVoxel.blue  = (::dominantColor==2)?randIntInRange(200,255):randIntInRange(40,100);
}

void setupPaintingVoxel() {
	glm::vec3 avatarPos = myAvatar.getPos();

	::paintingVoxel.x = avatarPos.z/-10.0;	// voxel space x is negative z head space
	::paintingVoxel.y = avatarPos.y/-10.0;  // voxel space y is negative y head space
	::paintingVoxel.z = avatarPos.x/-10.0;  // voxel space z is negative x head space
	::paintingVoxel.s = 1.0/256;
	
	shiftPaintingColor();
}

void addRandomSphere(bool wantColorRandomizer)
{
	float r = randFloatInRange(0.05,0.1);
	float xc = randFloatInRange(r,(1-r));
	float yc = randFloatInRange(r,(1-r));
	float zc = randFloatInRange(r,(1-r));
	float s = 0.001; // size of voxels to make up surface of sphere
	bool solid = false;

	printf("random sphere\n");
	printf("radius=%f\n",r);
	printf("xc=%f\n",xc);
	printf("yc=%f\n",yc);
	printf("zc=%f\n",zc);

	voxels.createSphere(r,xc,yc,zc,s,solid,wantColorRandomizer);
}


const float KEYBOARD_YAW_RATE = 0.8;
const float KEYBOARD_PITCH_RATE = 0.6;
const float KEYBOARD_STRAFE_RATE = 0.03;
const float KEYBOARD_FLY_RATE = 0.08;

void specialkeyUp(int k, int x, int y) {
    if (k == GLUT_KEY_UP) {
        myAvatar.setDriveKeys(FWD, 0);
        myAvatar.setDriveKeys(UP, 0);
    }
    if (k == GLUT_KEY_DOWN) {
        myAvatar.setDriveKeys(BACK, 0);
        myAvatar.setDriveKeys(DOWN, 0);
    }
    if (k == GLUT_KEY_LEFT) {
        myAvatar.setDriveKeys(LEFT, 0);
        myAvatar.setDriveKeys(ROT_LEFT, 0);
    }
    if (k == GLUT_KEY_RIGHT) {
        myAvatar.setDriveKeys(RIGHT, 0);
        myAvatar.setDriveKeys(ROT_RIGHT, 0);
    }
    
}

void specialkey(int k, int x, int y)
{
    if (k == GLUT_KEY_UP || k == GLUT_KEY_DOWN || k == GLUT_KEY_LEFT || k == GLUT_KEY_RIGHT) {
        if (k == GLUT_KEY_UP) {
            if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) myAvatar.setDriveKeys(UP, 1);
            else myAvatar.setDriveKeys(FWD, 1);
        }
        if (k == GLUT_KEY_DOWN) {
            if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) myAvatar.setDriveKeys(DOWN, 1);
            else myAvatar.setDriveKeys(BACK, 1);
        }
        if (k == GLUT_KEY_LEFT) {
            if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) myAvatar.setDriveKeys(LEFT, 1);
            else myAvatar.setDriveKeys(ROT_LEFT, 1);  
        }
        if (k == GLUT_KEY_RIGHT) {
            if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) myAvatar.setDriveKeys(RIGHT, 1);
            else myAvatar.setDriveKeys(ROT_RIGHT, 1);   
        }
#ifndef _WIN32
        audio.setWalkingState(true);
#endif
    }    
}


void keyUp(unsigned char k, int x, int y) {
    if (k == 'e') myAvatar.setDriveKeys(UP, 0);
    if (k == 'c') myAvatar.setDriveKeys(DOWN, 0);
    if (k == 'w') myAvatar.setDriveKeys(FWD, 0);
    if (k == 's') myAvatar.setDriveKeys(BACK, 0);
    if (k == 'a') myAvatar.setDriveKeys(ROT_LEFT, 0);
    if (k == 'd') myAvatar.setDriveKeys(ROT_RIGHT, 0);

}

void key(unsigned char k, int x, int y)
{
    
	//  Process keypresses 
 	if (k == 'q')  ::terminate();
    if (k == '/')  statsOn = !statsOn;		// toggle stats
    if (k == '*')  ::starsOn = !::starsOn;		// toggle stars
    if (k == 'V')  ::showingVoxels = !::showingVoxels;		// toggle voxels
    if (k == 'F')  ::frustumOn = !::frustumOn;		// toggle view frustum debugging
    if (k == 'C')  ::cameraFrustum = !::cameraFrustum;	// toggle which frustum to look at
    if (k == 'G')  ::viewFrustumFromOffset = !::viewFrustumFromOffset;	// toggle view frustum from offset debugging
    
	if (k == '[') ::viewFrustumOffsetYaw   -= 0.5;
	if (k == ']') ::viewFrustumOffsetYaw   += 0.5;
	if (k == '{') ::viewFrustumOffsetPitch -= 0.5;
	if (k == '}') ::viewFrustumOffsetPitch += 0.5;
	if (k == '(') ::viewFrustumOffsetRoll  -= 0.5;
	if (k == ')') ::viewFrustumOffsetRoll  += 0.5;

    if (k == '&') {
    	::paintOn = !::paintOn;		// toggle paint
    	::setupPaintingVoxel();		// also randomizes colors
    }
    if (k == '^')  ::shiftPaintingColor();		// shifts randomize color between R,G,B dominant
    if (k == '-')  ::sendVoxelServerEraseAll();	// sends erase all command to voxel server
    if (k == '%')  ::sendVoxelServerAddScene();	// sends add scene command to voxel server
	if (k == 'n') 
    {
        noiseOn = !noiseOn;                   // Toggle noise 
        if (noiseOn)
        {
            myAvatar.setNoise(noise);
        }
        else 
        {
            myAvatar.setNoise(0);
        }

    }
    
    if (k == 'h') {
        displayHead = !displayHead;
        #ifndef _WIN32
        audio.setMixerLoopbackFlag(displayHead);
        #endif
    }
    
    if (k == 'm') setMenu(-2);
    
    if (k == 'f') displayField = !displayField;
    if (k == 'l') displayLevels = !displayLevels;
    if (k == 'e') myAvatar.setDriveKeys(UP, 1);
    if (k == 'c') myAvatar.setDriveKeys(DOWN, 1);
    if (k == 'w') myAvatar.setDriveKeys(FWD, 1);
    if (k == 's') myAvatar.setDriveKeys(BACK, 1);
    if (k == ' ') reset_sensors();
    if (k == 't') renderPitchRate -= KEYBOARD_PITCH_RATE;
    if (k == 'g') renderPitchRate += KEYBOARD_PITCH_RATE;
#ifdef STARFIELD_KEYS
    if (k == 'u') stars.setResolution(starsTiles += 1);
    if (k == 'j') stars.setResolution(starsTiles = max(starsTiles-1,1));
    if (k == 'i') if (starsLod < 1.0) starsLod = stars.changeLOD(1.01);
    if (k == 'k') if (starsLod > 0.01) starsLod = stars.changeLOD(0.99);
    if (k == 'r') stars.readInput(starFile, 0);
#endif
    if (k == 'a') myAvatar.setDriveKeys(ROT_LEFT, 1); 
    if (k == 'd') myAvatar.setDriveKeys(ROT_RIGHT, 1);

	// press the . key to get a new random sphere of voxels added 
    if (k == '.') addRandomSphere(wantColorRandomizer);
}

//  Receive packets from other agents/servers and decide what to do with them!
void *networkReceive(void *args)
{    
    sockaddr senderAddress;
    ssize_t bytesReceived;
    char *incomingPacket = new char[MAX_PACKET_SIZE];

    while (!stopNetworkReceiveThread) {
        if (AgentList::getInstance()->getAgentSocket().receive(&senderAddress, incomingPacket, &bytesReceived)) {
            packetCount++;
            bytesCount += bytesReceived;
            
            switch (incomingPacket[0]) {
                case PACKET_HEADER_TRANSMITTER_DATA:
                    myAvatar.hand->processTransmitterData(incomingPacket, bytesReceived);
                    break;
                case PACKET_HEADER_VOXEL_DATA:
                case PACKET_HEADER_Z_COMMAND:
                case PACKET_HEADER_ERASE_VOXEL:
                    voxels.parseData(incomingPacket, bytesReceived);
                    break;
                case PACKET_HEADER_BULK_AVATAR_DATA:
                    AgentList::getInstance()->processBulkAgentData(&senderAddress, incomingPacket, bytesReceived, sizeof(float) * 11);
                    break;
                default:
                    AgentList::getInstance()->processAgentData(&senderAddress, incomingPacket, bytesReceived);
                    break;
            }
        }
    }
    
    delete[] incomingPacket;
    pthread_exit(0); 
    return NULL;
}

void idle(void)
{
    timeval check;
    gettimeofday(&check, NULL);
    
    //  Only run simulation code if more than IDLE_SIMULATE_MSECS have passed since last time
    
    if (diffclock(&lastTimeIdle, &check) > IDLE_SIMULATE_MSECS)
    {
		// If mouse is being dragged, update hand movement in the avatar
		if ( mousePressed == 1 )
		{
			float xOffset = ( mouseX - mouseStartX ) / (double)WIDTH;
			float yOffset = ( mouseY - mouseStartY ) / (double)HEIGHT;
			
			float leftRight	= xOffset;
			float downUp	= yOffset;
			float backFront	= 0.0;
			
			glm::vec3 handMovement( leftRight, downUp, backFront );
			myAvatar.setHandMovement( handMovement );		
		}		
		
        //  Simulation
        simulateHead(1.f/FPS);
		
		
		//test
		/*
		//  simulate the other agents
        for(std::vector<Agent>::iterator agent = agentList.getAgents().begin(); agent != agentList.getAgents().end(); agent++) 
		{
            if (agent->getLinkedData() != NULL) 
			{
                Head *agentHead = (Head *)agent->getLinkedData();
                agentHead->simulate(1.f/FPS);
            }
        }
		*/
		
        simulateHand(1.f/FPS);
        
        field.simulate(1.f/FPS);
        myAvatar.simulate(1.f/FPS);
        balls.simulate(1.f/FPS);
        cloud.simulate(1.f/FPS);

        glutPostRedisplay();
        lastTimeIdle = check;
        
    }
    
    //  Read serial data 
    if (serialPort.active) {
        serialPort.readData();
    }
}



void reshape(int width, int height)
{
    WIDTH = width;
    HEIGHT = height; 


    glMatrixMode(GL_PROJECTION); //hello
    fov.setResolution(width, height)
            .setBounds(glm::vec3(-0.5f,-0.5f,-500.0f), glm::vec3(0.5f, 0.5f, 0.1f) )
            .setPerspective(0.7854f);
    glLoadMatrixf(glm::value_ptr(fov.getViewerScreenXform()));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(0, 0, width, height);
}



void mouseFunc( int button, int state, int x, int y ) 
{
    if( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
        if (!menu.mouseClick(x, y)) {
            mouseX = x;
            mouseY = y;
            mousePressed = 1;
            mouseStartX = x;
            mouseStartY = y;
        }
    }
	if( button == GLUT_LEFT_BUTTON && state == GLUT_UP ) {
        mouseX = x;
        mouseY = y;
        mousePressed = 0;
    }
	
}

void motionFunc( int x, int y)
{
	mouseX = x;
	mouseY = y;
}

void mouseoverFunc( int x, int y)
{
    menu.mouseOver(x, y);
	mouseX = x;
	mouseY = y;
    if (mousePressed == 0)
    {}
}

void attachNewHeadToAgent(Agent *newAgent) {
    if (newAgent->getLinkedData() == NULL) {
        newAgent->setLinkedData(new Head());
    }
}

#ifndef _WIN32
void audioMixerUpdate(in_addr_t newMixerAddress, in_port_t newMixerPort) {
    audio.updateMixerParams(newMixerAddress, newMixerPort);
}
#endif

int main(int argc, const char * argv[])
{
    AgentList::createInstance(AGENT_TYPE_INTERFACE);
    
    gettimeofday(&applicationStartupTime, NULL);
    const char* domainIP = getCmdOption(argc, argv, "--domain");
    if (domainIP) {
		strcpy(DOMAIN_IP,domainIP);
	}

    // Handle Local Domain testing with the --local command line
    if (cmdOptionExists(argc, argv, "--local")) {
    	printf("Local Domain MODE!\n");
		int ip = getLocalAddress();
		sprintf(DOMAIN_IP,"%d.%d.%d.%d", (ip & 0xFF), ((ip >> 8) & 0xFF),((ip >> 16) & 0xFF), ((ip >> 24) & 0xFF));
    }
    
    // the callback for our instance of AgentList is attachNewHeadToAgent
    AgentList::getInstance()->linkedDataCreateCallback = &attachNewHeadToAgent;
    
    #ifndef _WIN32
    AgentList::getInstance()->audioMixerSocketUpdate = &audioMixerUpdate;
    #endif
    
#ifdef _WIN32
    WSADATA WsaData;
    int wsaresult = WSAStartup( MAKEWORD(2,2), &WsaData );
#endif

    // start the agentList threads
    AgentList::getInstance()->startSilentAgentRemovalThread();
    AgentList::getInstance()->startDomainServerCheckInThread();
    AgentList::getInstance()->startPingUnknownAgentsThread();

    glutInit(&argc, (char**)argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Interface");
    
    #ifdef _WIN32
    glewInit();
    #endif

    printf( "Created Display Window.\n" );
        
    initMenu();
    initDisplay();
    printf( "Initialized Display.\n" );

    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialkey);
    glutSpecialUpFunc(specialkeyUp);
	glutMotionFunc(motionFunc);
    glutPassiveMotionFunc(mouseoverFunc);
	glutMouseFunc(mouseFunc);
    glutIdleFunc(idle);
	

    init();
    printf( "Init() complete.\n" );


	// Check to see if the user passed in a command line option for randomizing colors
	if (cmdOptionExists(argc, argv, "--NoColorRandomizer")) {
		wantColorRandomizer = false;
	}
	
	// Check to see if the user passed in a command line option for loading a local
	// Voxel File. If so, load it now.
    const char* voxelsFilename = getCmdOption(argc, argv, "-i");
    if (voxelsFilename) {
	    voxels.loadVoxelsFile(voxelsFilename,wantColorRandomizer);
        printf("Local Voxel File loaded.\n");
	}
    
    // create thread for receipt of data via UDP
    pthread_create(&networkReceiveThread, NULL, networkReceive, NULL);
    printf("Network receive thread created.\n");
    
    glutTimerFunc(1000, Timer, 0);
    glutMainLoop();

    printf("Normal exit.\n");
    ::terminate();
    return EXIT_SUCCESS;
}   

