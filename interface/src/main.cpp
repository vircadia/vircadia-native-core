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

#include "Log.h"
#include "shared_Log.h"
#include "voxels_Log.h"
#include "avatars_Log.h"

#include "Field.h"
#include "world.h"
#include "Util.h"
#ifndef _WIN32
#include "Audio.h"
#endif

#include "AngleUtil.h"
#include "Stars.h"

#include "MenuRow.h"
#include "MenuColumn.h"
#include "Menu.h"
#include "Camera.h"
#include "Head.h"
#include "Particle.h"
#include "Texture.h"
#include "Cloud.h"
#include <AgentList.h>
#include <AgentTypes.h>
#include "VoxelSystem.h"
#include "Oscilloscope.h"
#include "UDPSocket.h"
#include "SerialInterface.h"
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <AvatarData.h>
#include <PerfStat.h>
#include <SimpleMovingAverage.h>

#include "ViewFrustum.h"

using namespace std;

void reshape(int width, int height); // will be defined below


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
float aspectRatio = 1.0f;

bool wantColorRandomizer = true;    // for addSphere and load file

Oscilloscope audioScope(256,200,true);

ViewFrustum viewFrustum;			// current state of view frustum, perspective, orientation, etc.

Head myAvatar(true);                // The rendered avatar of oneself
Camera myCamera;                    // My view onto the world (sometimes on myself :)
Camera viewFrustumOffsetCamera;     // The camera we use to sometimes show the view frustum from an offset mode

//  Starfield information
char starFile[] = "https://s3-us-west-1.amazonaws.com/highfidelity/stars.txt";
char starCacheFile[] = "cachedStars.txt";
Stars stars;

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

float startYaw = 122.f;
float renderYawRate = 0.f;
float renderPitchRate = 0.f; 

//  Where one's own agent begins in the world (needs to become a dynamic thing passed to the program)
glm::vec3 start_location(6.1f, 0, 1.4f);

bool statsOn = false;               //  Whether to show onscreen text overlay with stats
bool starsOn = false;               //  Whether to display the stars
bool paintOn = false;               //  Whether to paint voxels as you fly around
VoxelDetail paintingVoxel;          //	The voxel we're painting if we're painting
unsigned char dominantColor = 0;    //	The dominant color of the voxel we're painting
bool perfStatsOn = false;			//  Do we want to display perfStats?

int noiseOn = 0;					//  Whether to add random noise 
float noise = 1.0;                  //  Overall magnitude scaling for random noise levels 

int displayLevels = 0;
int displayHead = 0;
int displayField = 0;

int displayHeadMouse = 1;         //  Display sample mouse pointer controlled by head movement
int headMouseX, headMouseY; 

int mouseX = 0;
int mouseY = 0;

//  Mouse location at start of last down click
int mousePressed = 0; //  true if mouse has been pressed (clear when finished)

Menu menu;       // main menu
int menuOn = 1;  //  Whether to show onscreen menu

struct HandController
{
    bool  enabled;
    int   startX;
    int   startY;
    int   x; 
    int   y;
    int   lastX; 
    int   lastY;
    int   velocityX;
    int   velocityY;
    float rampUpRate;
    float rampDownRate;
    float envelope;
};

HandController handController;

void initializeHandController() {
   handController.enabled      = false;
   handController.startX       = WIDTH  / 2;
   handController.startY       = HEIGHT / 2;
   handController.x            = 0; 
   handController.y            = 0;
   handController.lastX        = 0; 
   handController.lastY        = 0;
   handController.velocityX    = 0;
   handController.velocityY    = 0;
   handController.rampUpRate   = 0.05;
   handController.rampDownRate = 0.02;
   handController.envelope     = 0.0f;
}

void updateHandController( int x, int y ) {
    handController.lastX = handController.x;
    handController.lastY = handController.y;
    handController.x = x;
    handController.y = y;
    handController.velocityX = handController.x - handController.lastX;
    handController.velocityY = handController.y - handController.lastY;

    if (( handController.velocityX != 0 )
    ||  ( handController.velocityY != 0 )) {
        handController.enabled = true;
        myAvatar.startHandMovement();
        if ( handController.envelope < 1.0 ) {
            handController.envelope += handController.rampUpRate;
            if ( handController.envelope >= 1.0 ) { 
                handController.envelope = 1.0; 
            }
        }
    }

   if ( ! handController.enabled ) {
        if ( handController.envelope > 0.0 ) {
            handController.envelope -= handController.rampDownRate;
            if ( handController.envelope <= 0.0 ) { 
                handController.startX = WIDTH	 / 2;
                handController.startY = HEIGHT / 2;
                handController.envelope = 0.0; 
//prototype                
//myAvatar.stopHandMovement();
            }
        }
    }
    
    if ( handController.envelope > 0.0 ) {
        float leftRight	= ( ( handController.x - handController.startX ) / (float)WIDTH  ) * handController.envelope;
        float downUp	= ( ( handController.y - handController.startY ) / (float)HEIGHT ) * handController.envelope;
        float backFront	= 0.0;			
        myAvatar.setHandMovementValues( glm::vec3( leftRight, downUp, backFront ) );		
    }
}



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

	glm::vec3 avatarPos = myAvatar.getBodyPosition();
    
    char stats[200];
    sprintf(stats, "FPS = %3.0f  Pkts/s = %d  Bytes/s = %d Head(x,y,z)= %4.2f, %4.2f, %4.2f ", 
            FPS, packetsPerSecond,  bytesPerSecond, avatarPos.x,avatarPos.y,avatarPos.z);
    drawtext(10, statsVerticalOffset + 49, 0.10f, 0, 1.0, 0, stats);
    
    std::stringstream voxelStats;
    voxelStats << "Voxels Rendered: " << voxels.getVoxelsRendered();
    drawtext(10, statsVerticalOffset + 70, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
	voxelStats.str("");
	voxelStats << "Voxels Created: " << voxels.getVoxelsCreated() << " (" << voxels.getVoxelsCreatedPerSecondAverage()
    << "/sec) ";
    drawtext(10, statsVerticalOffset + 250, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
	voxelStats.str("");
	voxelStats << "Voxels Colored: " << voxels.getVoxelsColored() << " (" << voxels.getVoxelsColoredPerSecondAverage()
    << "/sec) ";
    drawtext(10, statsVerticalOffset + 270, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
	voxelStats.str("");
	voxelStats << "Voxels Bytes Read: " << voxels.getVoxelsBytesRead()
    << " (" << voxels.getVoxelsBytesReadPerSecondAverage() << " Bps)";
    drawtext(10, statsVerticalOffset + 290,0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());

	voxelStats.str("");
	float voxelsBytesPerColored = voxels.getVoxelsColored()
        ? ((float) voxels.getVoxelsBytesRead() / voxels.getVoxelsColored())
        : 0;
    
	voxelStats << "Voxels Bytes per Colored: " << voxelsBytesPerColored;
    drawtext(10, statsVerticalOffset + 310, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
    Agent *avatarMixer = AgentList::getInstance()->soloAgentOfType(AGENT_TYPE_AVATAR_MIXER);
    char avatarMixerStats[200];
    if (avatarMixer) {
        sprintf(avatarMixerStats, "Avatar Mixer - %.f kbps, %.f pps",
                roundf(avatarMixer->getAverageKilobitsPerSecond()),
                roundf(avatarMixer->getAveragePacketsPerSecond()));
    } else {
        sprintf(avatarMixerStats, "No Avatar Mixer");
    }
    drawtext(10, statsVerticalOffset + 330, 0.10f, 0, 1.0, 0, avatarMixerStats);
    
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
    
    initializeHandController();

    headMouseX = WIDTH/2;
    headMouseY = HEIGHT/2; 

    stars.readInput(starFile, starCacheFile, 0);
 
    //  Initialize Field values
    field = Field();
 
    if (noiseOn) {   
        myAvatar.setNoise(noise);
    }
    myAvatar.setBodyPosition(start_location);
	myCamera.setPosition( start_location );
    
	
#ifdef MARKER_CAPTURE
    if(marker_capture_enabled){
        marker_capturer.position_updated(&position_updated);
        marker_capturer.frame_updated(&marker_frame_available);
        if(!marker_capturer.init_capture()){
            printLog("Camera-based marker capture initialized.\n");
        }else{
            printLog("Error initializing camera-based marker capture.\n");
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
    
    renderYawRate = 0; 
    renderPitchRate = 0;
    myAvatar.setBodyPosition(start_location);
    headMouseX = WIDTH/2;
    headMouseY = HEIGHT/2;
    
    myAvatar.reset();
    
    if (serialPort.active) {
        serialPort.resetTrailingAverages();
    }
}

//
//  Using gyro data, update both view frustum and avatar head position
//
void updateAvatar(float frametime)
{
    float gyroPitchRate = serialPort.getRelativeValue(HEAD_PITCH_RATE);
    float gyroYawRate   = serialPort.getRelativeValue(HEAD_YAW_RATE  );
    
    myAvatar.UpdateGyros(frametime, &serialPort, headMirror, &gravity);
		
    //  
    //  Update gyro-based mouse (X,Y on screen)
    // 
    const float MIN_MOUSE_RATE = 30.0;
    const float MOUSE_SENSITIVITY = 0.1f;
    if (powf(gyroYawRate*gyroYawRate + 
             gyroPitchRate*gyroPitchRate, 0.5) > MIN_MOUSE_RATE)
    {
        headMouseX += gyroYawRate*MOUSE_SENSITIVITY;
        headMouseY += gyroPitchRate*MOUSE_SENSITIVITY*(float)HEIGHT/(float)WIDTH; 
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
    
    if (fabs(gyroYawRate) > MIN_YAW_RATE)  
    {   
        if (gyroYawRate > 0)
            renderYawRate += (gyroYawRate - MIN_YAW_RATE) * YAW_SENSITIVITY * frametime;
        else 
            renderYawRate += (gyroYawRate + MIN_YAW_RATE) * YAW_SENSITIVITY * frametime;
    }
    if (fabs(gyroPitchRate) > MIN_PITCH_RATE) 
    {
        if (gyroPitchRate > 0)
            renderPitchRate += (gyroPitchRate - MIN_PITCH_RATE) * PITCH_SENSITIVITY * frametime;
        else 
            renderPitchRate += (gyroPitchRate + MIN_PITCH_RATE) * PITCH_SENSITIVITY * frametime;
    }
         
    float renderPitch = myAvatar.getRenderPitch();
    // Decay renderPitch toward zero because we never look constantly up/down 
    renderPitch *= (1.f - 2.0*frametime);

    //  Decay angular rates toward zero 
    renderPitchRate *= (1.f - 5.0*frametime);
    renderYawRate *= (1.f - 7.0*frametime);
    
    //  Update own avatar data
    myAvatar.setRenderYaw(myAvatar.getRenderYaw() + renderYawRate);
    myAvatar.setRenderPitch(renderPitch + renderPitchRate);
    
    //  Get audio loudness data from audio input device
    float loudness, averageLoudness;
    #ifndef _WIN32
    audio.getInputLoudness(&loudness, &averageLoudness);
    myAvatar.setLoudness(loudness);
    myAvatar.setAverageLoudness(averageLoudness);
    #endif

    //  Send my stream of head/hand data to the avatar mixer and voxel server
    unsigned char broadcastString[200];
    *broadcastString = PACKET_HEADER_HEAD_DATA;
    
    int broadcastBytes = myAvatar.getBroadcastData(broadcastString + 1);
    broadcastBytes++;
    
    const char broadcastReceivers[2] = {AGENT_TYPE_VOXEL, AGENT_TYPE_AVATAR_MIXER};
    
    AgentList::getInstance()->broadcastToAgents(broadcastString, broadcastBytes, broadcastReceivers, 2);

    // If I'm in paint mode, send a voxel out to VOXEL server agents.
    if (::paintOn) {
    
    	glm::vec3 avatarPos = myAvatar.getBodyPosition();

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
                AgentList::getInstance()->broadcastToAgents(bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
				delete bufferOut;
			}
		}
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// renderViewFrustum()
//
// Description: this will load the view frustum bounds for EITHER the head
// 				or the "myCamera". 
//

// These global scoped variables are used by our loadViewFrustum() and renderViewFrustum functions below, but are also
//  available as globals so that the keyboard and menu can manipulate them.

bool  frustumOn = false;                  // Whether or not to display the debug view frustum
bool  cameraFrustum = true;               // which frustum to look at

bool  viewFrustumFromOffset     =false;   // Wether or not to offset the view of the frustum
float viewFrustumOffsetYaw      = -135.0; // the following variables control yaw, pitch, roll and distance form regular
float viewFrustumOffsetPitch    = 0.0;    // camera to the offset camera
float viewFrustumOffsetRoll     = 0.0;
float viewFrustumOffsetDistance = 25.0;
float viewFrustumOffsetUp       = 0.0;

void loadViewFrustum(ViewFrustum& viewFrustum) {
	// We will use these below, from either the camera or head vectors calculated above	
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 up;
	glm::vec3 right;
	float fov, nearClip, farClip;
	float yaw, pitch, roll;
	
	// Camera or Head?
	if (::cameraFrustum) {
		position    = ::myCamera.getPosition();
	} else {
		position    = ::myAvatar.getHeadPosition();
	}
    
    // This bit of hackery is all because our Cameras report the incorrect yaw.
    // For whatever reason, the camera has a yaw set to 180.0-trueYaw, so we basically
    // need to get the "yaw" from the camera and adjust it to be the trueYaw
    yaw         =  -(::myCamera.getOrientation().getYaw()-180);
    pitch       = ::myCamera.getOrientation().getPitch();
    roll        = ::myCamera.getOrientation().getRoll();
    fov         = ::myCamera.getFieldOfView();
    nearClip    = ::myCamera.getNearClip();
    farClip     = ::myCamera.getFarClip();
	
	// We can't use the camera's Orientation because of it's broken yaw. so we make a new
	// correct orientation to get our vectors
    Orientation o;
    o.yaw(yaw);
    o.pitch(pitch);
    o.roll(roll);

    direction   = o.getFront();
    up          = o.getUp();
    right       = o.getRight();

    /*
    printf("position.x=%f, position.y=%f, position.z=%f\n", position.x, position.y, position.z);
    printf("yaw=%f, pitch=%f, roll=%f\n", yaw,pitch,roll);
    printf("direction.x=%f, direction.y=%f, direction.z=%f\n", direction.x, direction.y, direction.z);
    printf("up.x=%f, up.y=%f, up.z=%f\n", up.x, up.y, up.z);
    printf("right.x=%f, right.y=%f, right.z=%f\n", right.x, right.y, right.z);
    printf("fov=%f\n", fov);
    printf("nearClip=%f\n", nearClip);
    printf("farClip=%f\n", farClip);
    */
    
    // Set the viewFrustum up with the correct position and orientation of the camera	
    viewFrustum.setPosition(position);
    viewFrustum.setOrientation(direction,up,right);
    
    // Also make sure it's got the correct lens details from the camera
    viewFrustum.setFieldOfView(fov);
    viewFrustum.setNearClip(nearClip);
    viewFrustum.setFarClip(farClip);

    // Ask the ViewFrustum class to calculate our corners
    viewFrustum.calculate();
}

/////////////////////////////////////////////////////////////////////////////////////
// renderViewFrustum()
//
// Description: this will render the view frustum bounds for EITHER the head
// 				or the "myCamera". 
//
// Frustum rendering mode. For debug purposes, we allow drawing the frustum in a couple of different ways.
// We can draw it with each of these parts:
//    * Origin Direction/Up/Right vectors - these will be drawn at the point of the camera
//    * Near plane - this plane is drawn very close to the origin point.
//    * Right/Left planes - these two planes are drawn between the near and far planes.
//    * Far plane - the plane is drawn in the distance.
// Modes - the following modes, will draw the following parts.
//    * All - draws all the parts listed above
//    * Planes - draws the planes but not the origin vectors
//    * Origin Vectors - draws the origin vectors ONLY
//    * Near Plane - draws only the near plane
//    * Far Plane - draws only the far plane
#define FRUSTUM_DRAW_MODE_ALL        0
#define FRUSTUM_DRAW_MODE_VECTORS    1
#define FRUSTUM_DRAW_MODE_PLANES     2
#define FRUSTUM_DRAW_MODE_NEAR_PLANE 3
#define FRUSTUM_DRAW_MODE_FAR_PLANE  4
#define FRUSTUM_DRAW_MODE_COUNT      5

int   frustumDrawingMode = FRUSTUM_DRAW_MODE_ALL; // the mode we're drawing the frustum in, see notes above

void renderViewFrustum(ViewFrustum& viewFrustum) {

    // Load it with the latest details!
    loadViewFrustum(viewFrustum);
	
    glm::vec3 position  = viewFrustum.getPosition();
    glm::vec3 direction = viewFrustum.getDirection();
    glm::vec3 up        = viewFrustum.getUp();
    glm::vec3 right     = viewFrustum.getRight();
	
    //  Get ready to draw some lines
    glDisable(GL_LIGHTING);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glLineWidth(1.0);
    glBegin(GL_LINES);

	if (::frustumDrawingMode == FRUSTUM_DRAW_MODE_ALL || ::frustumDrawingMode == FRUSTUM_DRAW_MODE_VECTORS) {
		// Calculate the origin direction vectors
		glm::vec3 lookingAt      = position + (direction * 0.2f);
		glm::vec3 lookingAtUp    = position + (up * 0.2f);
		glm::vec3 lookingAtRight = position + (right * 0.2f);

		// Looking At = white
		glColor3f(1,1,1);
		glVertex3f(position.x, position.y, position.z);
		glVertex3f(lookingAt.x, lookingAt.y, lookingAt.z);

		// Looking At Up = purple
		glColor3f(1,0,1);
		glVertex3f(position.x, position.y, position.z);
		glVertex3f(lookingAtUp.x, lookingAtUp.y, lookingAtUp.z);

		// Looking At Right = cyan
		glColor3f(0,1,1);
		glVertex3f(position.x, position.y, position.z);
		glVertex3f(lookingAtRight.x, lookingAtRight.y, lookingAtRight.z);
	}

	if (::frustumDrawingMode == FRUSTUM_DRAW_MODE_ALL || ::frustumDrawingMode == FRUSTUM_DRAW_MODE_PLANES
			|| ::frustumDrawingMode == FRUSTUM_DRAW_MODE_NEAR_PLANE) {
		// Drawing the bounds of the frustum
		// viewFrustum.getNear plane - bottom edge 
		glColor3f(1,0,0);
		glVertex3f(viewFrustum.getNearBottomLeft().x, viewFrustum.getNearBottomLeft().y, viewFrustum.getNearBottomLeft().z);
		glVertex3f(viewFrustum.getNearBottomRight().x, viewFrustum.getNearBottomRight().y, viewFrustum.getNearBottomRight().z);

		// viewFrustum.getNear plane - top edge
		glVertex3f(viewFrustum.getNearTopLeft().x, viewFrustum.getNearTopLeft().y, viewFrustum.getNearTopLeft().z);
		glVertex3f(viewFrustum.getNearTopRight().x, viewFrustum.getNearTopRight().y, viewFrustum.getNearTopRight().z);

		// viewFrustum.getNear plane - right edge
		glVertex3f(viewFrustum.getNearBottomRight().x, viewFrustum.getNearBottomRight().y, viewFrustum.getNearBottomRight().z);
		glVertex3f(viewFrustum.getNearTopRight().x, viewFrustum.getNearTopRight().y, viewFrustum.getNearTopRight().z);

		// viewFrustum.getNear plane - left edge
		glVertex3f(viewFrustum.getNearBottomLeft().x, viewFrustum.getNearBottomLeft().y, viewFrustum.getNearBottomLeft().z);
		glVertex3f(viewFrustum.getNearTopLeft().x, viewFrustum.getNearTopLeft().y, viewFrustum.getNearTopLeft().z);
	}

	if (::frustumDrawingMode == FRUSTUM_DRAW_MODE_ALL || ::frustumDrawingMode == FRUSTUM_DRAW_MODE_PLANES
			|| ::frustumDrawingMode == FRUSTUM_DRAW_MODE_FAR_PLANE) {
		// viewFrustum.getFar plane - bottom edge 
		glColor3f(0,1,0); // GREEN!!!
		glVertex3f(viewFrustum.getFarBottomLeft().x, viewFrustum.getFarBottomLeft().y, viewFrustum.getFarBottomLeft().z);
		glVertex3f(viewFrustum.getFarBottomRight().x, viewFrustum.getFarBottomRight().y, viewFrustum.getFarBottomRight().z);

		// viewFrustum.getFar plane - top edge
		glVertex3f(viewFrustum.getFarTopLeft().x, viewFrustum.getFarTopLeft().y, viewFrustum.getFarTopLeft().z);
		glVertex3f(viewFrustum.getFarTopRight().x, viewFrustum.getFarTopRight().y, viewFrustum.getFarTopRight().z);

		// viewFrustum.getFar plane - right edge
		glVertex3f(viewFrustum.getFarBottomRight().x, viewFrustum.getFarBottomRight().y, viewFrustum.getFarBottomRight().z);
		glVertex3f(viewFrustum.getFarTopRight().x, viewFrustum.getFarTopRight().y, viewFrustum.getFarTopRight().z);

		// viewFrustum.getFar plane - left edge
		glVertex3f(viewFrustum.getFarBottomLeft().x, viewFrustum.getFarBottomLeft().y, viewFrustum.getFarBottomLeft().z);
		glVertex3f(viewFrustum.getFarTopLeft().x, viewFrustum.getFarTopLeft().y, viewFrustum.getFarTopLeft().z);
	}

	if (::frustumDrawingMode == FRUSTUM_DRAW_MODE_ALL || ::frustumDrawingMode == FRUSTUM_DRAW_MODE_PLANES) {
		// RIGHT PLANE IS CYAN
		// right plane - bottom edge - viewFrustum.getNear to distant 
		glColor3f(0,1,1);
		glVertex3f(viewFrustum.getNearBottomRight().x, viewFrustum.getNearBottomRight().y, viewFrustum.getNearBottomRight().z);
		glVertex3f(viewFrustum.getFarBottomRight().x, viewFrustum.getFarBottomRight().y, viewFrustum.getFarBottomRight().z);

		// right plane - top edge - viewFrustum.getNear to distant
		glVertex3f(viewFrustum.getNearTopRight().x, viewFrustum.getNearTopRight().y, viewFrustum.getNearTopRight().z);
		glVertex3f(viewFrustum.getFarTopRight().x, viewFrustum.getFarTopRight().y, viewFrustum.getFarTopRight().z);

		// LEFT PLANE IS BLUE
		// left plane - bottom edge - viewFrustum.getNear to distant
		glColor3f(0,0,1);
		glVertex3f(viewFrustum.getNearBottomLeft().x, viewFrustum.getNearBottomLeft().y, viewFrustum.getNearBottomLeft().z);
		glVertex3f(viewFrustum.getFarBottomLeft().x, viewFrustum.getFarBottomLeft().y, viewFrustum.getFarBottomLeft().z);

		// left plane - top edge - viewFrustum.getNear to distant
		glVertex3f(viewFrustum.getNearTopLeft().x, viewFrustum.getNearTopLeft().y, viewFrustum.getNearTopLeft().z);
		glVertex3f(viewFrustum.getFarTopLeft().x, viewFrustum.getFarTopLeft().y, viewFrustum.getFarTopLeft().z);
	}

    glEnd();
    glEnable(GL_LIGHTING);
}


void display(void)
{
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
		myCamera.setTargetPosition( myAvatar.getBodyPosition() );

		if ( displayHead ) {
			//-----------------------------------------------
			// set the camera to looking at my own face
			//-----------------------------------------------
			myCamera.setTargetPosition	( myAvatar.getBodyPosition() ); // XXXBHG - Shouldn't we use Head position here?
			myCamera.setYaw				( - myAvatar.getBodyYaw() );
			myCamera.setPitch			( 0.0  );
			myCamera.setRoll			( 0.0  );
			myCamera.setUp				( 0.6 );	
			myCamera.setDistance		( 0.3  );
			myCamera.setTightness		( 100.0f );
			myCamera.update				( 1.f/FPS );
		} else {
			//----------------------------------------------------
			// set the camera to third-person view behind my av
			//----------------------------------------------------		
			myCamera.setTargetPosition	( myAvatar.getBodyPosition() );
			myCamera.setYaw				( 180.0 - myAvatar.getBodyYaw() );
			myCamera.setPitch			(   0.0 );  // temporarily, this must be 0.0 or else bad juju
			myCamera.setRoll			(   0.0 );
			myCamera.setUp				(   0.45);
			myCamera.setDistance		(   1.0 );
			myCamera.setTightness		( 10.0f );
			myCamera.update				( 1.f/FPS );
		}
		
		// Note: whichCamera is used to pick between the normal camera myCamera for our 
		// main camera, vs, an alternate camera. The alternate camera we support right now
		// is the viewFrustumOffsetCamera. But theoretically, we could use this same mechanism
		// to add other cameras.
		//
		// Why have two cameras? Well, one reason is that because in the case of the renderViewFrustum()
		// code, we want to keep the state of "myCamera" intact, so we can render what the view frustum of
		// myCamera is. But we also want to do meaningful camera transforms on OpenGL for the offset camera
		Camera whichCamera = myCamera;
		Camera viewFrustumOffsetCamera = myCamera;

		if (::viewFrustumFromOffset && ::frustumOn) {

			// set the camera to third-person view but offset so we can see the frustum
			viewFrustumOffsetCamera.setYaw		(  180.0 - myAvatar.getBodyYaw() + ::viewFrustumOffsetYaw );
			viewFrustumOffsetCamera.setPitch	(  ::viewFrustumOffsetPitch    );
			viewFrustumOffsetCamera.setRoll     (  ::viewFrustumOffsetRoll     ); 
			viewFrustumOffsetCamera.setUp		(  ::viewFrustumOffsetUp       );
			viewFrustumOffsetCamera.setDistance (  ::viewFrustumOffsetDistance );
			viewFrustumOffsetCamera.update(1.f/FPS);
			whichCamera = viewFrustumOffsetCamera;
		}		

		// transform view according to whichCamera
		// could be myCamera (if in normal mode)
		// or could be viewFrustumOffsetCamera if in offset mode
		// I changed the ordering here - roll is FIRST (JJV) 
        glRotatef	( whichCamera.getRoll(),	0, 0, 1 );
        glRotatef	( whichCamera.getPitch(),	1, 0, 0 );
        glRotatef	( whichCamera.getYaw(),	    0, 1, 0 );
        glTranslatef( -whichCamera.getPosition().x, -whichCamera.getPosition().y, -whichCamera.getPosition().z );

        if (::starsOn) {
            // should be the first rendering pass - w/o depth buffer / lighting

            glm::mat4 view;
            glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(view)); 
        	stars.render(angleConvert<Degrees,Radians>(whichCamera.getFieldOfView()),  aspectRatio, view);
        }

        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        
        
        /*
        // Test - Draw a blue sphere around a body part of mine!
        
        glPushMatrix();
        glColor4f(0,0,1, 0.7);
        glTranslatef(myAvatar.getBonePosition(AVATAR_BONE_RIGHT_HAND).x,
                      myAvatar.getBonePosition(AVATAR_BONE_RIGHT_HAND).y,
                      myAvatar.getBonePosition(AVATAR_BONE_RIGHT_HAND).z);
        glutSolidSphere(0.03, 10, 10);
        glPopMatrix();
        */
		
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
                Head *avatar = (Head *)agent->getLinkedData();
                //glPushMatrix();
                
//printf( "rendering remote avatar\n" );                
                
                avatar->render(0);
                //glPopMatrix();
            }
        }
    
        if ( !displayHead ) balls.render();
    
        //  Render the world box
        if (!displayHead && statsOn) render_world_box();
        
        // brad's frustum for debugging
        if (::frustumOn) renderViewFrustum(::viewFrustum);
    
        //Render my own avatar
		myAvatar.render(true);	
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
        logger.render(WIDTH, HEIGHT);
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
        float startupTime = (usecTimestampNow() - usecTimestamp(&applicationStartupTime))/1000000.0;
        justStarted = false;
        char title[30];
        snprintf(title, 30, "Interface: %4.2f seconds", startupTime);
        glutSetWindowTitle(title);
    }
}

// int version of setValue()
int setValue(int state, int *value) {
    if (state == MENU_ROW_PICKED) {
        *value = !(*value);
    } else if (state == MENU_ROW_GET_VALUE) {
        return *value;
    } else {
        *value = state;
    }
    return *value;
}

// bool version of setValue()
int setValue(int state, bool *value) {
    if (state == MENU_ROW_PICKED) {
        *value = !(*value);
    } else if (state == MENU_ROW_GET_VALUE) {
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

int setVoxels(int state) {
    return setValue(state, &::showingVoxels);
}

int setStars(int state) {
    return setValue(state, &::starsOn);
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

int setDisplayFrustum(int state) {
    return setValue(state, &::frustumOn);
}

int setFrustumOffset(int state) {
    int value = setValue(state, &::viewFrustumFromOffset);

    // reshape so that OpenGL will get the right lens details for the camera of choice    
    if (state == MENU_ROW_PICKED) {
        reshape(::WIDTH,::HEIGHT);
    }
    
    return value;
}

int setFrustumOrigin(int state) {
    return setValue(state, &::cameraFrustum);
}

int quitApp(int state) {
    if (state == MENU_ROW_PICKED) {
        ::terminate();
    }
    return 2; // non state so menu class doesn't add "state"
}

int setFrustumRenderMode(int state) {
    if (state == MENU_ROW_PICKED) {
		::frustumDrawingMode = (::frustumDrawingMode+1)%FRUSTUM_DRAW_MODE_COUNT;
    }
    return ::frustumDrawingMode;
}

int doRandomizeVoxelColors(int state) {
    if (state == MENU_ROW_PICKED) {
        ::voxels.randomizeVoxelColors();
    }
    return state;
}


int doFalseRandomizeVoxelColors(int state) {
    if (state == MENU_ROW_PICKED) {
        ::voxels.falseColorizeRandom();
    }
    return state;
}

int doTrueVoxelColors(int state) {
    if (state == MENU_ROW_PICKED) {
        ::voxels.trueColorize();
    }
    return state;
}

int doFalseColorizeByDistance(int state) {
    if (state == MENU_ROW_PICKED) {
        loadViewFrustum(::viewFrustum);
        voxels.falseColorizeDistanceFromView(&::viewFrustum);
    }
    return state;
}

int doFalseColorizeInView(int state) {
    if (state == MENU_ROW_PICKED) {
        loadViewFrustum(::viewFrustum);
        // we probably want to make sure the viewFrustum is initialized first
        voxels.falseColorizeInView(&::viewFrustum);
    }
    return state;
}


const char* modeAll     = " - All "; 
const char* modeVectors = " - Vectors "; 
const char* modePlanes  = " - Planes "; 
const char* modeNear    = " - Near "; 
const char* modeFar     = " - Far "; 

const char* getFrustumRenderModeName(int state) {
	const char * mode;
	switch (state) {
		case FRUSTUM_DRAW_MODE_ALL: 
			mode = modeAll;
			break;
		case FRUSTUM_DRAW_MODE_VECTORS: 
			mode = modeVectors;
			break;
		case FRUSTUM_DRAW_MODE_PLANES:
			mode = modePlanes;
			break;
		case FRUSTUM_DRAW_MODE_NEAR_PLANE: 
			mode = modeNear;
			break;
		case FRUSTUM_DRAW_MODE_FAR_PLANE: 
			mode = modeFar;
			break;
	}
	return mode;
}

void initMenu() {
    MenuColumn *menuColumnOptions, *menuColumnTools, *menuColumnDebug, *menuColumnFrustum;
    //  Options
    menuColumnOptions = menu.addColumn("Options");
    menuColumnOptions->addRow("(H)ead", setHead); 
    menuColumnOptions->addRow("Field (f)", setField); 
    menuColumnOptions->addRow("(N)oise", setNoise); 
    menuColumnOptions->addRow("Mirror", setMirror);
    menuColumnOptions->addRow("(V)oxels", setVoxels);
    menuColumnOptions->addRow("Stars (*)", setStars);
    menuColumnOptions->addRow("(Q)uit", quitApp);

    //  Tools
    menuColumnTools = menu.addColumn("Tools");
    menuColumnTools->addRow("Stats (/)", setStats); 
    menuColumnTools->addRow("(M)enu", setMenu);

    // Frustum Options
    menuColumnFrustum = menu.addColumn("Frustum");
    menuColumnFrustum->addRow("Display (F)rustum", setDisplayFrustum); 
    menuColumnFrustum->addRow("Use (O)ffset Camera", setFrustumOffset); 
    menuColumnFrustum->addRow("Switch (C)amera", setFrustumOrigin); 
    menuColumnFrustum->addRow("(R)ender Mode", setFrustumRenderMode, getFrustumRenderModeName); 

    // Debug
    menuColumnDebug = menu.addColumn("Debug");
    menuColumnDebug->addRow("Randomize Voxel TRUE Colors", doRandomizeVoxelColors);
    menuColumnDebug->addRow("FALSE Color Voxels Randomly", doFalseRandomizeVoxelColors);
    menuColumnDebug->addRow("FALSE Color Voxels by Distance", doFalseColorizeByDistance);
    menuColumnDebug->addRow("FALSE Color Voxel Out of View", doFalseColorizeInView);
    menuColumnDebug->addRow("Show TRUE Colors", doTrueVoxelColors);
}

void testPointToVoxel()
{
	float y=0;
	float z=0;
	float s=0.1;
	for (float x=0; x<=1; x+= 0.05)
	{
		printLog(" x=%f");

		unsigned char red   = 200; //randomColorValue(65);
		unsigned char green = 200; //randomColorValue(65);
		unsigned char blue  = 200; //randomColorValue(65);
	
		unsigned char* voxelCode = pointToVoxel(x, y, z, s,red,green,blue);
		printVoxelCode(voxelCode);
		delete voxelCode;
		printLog("\n");
	}
}

void sendVoxelServerEraseAll() {
	char message[100];
    sprintf(message,"%c%s",'Z',"erase all");
	int messageSize = strlen(message) + 1;
	AgentList::getInstance()->broadcastToAgents((unsigned char*) message, messageSize, &AGENT_TYPE_VOXEL, 1);
}

void sendVoxelServerAddScene() {
	char message[100];
    sprintf(message,"%c%s",'Z',"add scene");
	int messageSize = strlen(message) + 1;
	AgentList::getInstance()->broadcastToAgents((unsigned char*)message, messageSize, &AGENT_TYPE_VOXEL, 1);
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
	glm::vec3 avatarPos = myAvatar.getBodyPosition();

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

	printLog("random sphere\n");
	printLog("radius=%f\n",r);
	printLog("xc=%f\n",xc);
	printLog("yc=%f\n",yc);
	printLog("zc=%f\n",zc);

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
 	if (k == 'q' || k == 'Q')  ::terminate();
    if (k == '/')  statsOn = !statsOn;		// toggle stats
    if (k == '*')  ::starsOn = !::starsOn;		// toggle stars
    if (k == 'V' || k == 'v')  ::showingVoxels = !::showingVoxels;		// toggle voxels
    if (k == 'F')  ::frustumOn = !::frustumOn;		// toggle view frustum debugging
    if (k == 'C')  ::cameraFrustum = !::cameraFrustum;	// toggle which frustum to look at
    if (k == 'O' || k == 'G') setFrustumOffset(MENU_ROW_PICKED); // toggle view frustum offset debugging
    
	if (k == '[') ::viewFrustumOffsetYaw       -= 0.5;
	if (k == ']') ::viewFrustumOffsetYaw       += 0.5;
	if (k == '{') ::viewFrustumOffsetPitch     -= 0.5;
	if (k == '}') ::viewFrustumOffsetPitch     += 0.5;
	if (k == '(') ::viewFrustumOffsetRoll      -= 0.5;
	if (k == ')') ::viewFrustumOffsetRoll      += 0.5;
	if (k == '<') ::viewFrustumOffsetDistance  -= 0.5;
	if (k == '>') ::viewFrustumOffsetDistance  += 0.5;
	if (k == ',') ::viewFrustumOffsetUp        -= 0.05;
	if (k == '.') ::viewFrustumOffsetUp        += 0.05;

//	if (k == '|') ViewFrustum::fovAngleAdust   -= 0.05;
//	if (k == '\\') ViewFrustum::fovAngleAdust  += 0.05;

	if (k == 'R') setFrustumRenderMode(MENU_ROW_PICKED);

    if (k == '&') {
    	::paintOn = !::paintOn;		// toggle paint
    	::setupPaintingVoxel();		// also randomizes colors
    }
    if (k == '^')  ::shiftPaintingColor();		// shifts randomize color between R,G,B dominant
    if (k == '-')  ::sendVoxelServerEraseAll();	// sends erase all command to voxel server
    if (k == '%')  ::sendVoxelServerAddScene();	// sends add scene command to voxel server
	if (k == 'n' || k == 'N') 
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
    
    if (k == 'm' || k == 'M') setMenu(MENU_ROW_PICKED);
    
    if (k == 'f') displayField = !displayField;
    if (k == 'l') displayLevels = !displayLevels;
    if (k == 'e') myAvatar.setDriveKeys(UP, 1);
    if (k == 'c') myAvatar.setDriveKeys(DOWN, 1);
    if (k == 'w') myAvatar.setDriveKeys(FWD, 1);
    if (k == 's') myAvatar.setDriveKeys(BACK, 1);
    if (k == ' ') reset_sensors();
    if (k == 't') renderPitchRate -= KEYBOARD_PITCH_RATE;
    if (k == 'g') renderPitchRate += KEYBOARD_PITCH_RATE;
    if (k == 'a') myAvatar.setDriveKeys(ROT_LEFT, 1); 
    if (k == 'd') myAvatar.setDriveKeys(ROT_RIGHT, 1);
}

//  Receive packets from other agents/servers and decide what to do with them!
void *networkReceive(void *args)
{    
    sockaddr senderAddress;
    ssize_t bytesReceived;
    unsigned char *incomingPacket = new unsigned char[MAX_PACKET_SIZE];

    while (!stopNetworkReceiveThread) {
        if (AgentList::getInstance()->getAgentSocket().receive(&senderAddress, incomingPacket, &bytesReceived)) {
            packetCount++;
            bytesCount += bytesReceived;
            
            switch (incomingPacket[0]) {
                case PACKET_HEADER_TRANSMITTER_DATA:
                    myAvatar.processTransmitterData(incomingPacket, bytesReceived);
                    break;
                case PACKET_HEADER_VOXEL_DATA:
                case PACKET_HEADER_Z_COMMAND:
                case PACKET_HEADER_ERASE_VOXEL:
                    voxels.parseData(incomingPacket, bytesReceived);
                    break;
                case PACKET_HEADER_BULK_AVATAR_DATA:
                    AgentList::getInstance()->processBulkAgentData(&senderAddress,
                                                                   incomingPacket,
                                                                   bytesReceived,
                                                                   BYTES_PER_AVATAR);
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

void idle(void) {
    timeval check;
    gettimeofday(&check, NULL);
    
    //  Only run simulation code if more than IDLE_SIMULATE_MSECS have passed since last time
    
    if (diffclock(&lastTimeIdle, &check) > IDLE_SIMULATE_MSECS) {
		
        float deltaTime = 1.f/FPS;

        // update behaviors for avatar hand movement 
        updateHandController( mouseX, mouseY );
        
		// tell my avatar if the mouse is being pressed...
		if ( mousePressed == 1 ) {
			myAvatar.setMousePressed( true );
		}
		else {
			myAvatar.setMousePressed( false );
		}
        
        // walking triggers the handController to stop
        if ( myAvatar.getMode() == AVATAR_MODE_WALKING ) {
            handController.enabled = false;
		}
        
        //
        //  Sample hardware, update view frustum if needed, Lsend avatar data to mixer/agents
        //
        updateAvatar( 1.f/FPS );
		
        //loop through all the other avatars and simulate them.
        AgentList * agentList = AgentList::getInstance();
        for(std::vector<Agent>::iterator agent = agentList->getAgents().begin(); agent != agentList->getAgents().end(); agent++) 
		{
            if (agent->getLinkedData() != NULL) 
			{
                Head *avatar = (Head *)agent->getLinkedData();
                
//printf( "simulating remote avatar\n" );                
                
                avatar->simulate(deltaTime);
            }
        }
        
        //updateAvatarHand(1.f/FPS);
    
        field.simulate   (deltaTime);
        myAvatar.simulate(deltaTime);
        balls.simulate   (deltaTime);
        cloud.simulate   (deltaTime);

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
    aspectRatio = ((float)width/(float)height); // based on screen resize

    float fov;
    float nearClip;
    float farClip;

    // get the lens details from the current camera
    if (::viewFrustumFromOffset) {
        fov       = ::viewFrustumOffsetCamera.getFieldOfView();
        nearClip  = ::viewFrustumOffsetCamera.getNearClip();
        farClip   = ::viewFrustumOffsetCamera.getFarClip();
    } else {
        fov       = ::myCamera.getFieldOfView();
        nearClip  = ::myCamera.getNearClip();
        farClip   = ::myCamera.getFarClip();
    }

    //printLog("reshape() width=%d, height=%d, aspectRatio=%f fov=%f near=%f far=%f \n",
    //    width,height,aspectRatio,fov,nearClip,farClip);
    
    // Tell our viewFrustum about this change
    ::viewFrustum.setAspectRatio(aspectRatio);

    
    glViewport(0, 0, width, height); // shouldn't this account for the menu???

    glMatrixMode(GL_PROJECTION); //hello

    glLoadIdentity();
    
    // XXXBHG - If we're in view frustum mode, then we need to do this little bit of hackery so that
    // OpenGL won't clip our frustum rendering lines. This is a debug hack for sure! Basically, this makes
    // the near clip a little bit closer (therefor you see more) and the far clip a little bit farther (also,
    // to see more.)
    if (::frustumOn) {
        nearClip -= 0.01f;
        farClip  += 0.01f;
    }
    
    // On window reshape, we need to tell OpenGL about our new setting
    gluPerspective(fov,aspectRatio,nearClip,farClip);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

     
}

void mouseFunc( int button, int state, int x, int y ) 
{
    if( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
        if (!menu.mouseClick(x, y)) {
            mouseX = x;
            mouseY = y;
            mousePressed = 1;
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
        newAgent->setLinkedData(new Head(false));
    }
}

#ifndef _WIN32
void audioMixerUpdate(in_addr_t newMixerAddress, in_port_t newMixerPort) {
    audio.updateMixerParams(newMixerAddress, newMixerPort);
}
#endif

int main(int argc, const char * argv[])
{
    shared_lib::printLog = & ::printLog;
    voxels_lib::printLog = & ::printLog;
    avatars_lib::printLog = & ::printLog;

    // Quick test of the Orientation class on startup!
    if (cmdOptionExists(argc, argv, "--testOrientation")) {
        testOrientationClass();
        return EXIT_SUCCESS;
    }

    AgentList::createInstance(AGENT_TYPE_INTERFACE);
    
    gettimeofday(&applicationStartupTime, NULL);
    const char* domainIP = getCmdOption(argc, argv, "--domain");
    if (domainIP) {
		strcpy(DOMAIN_IP,domainIP);
	}

    // Handle Local Domain testing with the --local command line
    if (cmdOptionExists(argc, argv, "--local")) {
    	printLog("Local Domain MODE!\n");
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
    WIDTH = glutGet(GLUT_SCREEN_WIDTH);
    HEIGHT = glutGet(GLUT_SCREEN_HEIGHT);
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Interface");
    
    #ifdef _WIN32
    glewInit();
    #endif
    
    // Before we render anything, let's set up our viewFrustumOffsetCamera with a sufficiently large
    // field of view and near and far clip to make it interesting.
    //viewFrustumOffsetCamera.setFieldOfView(90.0);
    viewFrustumOffsetCamera.setNearClip(0.1);
    viewFrustumOffsetCamera.setFarClip(500.0);

    printLog( "Created Display Window.\n" );
        
    initMenu();
    initDisplay();
    printLog( "Initialized Display.\n" );

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
    printLog( "Init() complete.\n" );

	// Check to see if the user passed in a command line option for randomizing colors
	if (cmdOptionExists(argc, argv, "--NoColorRandomizer")) {
		wantColorRandomizer = false;
	}
	
	// Check to see if the user passed in a command line option for loading a local
	// Voxel File. If so, load it now.
    const char* voxelsFilename = getCmdOption(argc, argv, "-i");
    if (voxelsFilename) {
	    voxels.loadVoxelsFile(voxelsFilename,wantColorRandomizer);
        printLog("Local Voxel File loaded.\n");
	}
    
    // create thread for receipt of data via UDP
    pthread_create(&networkReceiveThread, NULL, networkReceive, NULL);
    printLog("Network receive thread created.\n");
    
    glutTimerFunc(1000, Timer, 0);
    glutMainLoop();

    printLog("Normal exit.\n");
    ::terminate();
    return EXIT_SUCCESS;
}   
