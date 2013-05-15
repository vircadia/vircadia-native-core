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

#include "InterfaceConfig.h"

#include "Log.h"
#include "shared_Log.h"
#include "voxels_Log.h"
#include "avatars_Log.h"

#include "world.h"
#include "Util.h"
#ifndef _WIN32
#include "Audio.h"
#endif

#include "AngleUtil.h"
#include "Stars.h"

#include "ui/ChatEntry.h"
#include "ui/MenuRow.h"
#include "ui/MenuColumn.h"
#include "ui/Menu.h"
#include "ui/TextRenderer.h"
#include "renderer/ProgramObject.h"
#include "renderer/ShaderObject.h"

#include "Application.h"
#include "Camera.h"
#include "Avatar.h"
#include <AgentList.h>
#include <AgentTypes.h>
#include "VoxelSystem.h"
#include "Environment.h"
#include "Oscilloscope.h"
#include "UDPSocket.h"
#include "SerialInterface.h"
#include <PacketHeaders.h>
#include <AvatarData.h>
#include <PerfStat.h>
#include <SimpleMovingAverage.h>

#include "ViewFrustum.h"
#include "HandControl.h"
#include "AvatarRenderer.h"
#include "OculusManager.h"

using namespace std;

void reshape(int width, int height); // will be defined below
void loadViewFrustum(ViewFrustum& viewFrustum);  // will be defined below

glm::vec3 getGravity(glm::vec3 pos); //get the local gravity vector at this location in the universe

bool enableNetworkThread = true;
pthread_t networkReceiveThread;
bool stopNetworkReceiveThread = false;
 
unsigned char incomingPacket[MAX_PACKET_SIZE];
int packetCount = 0;
int packetsPerSecond = 0; 
int bytesPerSecond = 0;
int bytesCount = 0;

int screenWidth = 1200; //  Window size
int screenHeight = 800;

int fullscreen = 0;
float aspectRatio = 1.0f;

//  PER:  Jeffrey - please move these our of main.cpp - also these not constants!
float mouseViewShiftYaw   = 0.0f;
float mouseViewShiftPitch = 0.0f;
bool  USING_MOUSE_VIEW_SHIFT = false;
float MOUSE_VIEW_SHIFT_RATE         = 40.0f;
float MOUSE_VIEW_SHIFT_YAW_MARGIN   = (float)(::screenWidth  * 0.2f);
float MOUSE_VIEW_SHIFT_PITCH_MARGIN = (float)(::screenHeight * 0.2f);
float MOUSE_VIEW_SHIFT_YAW_LIMIT    = 45.0;
float MOUSE_VIEW_SHIFT_PITCH_LIMIT  = 30.0;

bool wantColorRandomizer = true;    // for addSphere and load file

Oscilloscope audioScope(256,200,true);

ViewFrustum viewFrustum;            // current state of view frustum, perspective, orientation, etc.

Avatar myAvatar(true);            // The rendered avatar of oneself
Camera myCamera;                  // My view onto the world 
Camera viewFrustumOffsetCamera;   // The camera we use to sometimes show the view frustum from an offset mode

AvatarRenderer avatarRenderer;

//  Starfield information
char starFile[] = "https://s3-us-west-1.amazonaws.com/highfidelity/stars.txt";
char starCacheFile[] = "cachedStars.txt";
Stars stars;

glm::vec3 box(WORLD_SIZE,WORLD_SIZE,WORLD_SIZE);

VoxelSystem voxels;

Environment environment;


#ifndef _WIN32
Audio audio(&audioScope, &myAvatar);
#endif

#define IDLE_SIMULATE_MSECS 16           //  How often should call simulate and other stuff 
                                         //  in the idle loop?  (60 FPS is default)


glm::vec3 start_location(6.1f, 0, 1.4f);   //  Where one's own agent begins in the world
                                           // (will be overwritten if avatar data file is found)

bool renderWarningsOn = false;      //  Whether to show render pipeline warnings
bool renderStatsOn = false;         //  Whether to show onscreen text overlay with stats
bool renderVoxels = true;           //  Whether to render voxels
bool renderStarsOn = true;          //  Whether to display the stars
bool renderAtmosphereOn = true;     //  Whether to display the atmosphere
bool renderAvatarsOn = true;        //  Whether to render avatars
bool renderFirstPersonOn = false;   //  Whether to render in first person 
bool paintOn = false;               //  Whether to paint voxels as you fly around
VoxelDetail paintingVoxel;          //    The voxel we're painting if we're painting
unsigned char dominantColor = 0;    //    The dominant color of the voxel we're painting
bool perfStatsOn = false;           //  Do we want to display perfStats?
bool wantMonochrome = false;        // ask server to send us in monochrome
bool wantResIn = false;             // ask server to res in

bool logOn = true;                  //  Whether to show on-screen log

bool wantToKillLocalVoxels = false;

int noiseOn = 0;                    //  Whether to add random noise 
float noise = 1.0;                  //  Overall magnitude scaling for random noise levels 

bool gyroLook = true;               //  Whether to allow the gyro data from head to move your view

int displayLevels = 0;
bool lookingInMirror = 0;           //  Are we currently rendering one's own head as if in mirror?

int displayHeadMouse = 1;         //  Display sample mouse pointer controlled by head movement
int headMouseX, headMouseY; 

HandControl handControl;

int mouseX = 0;
int mouseY = 0;

//  Mouse location at start of last down click
int mousePressed = 0; //  true if mouse has been pressed (clear when finished)

// The current mode for mouse interaction
enum MouseMode { NO_EDIT_MODE, ADD_VOXEL_MODE, DELETE_VOXEL_MODE, COLOR_VOXEL_MODE };
MouseMode mouseMode = NO_EDIT_MODE;
VoxelDetail mouseVoxel; // details of the voxel under the mouse cursor
float mouseVoxelScale = 1.0f / 1024.0f; // the scale for adding/removing voxels

Menu menu;       // main menu
int menuOn = 1;  //  Whether to show onscreen menu

ChatEntry chatEntry;       // chat entry field
bool chatEntryOn = false;  //  Whether to show the chat entry

bool oculusOn = false;              //  Whether to configure the display for the Oculus Rift
GLuint oculusTextureID = 0;         //  The texture to which we render for Oculus distortion
ProgramObject* oculusProgram = 0;   //  The GLSL program containing the distortion shader
float oculusDistortionScale = 1.25; //  Controls the Oculus field of viewa

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
void Timer(int extra) {
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

void displayStats(void) {
    int statsVerticalOffset = 50;
    if (::menuOn == 0) {
        statsVerticalOffset = 8;
    }

    char stats[200];
    sprintf(stats, "%3.0f FPS, %d Pkts/sec, %3.2f Mbps", 
            FPS, packetsPerSecond,  (float)bytesPerSecond * 8.f / 1000000.f);
    drawtext(10, statsVerticalOffset + 15, 0.10f, 0, 1.0, 0, stats);
    
    std::stringstream voxelStats;
    voxelStats.precision(4);
    voxelStats << "Voxels Rendered: " << voxels.getVoxelsRendered() / 1000.f << "K Updated: " << voxels.getVoxelsUpdated()/1000.f << "K";
    drawtext(10, statsVerticalOffset + 230, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
    voxelStats.str("");
    voxelStats << "Voxels Created: " << voxels.getVoxelsCreated() / 1000.f << "K (" << voxels.getVoxelsCreatedPerSecondAverage() / 1000.f
    << "Kps) ";
    drawtext(10, statsVerticalOffset + 250, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
    voxelStats.str("");
    voxelStats << "Voxels Colored: " << voxels.getVoxelsColored() / 1000.f << "K (" << voxels.getVoxelsColoredPerSecondAverage() / 1000.f
    << "Kps) ";
    drawtext(10, statsVerticalOffset + 270, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
    voxelStats.str("");
    voxelStats << "Voxel Bits Read: " << voxels.getVoxelsBytesRead() * 8.f / 1000000.f 
    << "M (" << voxels.getVoxelsBytesReadPerSecondAverage() * 8.f / 1000000.f << " Mbps)";
    drawtext(10, statsVerticalOffset + 290,0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());

    voxelStats.str("");
    float voxelsBytesPerColored = voxels.getVoxelsColored()
        ? ((float) voxels.getVoxelsBytesRead() / voxels.getVoxelsColored())
        : 0;
    
    voxelStats << "Voxels Bits per Colored: " << voxelsBytesPerColored * 8;
    drawtext(10, statsVerticalOffset + 310, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
    Agent *avatarMixer = AgentList::getInstance()->soloAgentOfType(AGENT_TYPE_AVATAR_MIXER);
    char avatarMixerStats[200];
    
    if (avatarMixer) {
        sprintf(avatarMixerStats, "Avatar Mixer: %.f kbps, %.f pps",
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

void initDisplay(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel (GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    
    if (fullscreen) glutFullScreen();
}

void init(void) {
    voxels.init();
    voxels.setViewerAvatar(&myAvatar);
    voxels.setCamera(&myCamera);
    
    environment.init();
    
    handControl.setScreenDimensions(::screenWidth, ::screenHeight);

    headMouseX = ::screenWidth /2;
    headMouseY = ::screenHeight/2; 

    stars.readInput(starFile, starCacheFile, 0);
  
    if (noiseOn) {   
        myAvatar.setNoise(noise);
    }
    
    myAvatar.setPosition(start_location);
    Camera::CameraFollowingAttributes a;            
    a.upShift   = -0.2f;
    a.distance  = 1.5f;
    a.tightness = 8.0f;
    myCamera.setMode(CAMERA_MODE_THIRD_PERSON, a);
    myCamera.initialize();
    myAvatar.setDisplayingHead(true);  
    
    OculusManager::connect();
    
    gettimeofday(&timerStart, NULL);
    gettimeofday(&lastTimeIdle, NULL);
}

void terminate () {
    // Close serial port
    // close(serial_fd);
    
    myAvatar.writeAvatarDataToFile();    

    #ifndef _WIN32
    audio.terminate();
    #endif
    
    if (enableNetworkThread) {
        stopNetworkReceiveThread = true;
        pthread_join(networkReceiveThread, NULL); 
    }
    
    exit(EXIT_SUCCESS);
}

void resetSensors() {
    
    myAvatar.setPosition(start_location);
    headMouseX = ::screenWidth / 2;
    headMouseY = ::screenHeight / 2;
    if (serialPort.active) {
        serialPort.resetAverages();
    }
    myAvatar.reset();
}

void sendVoxelEditMessage(PACKET_HEADER header, VoxelDetail& detail) {
    unsigned char* bufferOut;
    int sizeOut;

    if (createVoxelEditMessage(header, 0, 1, &detail, bufferOut, sizeOut)){
        AgentList::getInstance()->broadcastToAgents(bufferOut, sizeOut, &AGENT_TYPE_VOXEL, 1);
        delete bufferOut;
    }
}

//  Using gyro data, update both view frustum and avatar head position
void updateAvatar(float deltaTime) {
    
    // Update my avatar's head position from gyros
    myAvatar.updateHeadFromGyros(deltaTime, &serialPort, &gravity);

    //  Grab latest readings from the gyros
    float measuredPitchRate = serialPort.getLastPitchRate();
    float measuredYawRate = serialPort.getLastYawRate();
    
    //  Update gyro-based mouse (X,Y on screen)
    const float MIN_MOUSE_RATE = 1.0;
    const float HORIZONTAL_PIXELS_PER_DEGREE = 2880.f / 45.f;
    const float VERTICAL_PIXELS_PER_DEGREE = 1800.f / 30.f;
    if (powf(measuredYawRate * measuredYawRate +
             measuredPitchRate * measuredPitchRate, 0.5) > MIN_MOUSE_RATE)
    {
        headMouseX += measuredYawRate * HORIZONTAL_PIXELS_PER_DEGREE * deltaTime;
        headMouseY -= measuredPitchRate * VERTICAL_PIXELS_PER_DEGREE * deltaTime;
    }
    headMouseX = max(headMouseX, 0);
    headMouseX = min(headMouseX, ::screenWidth);
    headMouseY = max(headMouseY, 0);
    headMouseY = min(headMouseY, ::screenHeight);
    
    //  Update head and body pitch and yaw based on measured gyro rates
    if (::gyroLook) {
        // Render Yaw
        float renderYawSpring = fabs(headMouseX - ::screenWidth / 2.f) / (::screenWidth / 2.f);
        const float RENDER_YAW_MULTIPLY = 4.f;
        myAvatar.setRenderYaw((1.f - renderYawSpring * deltaTime) * myAvatar.getRenderYaw() +
                              renderYawSpring * deltaTime * -myAvatar.getHeadYaw() * RENDER_YAW_MULTIPLY);
        // Render Pitch
        float renderPitchSpring = fabs(headMouseY - ::screenHeight / 2.f) / (::screenHeight / 2.f);
        const float RENDER_PITCH_MULTIPLY = 4.f;
        myAvatar.setRenderPitch((1.f - renderPitchSpring * deltaTime) * myAvatar.getRenderPitch() +
                                renderPitchSpring * deltaTime * -myAvatar.getHeadPitch() * RENDER_PITCH_MULTIPLY);
    }
    
    
    if (USING_MOUSE_VIEW_SHIFT)
    {
        //make it so that when your mouse hits the edge of the screen, the camera shifts
        float rightBoundary  = (float)::screenWidth  - MOUSE_VIEW_SHIFT_YAW_MARGIN;
        float bottomBoundary = (float)::screenHeight - MOUSE_VIEW_SHIFT_PITCH_MARGIN;
        
        if (mouseX > rightBoundary) {
            float f = (mouseX - rightBoundary) / ( (float)::screenWidth - rightBoundary);
            mouseViewShiftYaw += MOUSE_VIEW_SHIFT_RATE * f * deltaTime;
            if (mouseViewShiftYaw > MOUSE_VIEW_SHIFT_YAW_LIMIT) { mouseViewShiftYaw = MOUSE_VIEW_SHIFT_YAW_LIMIT; }
        } else if (mouseX < MOUSE_VIEW_SHIFT_YAW_MARGIN) {
            float f = 1.0 - (mouseX / MOUSE_VIEW_SHIFT_YAW_MARGIN);
            mouseViewShiftYaw -= MOUSE_VIEW_SHIFT_RATE * f * deltaTime;
            if (mouseViewShiftYaw < -MOUSE_VIEW_SHIFT_YAW_LIMIT) { mouseViewShiftYaw = -MOUSE_VIEW_SHIFT_YAW_LIMIT; }
        }
        if (mouseY < MOUSE_VIEW_SHIFT_PITCH_MARGIN) {
            float f = 1.0 - (mouseY / MOUSE_VIEW_SHIFT_PITCH_MARGIN);
            mouseViewShiftPitch += MOUSE_VIEW_SHIFT_RATE * f * deltaTime;
            if ( mouseViewShiftPitch > MOUSE_VIEW_SHIFT_PITCH_LIMIT ) { mouseViewShiftPitch = MOUSE_VIEW_SHIFT_PITCH_LIMIT; }
        }
        else if (mouseY > bottomBoundary) {
            float f = (mouseY - bottomBoundary) / ((float)::screenHeight - bottomBoundary);
             mouseViewShiftPitch -= MOUSE_VIEW_SHIFT_RATE * f * deltaTime;
            if (mouseViewShiftPitch < -MOUSE_VIEW_SHIFT_PITCH_LIMIT) { mouseViewShiftPitch = -MOUSE_VIEW_SHIFT_PITCH_LIMIT; }
        }
    }
    
    if (OculusManager::isConnected()) {
        float yaw, pitch, roll;
        OculusManager::getEulerAngles(yaw, pitch, roll);
        
        myAvatar.setHeadYaw(-yaw);
        myAvatar.setHeadPitch(pitch);
        myAvatar.setHeadRoll(roll);
    }
    
    //  Get audio loudness data from audio input device
    #ifndef _WIN32
        myAvatar.setLoudness(audio.getInputLoudness());
    #endif

    // Update Avatar with latest camera and view frustum data...
    // NOTE: we get this from the view frustum, to make it simpler, since the
    // loadViewFrumstum() method will get the correct details from the camera
    // We could optimize this to not actually load the viewFrustum, since we don't
    // actually need to calculate the view frustum planes to send these details 
    // to the server.
    loadViewFrustum(::viewFrustum);
    myAvatar.setCameraPosition(::viewFrustum.getPosition());
    myAvatar.setCameraDirection(::viewFrustum.getDirection());
    myAvatar.setCameraUp(::viewFrustum.getUp());
    myAvatar.setCameraRight(::viewFrustum.getRight());
    myAvatar.setCameraFov(::viewFrustum.getFieldOfView());
    myAvatar.setCameraAspectRatio(::viewFrustum.getAspectRatio());
    myAvatar.setCameraNearClip(::viewFrustum.getNearClip());
    myAvatar.setCameraFarClip(::viewFrustum.getFarClip());
    
    AgentList* agentList = AgentList::getInstance();
    if (agentList->getOwnerID() != UNKNOWN_AGENT_ID) {
        // if I know my ID, send head/hand data to the avatar mixer and voxel server
        unsigned char broadcastString[200];
        unsigned char* endOfBroadcastStringWrite = broadcastString;
        
        *(endOfBroadcastStringWrite++) = PACKET_HEADER_HEAD_DATA;
        endOfBroadcastStringWrite += packAgentId(endOfBroadcastStringWrite, agentList->getOwnerID());
        
        endOfBroadcastStringWrite += myAvatar.getBroadcastData(endOfBroadcastStringWrite);
        
        const char broadcastReceivers[2] = {AGENT_TYPE_VOXEL, AGENT_TYPE_AVATAR_MIXER};
        AgentList::getInstance()->broadcastToAgents(broadcastString, endOfBroadcastStringWrite - broadcastString, broadcastReceivers, sizeof(broadcastReceivers));
    }

    // If I'm in paint mode, send a voxel out to VOXEL server agents.
    if (::paintOn) {
    
        glm::vec3 avatarPos = myAvatar.getPosition();

        // For some reason, we don't want to flip X and Z here.
        ::paintingVoxel.x = avatarPos.x / 10.0;
        ::paintingVoxel.y = avatarPos.y / 10.0;
        ::paintingVoxel.z = avatarPos.z / 10.0;
        
        if (::paintingVoxel.x >= 0.0 && ::paintingVoxel.x <= 1.0 &&
            ::paintingVoxel.y >= 0.0 && ::paintingVoxel.y <= 1.0 &&
            ::paintingVoxel.z >= 0.0 && ::paintingVoxel.z <= 1.0) {

            sendVoxelEditMessage(PACKET_HEADER_SET_VOXEL, ::paintingVoxel);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// loadViewFrustum()
//
// Description: this will load the view frustum bounds for EITHER the head
//                 or the "myCamera". 
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
    
    // Camera or Head?
    if (::cameraFrustum) {
        position = ::myCamera.getPosition();
    } else {
        position = ::myAvatar.getHeadPosition();
    }
    
    fov         = ::myCamera.getFieldOfView();
    nearClip    = ::myCamera.getNearClip();
    farClip     = ::myCamera.getFarClip();

    Orientation o = ::myCamera.getOrientation();

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
//                 or the "myCamera". 
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

// displays a single side (left, right, or combined for non-Oculus)
void displaySide(Camera& whichCamera) {
    glPushMatrix();
    
    if (::renderStarsOn) {
        // should be the first rendering pass - w/o depth buffer / lighting

        // compute starfield alpha based on distance from atmosphere
        float alpha = 1.0f;
        if (::renderAtmosphereOn) {
            float height = glm::distance(whichCamera.getPosition(), environment.getAtmosphereCenter());
            if (height < environment.getAtmosphereInnerRadius()) {
                alpha = 0.0f;
                
            } else if (height < environment.getAtmosphereOuterRadius()) {
                alpha = (height - environment.getAtmosphereInnerRadius()) /
                    (environment.getAtmosphereOuterRadius() - environment.getAtmosphereInnerRadius());
            }
        }

        // finally render the starfield
        stars.render(whichCamera.getFieldOfView(), whichCamera.getAspectRatio(), whichCamera.getNearClip(), alpha);
    }

    // draw the sky dome
    if (::renderAtmosphereOn) {
        environment.renderAtmosphere(whichCamera);
    }
    
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    
    // draw a red sphere  
    float sphereRadius = 0.25f;
    glColor3f(1,0,0);
    glPushMatrix();
        glutSolidSphere(sphereRadius, 15, 15);
    glPopMatrix();

    //draw a grid ground plane....
    drawGroundPlaneGrid(10.f);
    
    //  Draw voxels
    if (renderVoxels) {
        voxels.render();
    }
    
    // indicate what we'll be adding/removing in mouse mode, if anything
    if (::mouseVoxel.s != 0) {
        glPushMatrix();
        if (::mouseMode == ADD_VOXEL_MODE) {
            // use a contrasting color so that we can see what we're doing
            glColor3ub(::mouseVoxel.red + 128, ::mouseVoxel.green + 128, ::mouseVoxel.blue + 128);
        } else {
            glColor3ub(::mouseVoxel.red, ::mouseVoxel.green, ::mouseVoxel.blue);
        }
        glScalef(TREE_SCALE, TREE_SCALE, TREE_SCALE);
        glTranslatef(::mouseVoxel.x + ::mouseVoxel.s*0.5f,
                     ::mouseVoxel.y + ::mouseVoxel.s*0.5f,
                     ::mouseVoxel.z + ::mouseVoxel.s*0.5f);
        glLineWidth(4.0f);
        glutWireCube(::mouseVoxel.s);
        glLineWidth(1.0f);
        glPopMatrix();
    }
    
    if (::renderAvatarsOn) {
        //  Render avatars of other agents
        AgentList* agentList = AgentList::getInstance();
        agentList->lock();
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            if (agent->getLinkedData() != NULL && agent->getType() == AGENT_TYPE_AVATAR) {
                Avatar *avatar = (Avatar *)agent->getLinkedData();
                avatar->render(0, ::myCamera.getPosition());
            }
        }
        agentList->unlock();
        
        // Render my own Avatar 
        myAvatar.render(::lookingInMirror, ::myCamera.getPosition());
    }
    
    //  Render the world box
    if (!::lookingInMirror && ::renderStatsOn) { render_world_box(); }
    
    // brad's frustum for debugging
    if (::frustumOn) renderViewFrustum(::viewFrustum);
    
    glPopMatrix();
}

// this shader is an adaptation (HLSL -> GLSL, removed conditional) of the one in the Oculus sample
// code (Samples/OculusRoomTiny/RenderTiny_D3D1X_Device.cpp), which is under the Apache license
// (http://www.apache.org/licenses/LICENSE-2.0)
const char* DISTORTION_FRAGMENT_SHADER =
    "#version 120\n"
    "uniform sampler2D texture;"
    "uniform vec2 lensCenter;"
    "uniform vec2 screenCenter;"
    "uniform vec2 scale;"
    "uniform vec2 scaleIn;"
    "uniform vec4 hmdWarpParam;"
    "vec2 hmdWarp(vec2 in01) {"
    "   vec2 theta = (in01 - lensCenter) * scaleIn;"
    "   float rSq = theta.x * theta.x + theta.y * theta.y;"
    "   vec2 theta1 = theta * (hmdWarpParam.x + hmdWarpParam.y * rSq + "
    "                 hmdWarpParam.z * rSq * rSq + hmdWarpParam.w * rSq * rSq * rSq);"
    "   return lensCenter + scale * theta1;"
    "}"
    "void main(void) {"
    "   vec2 tc = hmdWarp(gl_TexCoord[0].st);"
    "   vec2 below = step(screenCenter.st + vec2(-0.25, -0.5), tc.st);"
    "   vec2 above = vec2(1.0, 1.0) - step(screenCenter.st + vec2(0.25, 0.5), tc.st);"
    "   gl_FragColor = mix(vec4(0.0, 0.0, 0.0, 1.0), texture2D(texture, tc), "
    "       above.s * above.t * below.s * below.t);"
    "}";

// the locations of the uniform variables
int textureLocation;
int lensCenterLocation;
int screenCenterLocation;
int scaleLocation;
int scaleInLocation;
int hmdWarpParamLocation;

// renders both sides into a texture, then renders the texture to the display with distortion
void displayOculus(Camera& whichCamera) {
    // magic numbers ahoy! in order to avoid pulling in the Oculus utility library that calculates
    // the rendering parameters from the hardware stats, i just folded their calculations into
    // constants using the stats for the current-model hardware as contained in the SDK file
    // LibOVR/Src/Util/Util_Render_Stereo.cpp

    // eye 

    // render the left eye view to the left side of the screen
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.151976, 0, 0); // +h, see Oculus SDK docs p. 26
    gluPerspective(whichCamera.getFieldOfView(), whichCamera.getAspectRatio(),
        whichCamera.getNearClip(), whichCamera.getFarClip());
    glTranslatef(0.032, 0, 0); // dip/2, see p. 27
    
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, ::screenWidth / 2, ::screenHeight);
    displaySide(whichCamera);

    // and the right eye to the right side
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glTranslatef(-0.151976, 0, 0); // -h
    gluPerspective(whichCamera.getFieldOfView(), whichCamera.getAspectRatio(),
        whichCamera.getNearClip(), whichCamera.getFarClip());
    glTranslatef(-0.032, 0, 0);
    
    glMatrixMode(GL_MODELVIEW);
    glViewport(::screenWidth / 2, 0, ::screenWidth / 2, ::screenHeight);
    displaySide(whichCamera);

    glPopMatrix();
    
    // restore our normal viewport
    glViewport(0, 0, ::screenWidth, ::screenHeight);

    if (::oculusTextureID == 0) {
        glGenTextures(1, &::oculusTextureID);
        glBindTexture(GL_TEXTURE_2D, ::oculusTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ::screenWidth, ::screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   
        
        ::oculusProgram = new ProgramObject();
        ::oculusProgram->attachFromSourceCode(GL_FRAGMENT_SHADER_ARB, DISTORTION_FRAGMENT_SHADER);
        ::oculusProgram->link();
        
        textureLocation = ::oculusProgram->getUniformLocation("texture");
        lensCenterLocation = ::oculusProgram->getUniformLocation("lensCenter");
        screenCenterLocation = ::oculusProgram->getUniformLocation("screenCenter");
        scaleLocation = ::oculusProgram->getUniformLocation("scale");
        scaleInLocation = ::oculusProgram->getUniformLocation("scaleIn");
        hmdWarpParamLocation = ::oculusProgram->getUniformLocation("hmdWarpParam");
        
    } else {
        glBindTexture(GL_TEXTURE_2D, ::oculusTextureID);
    }
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, ::screenWidth, ::screenHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, ::screenWidth, 0, ::screenHeight);           
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    
    // for reference on setting these values, see SDK file Samples/OculusRoomTiny/RenderTiny_Device.cpp
    
    float scaleFactor = 1.0 / ::oculusDistortionScale;
    float aspectRatio = (::screenWidth * 0.5) / ::screenHeight;
    
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    ::oculusProgram->bind();
    ::oculusProgram->setUniform(textureLocation, 0);
    ::oculusProgram->setUniform(lensCenterLocation, 0.287994, 0.5); // see SDK docs, p. 29
    ::oculusProgram->setUniform(screenCenterLocation, 0.25, 0.5);
    ::oculusProgram->setUniform(scaleLocation, 0.25 * scaleFactor, 0.5 * scaleFactor * aspectRatio);
    ::oculusProgram->setUniform(scaleInLocation, 4, 2 / aspectRatio);
    ::oculusProgram->setUniform(hmdWarpParamLocation, 1.0, 0.22, 0.24, 0);

    glColor3f(1, 0, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(0.5, 0);
    glVertex2f(::screenWidth/2, 0);
    glTexCoord2f(0.5, 1);
    glVertex2f(::screenWidth / 2, ::screenHeight);
    glTexCoord2f(0, 1);
    glVertex2f(0, ::screenHeight);
    glEnd();
    
    ::oculusProgram->setUniform(lensCenterLocation, 0.787994, 0.5);
    ::oculusProgram->setUniform(screenCenterLocation, 0.75, 0.5);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.5, 0);
    glVertex2f(::screenWidth / 2, 0);
    glTexCoord2f(1, 0);
    glVertex2f(::screenWidth, 0);
    glTexCoord2f(1, 1);
    glVertex2f(::screenWidth, ::screenHeight);
    glTexCoord2f(0.5, 1);
    glVertex2f(::screenWidth / 2, ::screenHeight);
    glEnd();
    
    glEnable(GL_BLEND);           
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    ::oculusProgram->release();
    
    glPopMatrix();
}

void displayOverlay() {
    //  Render 2D overlay:  I/O level bar graphs and text  
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity(); 
        gluOrtho2D(0, ::screenWidth, ::screenHeight, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
    
        #ifndef _WIN32
        audio.render(::screenWidth, ::screenHeight);
        audioScope.render(20, ::screenHeight - 200);
        #endif

       //noiseTest(::screenWidth, ::screenHeight);
    
    if (displayHeadMouse && !::lookingInMirror && USING_INVENSENSE_MPU9150) {
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
    if (displayLevels) serialPort.renderLevels(::screenWidth,::screenHeight);
    
    //  Display stats and log text onscreen
    glLineWidth(1.0f);
    glPointSize(1.0f);
    
    if (::renderStatsOn) { displayStats(); }
    if (::logOn) { logger.render(::screenWidth, ::screenHeight); }
        
    //  Show menu
    if (::menuOn) {
        glLineWidth(1.0f);
        glPointSize(1.0f);
        menu.render(::screenWidth,::screenHeight);
    }

    //  Show chat entry field
    if (::chatEntryOn) {
        chatEntry.render(::screenWidth, ::screenHeight);
    }

    //  Stats at upper right of screen about who domain server is telling us about
    glPointSize(1.0f);
    char agents[100];
    
    AgentList* agentList = AgentList::getInstance();
    int totalAvatars = 0, totalServers = 0;
    
    for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
        agent->getType() == AGENT_TYPE_AVATAR ? totalAvatars++ : totalServers++;
    }
    
    sprintf(agents, "Servers: %d, Avatars: %d\n", totalServers, totalAvatars);
    drawtext(::screenWidth - 150, 20, 0.10, 0, 1.0, 0, agents, 1, 0, 0);
    
    if (::paintOn) {
    
        char paintMessage[100];
        sprintf(paintMessage,"Painting (%.3f,%.3f,%.3f/%.3f/%d,%d,%d)",
            ::paintingVoxel.x,::paintingVoxel.y,::paintingVoxel.z,::paintingVoxel.s,
            (unsigned int)::paintingVoxel.red,(unsigned int)::paintingVoxel.green,(unsigned int)::paintingVoxel.blue);
        drawtext(::screenWidth - 350, 50, 0.10, 0, 1.0, 0, paintMessage, 1, 1, 0);
    }
    
    glPopMatrix();
}

void display(void)
{
    PerfStat("display");

    glEnable(GL_LINE_SMOOTH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    
    glPushMatrix();  {
        glLoadIdentity();

        // camera settings
        if (OculusManager::isConnected()) {
            myAvatar.setDisplayingHead(false);
            myCamera.setUpShift       (0.0f);
            myCamera.setDistance      (0.0f);
            myCamera.setTightness     (100.0f); 
            myCamera.setTargetPosition(myAvatar.getHeadPosition());
            myCamera.setTargetRotation(myAvatar.getBodyYaw() + myAvatar.getHeadYaw(), -myAvatar.getHeadPitch(), myAvatar.getHeadRoll());
        
        } else if (myCamera.getMode() == CAMERA_MODE_MIRROR) {
            myCamera.setTightness     (100.0f); 
            myCamera.setTargetPosition(myAvatar.getSpringyHeadPosition());
            myCamera.setTargetRotation(myAvatar.getBodyYaw() - 180.0f, 0.0f, 0.0f);
            
        } else {        
            if (myCamera.getMode() == CAMERA_MODE_FIRST_PERSON) {
                myCamera.setTargetPosition(myAvatar.getSpringyHeadPosition());
                myCamera.setTargetRotation(myAvatar.getAbsoluteHeadYaw()- mouseViewShiftYaw, -20.0f + myAvatar.getRenderPitch() + mouseViewShiftPitch, 0.0f);                
            } else if (myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
            
                myCamera.setTargetPosition(myAvatar.getHeadPosition());
                myCamera.setTargetRotation(myAvatar.getBodyYaw() - mouseViewShiftYaw, mouseViewShiftPitch, 0.0f);
            }
        }

        // important...
        myCamera.update( 1.f/FPS );
        
        // Render anything (like HUD items) that we want to be in 3D but not in worldspace
        /*
        const float HUD_Z_OFFSET = -5.f;
        glPushMatrix();
        glm::vec3 test(0.5, 0.5, 0.5);
        glTranslatef(1, 1, HUD_Z_OFFSET);
        drawVector(&test);
        glPopMatrix();
        */
        
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
            viewFrustumOffsetCamera.setTargetYaw(::viewFrustumOffsetYaw + myAvatar.getBodyYaw());
            viewFrustumOffsetCamera.setPitch    (::viewFrustumOffsetPitch   );
            viewFrustumOffsetCamera.setRoll     (::viewFrustumOffsetRoll    ); 
            viewFrustumOffsetCamera.setUpShift  (::viewFrustumOffsetUp      );
            viewFrustumOffsetCamera.setDistance (::viewFrustumOffsetDistance);
            viewFrustumOffsetCamera.update(1.f/FPS);
            whichCamera = viewFrustumOffsetCamera;
        }        

        // transform view according to whichCamera
        // could be myCamera (if in normal mode)
        // or could be viewFrustumOffsetCamera if in offset mode
        // I changed the ordering here - roll is FIRST (JJV) 

        glRotatef   (        whichCamera.getRoll(),  IDENTITY_FRONT.x, IDENTITY_FRONT.y, IDENTITY_FRONT.z);
        glRotatef   (        whichCamera.getPitch(), IDENTITY_RIGHT.x, IDENTITY_RIGHT.y, IDENTITY_RIGHT.z);
        glRotatef   (180.0 - whichCamera.getYaw(),     IDENTITY_UP.x,    IDENTITY_UP.y,    IDENTITY_UP.z   );

        glTranslatef(-whichCamera.getPosition().x, -whichCamera.getPosition().y, -whichCamera.getPosition().z);
        
        //  Setup 3D lights (after the camera transform, so that they are positioned in world space)
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        
        GLfloat light_position0[] = { 1.0, 1.0, 0.0, 0.0 };
        glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
        GLfloat ambient_color[] = { 0.7, 0.7, 0.8 };   
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color);
        GLfloat diffuse_color[] = { 0.8, 0.7, 0.7 };  
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color);
        GLfloat specular_color[] = { 1.0, 1.0, 1.0, 1.0};
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular_color);
        
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular_color);
        glMateriali(GL_FRONT, GL_SHININESS, 96);
        
        if (::oculusOn) {
            displayOculus(whichCamera);
            
        } else {
            displaySide(whichCamera);
            glPopMatrix();
            
            displayOverlay();
        }
    }
    
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
    return setValue(state, &::lookingInMirror);
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

int setLog(int state) {
    int iRet = setValue(state, &::logOn);
    return iRet;
}

int setGyroLook(int state) {
    int iRet = setValue(state, &::gyroLook);
    return iRet;
}

int setFullscreen(int state) {
    bool wasFullscreen = ::fullscreen;
    int value = setValue(state, &::fullscreen);
    if (::fullscreen != wasFullscreen) {
        if (::fullscreen) {
            glutFullScreen();    
            
        } else {
            glutReshapeWindow(::screenWidth, ::screenHeight);
        }
    }
    return value;
}

int setVoxels(int state) {
    return setValue(state, &::renderVoxels);
}

int setStars(int state) {
    return setValue(state, &::renderStarsOn);
}

int setAtmosphere(int state) {
    return setValue(state, &::renderAtmosphereOn);
}

int setRenderAvatars(int state) {
    return setValue(state, &::renderAvatarsOn);
}

int setRenderFirstPerson(int state) {
    bool value = setValue(state, &::renderFirstPersonOn);
    if (state == MENU_ROW_PICKED) {
        if (::renderFirstPersonOn) {
                
            Camera::CameraFollowingAttributes a;
            a.upShift   = 0.0f;
            a.distance  = 0.0f;
            a.tightness = 100.0f;
            myCamera.setMode(CAMERA_MODE_FIRST_PERSON, a);
            myAvatar.setDisplayingHead(false);  
        } else {
            Camera::CameraFollowingAttributes a;            
            a.upShift   = -0.2f;
            a.distance  = 1.5f;
            a.tightness = 8.0f;
            myCamera.setMode(CAMERA_MODE_THIRD_PERSON, a);
            myAvatar.setDisplayingHead(true);  
        }
    }
    return value;
}

int setOculus(int state) {
    bool wasOn = ::oculusOn;
    int value = setValue(state, &::oculusOn);
    if (::oculusOn != wasOn) {
        reshape(::screenWidth, ::screenHeight);
    }
    return value;
}

int setStats(int state) {
    return setValue(state, &::renderStatsOn);
}

int setMenu(int state) {
    return setValue(state, &::menuOn);
}

int setRenderWarnings(int state) {
    int value = setValue(state, &::renderWarningsOn);
    if (state == MENU_ROW_PICKED) {
        ::voxels.setRenderPipelineWarnings(::renderWarningsOn);
    }
    return value;
}

int setWantResIn(int state) {
    int value = setValue(state, &::wantResIn);
    if (state == MENU_ROW_PICKED) {
        ::myAvatar.setWantResIn(::wantResIn);
    }
    return value;
}

int setWantMonochrome(int state) {
    int value = setValue(state, &::wantMonochrome);
    if (state == MENU_ROW_PICKED) {
        ::myAvatar.setWantColor(!::wantMonochrome);
    }
    return value;
}

int setDisplayFrustum(int state) {
    return setValue(state, &::frustumOn);
}

int setFrustumOffset(int state) {
    int value = setValue(state, &::viewFrustumFromOffset);

    // reshape so that OpenGL will get the right lens details for the camera of choice    
    if (state == MENU_ROW_PICKED) {
        reshape(::screenWidth, ::screenHeight);
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

int doKillLocalVoxels(int state) {
    if (state == MENU_ROW_PICKED) {
        ::wantToKillLocalVoxels = true;
    }
    return state;
}

int doRandomizeVoxelColors(int state) {
    if (state == MENU_ROW_PICKED) {
        ::voxels.randomizeVoxelColors();
    }
    return state;
}

int doFalseRandomizeEveryOtherVoxelColors(int state) {
    if (state == MENU_ROW_PICKED) {
        ::voxels.falseColorizeRandomEveryOther();
    }
    return state;
}

int doTreeStats(int state) {
    if (state == MENU_ROW_PICKED) {
        ::voxels.collectStatsForTreesAndVBOs();
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
    MenuColumn *menuColumnOptions, *menuColumnRender, *menuColumnTools, *menuColumnDebug, *menuColumnFrustum;
    //  Options
    menuColumnOptions = menu.addColumn("Options");
    menuColumnOptions->addRow("Mirror (h)", setHead); 
    menuColumnOptions->addRow("Noise (n)", setNoise);
    menuColumnOptions->addRow("Gyro Look", setGyroLook);
    menuColumnOptions->addRow("Fullscreen (f)", setFullscreen);
    menuColumnOptions->addRow("Quit (q)", quitApp);

    //  Render
    menuColumnRender = menu.addColumn("Render");
    menuColumnRender->addRow("Voxels (V)", setVoxels);
    menuColumnRender->addRow("Stars (*)", setStars);
    menuColumnRender->addRow("Atmosphere (A)", setAtmosphere);
    menuColumnRender->addRow("Avatars", setRenderAvatars);
    menuColumnRender->addRow("First Person (p)", setRenderFirstPerson);
    menuColumnRender->addRow("Oculus (o)", setOculus);
    
    //  Tools
    menuColumnTools = menu.addColumn("Tools");
    menuColumnTools->addRow("Stats (/)", setStats);
    menuColumnTools->addRow("Log ", setLog);
    menuColumnTools->addRow("(M)enu", setMenu);

    // Frustum Options
    menuColumnFrustum = menu.addColumn("Frustum");
    menuColumnFrustum->addRow("Display (F)rustum", setDisplayFrustum); 
    menuColumnFrustum->addRow("Use (O)ffset Camera", setFrustumOffset); 
    menuColumnFrustum->addRow("Switch (C)amera", setFrustumOrigin); 
    menuColumnFrustum->addRow("(R)ender Mode", setFrustumRenderMode, getFrustumRenderModeName); 

    // Debug
    menuColumnDebug = menu.addColumn("Debug");
    menuColumnDebug->addRow("Show Render Pipeline Warnings", setRenderWarnings);
    menuColumnDebug->addRow("Kill Local Voxels", doKillLocalVoxels);
    menuColumnDebug->addRow("Randomize Voxel TRUE Colors", doRandomizeVoxelColors);
    menuColumnDebug->addRow("FALSE Color Voxels Randomly", doFalseRandomizeVoxelColors);
    menuColumnDebug->addRow("FALSE Color Voxel Every Other Randomly", doFalseRandomizeEveryOtherVoxelColors);
    menuColumnDebug->addRow("FALSE Color Voxels by Distance", doFalseColorizeByDistance);
    menuColumnDebug->addRow("FALSE Color Voxel Out of View", doFalseColorizeInView);
    menuColumnDebug->addRow("Show TRUE Colors", doTrueVoxelColors);
    menuColumnDebug->addRow("Calculate Tree Stats", doTreeStats);
    menuColumnDebug->addRow("Wants Res-In", setWantResIn);
    menuColumnDebug->addRow("Wants Monochrome", setWantMonochrome);
}

void testPointToVoxel() {
    float y=0;
    float z=0;
    float s=0.1;
    for (float x=0; x<=1; x+= 0.05) {
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
    glm::vec3 avatarPos = myAvatar.getPosition();

    ::paintingVoxel.x = avatarPos.z/-10.0;    // voxel space x is negative z head space
    ::paintingVoxel.y = avatarPos.y/-10.0;  // voxel space y is negative y head space
    ::paintingVoxel.z = avatarPos.x/-10.0;  // voxel space z is negative x head space
    ::paintingVoxel.s = 1.0/256;
    
    shiftPaintingColor();
}

void addVoxelInFrontOfAvatar() {
    VoxelDetail detail;
    
    glm::vec3 position = (myAvatar.getPosition() + myAvatar.getCameraDirection()) * (1.0f / TREE_SCALE);
    detail.s = ::mouseVoxelScale;
    
    detail.x = detail.s * floor(position.x / detail.s);
    detail.y = detail.s * floor(position.y / detail.s);
    detail.z = detail.s * floor(position.z / detail.s);
    detail.red = 128;
    detail.green = 128;
    detail.blue = 128;
    
    sendVoxelEditMessage(PACKET_HEADER_SET_VOXEL, detail);
    
    // create the voxel locally so it appears immediately            
    voxels.createVoxel(detail.x, detail.y, detail.z, detail.s, detail.red, detail.green, detail.blue);
}

void addVoxelUnderCursor() {
    if (::mouseVoxel.s != 0) {    
        sendVoxelEditMessage(PACKET_HEADER_SET_VOXEL, ::mouseVoxel);
        
        // create the voxel locally so it appears immediately            
        voxels.createVoxel(::mouseVoxel.x, ::mouseVoxel.y, ::mouseVoxel.z, ::mouseVoxel.s,
                           ::mouseVoxel.red, ::mouseVoxel.green, ::mouseVoxel.blue);
    }
}

void deleteVoxelUnderCursor() {
    if (::mouseVoxel.s != 0) {
        sendVoxelEditMessage(PACKET_HEADER_ERASE_VOXEL, ::mouseVoxel);
        
        // delete the voxel locally so it disappears immediately            
        voxels.deleteVoxelAt(::mouseVoxel.x, ::mouseVoxel.y, ::mouseVoxel.z, ::mouseVoxel.s);
    }
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

void specialkey(int k, int x, int y) {
    if (::chatEntryOn) {
        chatEntry.specialKey(k);
        return;
    }
    
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
    }    
}

void keyUp(unsigned char k, int x, int y) {
    if (::chatEntryOn) {
        myAvatar.setKeyState(NO_KEY_DOWN);
        return;
    }

    if (k == 'e') myAvatar.setDriveKeys(UP, 0);
    if (k == 'c') myAvatar.setDriveKeys(DOWN, 0);
    if (k == 'w') myAvatar.setDriveKeys(FWD, 0);
    if (k == 's') myAvatar.setDriveKeys(BACK, 0);
    if (k == 'a') myAvatar.setDriveKeys(ROT_LEFT, 0);
    if (k == 'd') myAvatar.setDriveKeys(ROT_RIGHT, 0);
}

void toggleMouseMode(MouseMode mode) {
    ::mouseMode = (::mouseMode == mode) ? NO_EDIT_MODE : mode;
}

void key(unsigned char k, int x, int y) {
    if (::chatEntryOn) {
        if (chatEntry.key(k)) {
            myAvatar.setKeyState(k == '\b' || k == 127 ? // backspace or delete
                DELETE_KEY_DOWN : INSERT_KEY_DOWN);            
            myAvatar.setChatMessage(string(chatEntry.getContents().size(), SOLID_BLOCK_CHAR));
            
        } else {
            myAvatar.setChatMessage(chatEntry.getContents());
            chatEntry.clear();
            ::chatEntryOn = false;
        }
        return;
    }
    
    //  Process keypresses 

    if (k == 'S') {
        ::voxels.collectStatsForTreesAndVBOs();
    }
    
    if (k == 'q' || k == 'Q')  ::terminate();
    if (k == '/')  ::renderStatsOn = !::renderStatsOn;        // toggle stats
    if (k == '*')  ::renderStarsOn = !::renderStarsOn;        // toggle stars
    if (k == 'V' || k == 'v')  ::renderVoxels = !::renderVoxels;        // toggle voxels
    if (k == 'A') ::renderAtmosphereOn = !::renderAtmosphereOn;
    if (k == 'F')  ::frustumOn = !::frustumOn;        // toggle view frustum debugging
    if (k == 'C')  ::cameraFrustum = !::cameraFrustum;    // toggle which frustum to look at
    if (k == 'O' || k == 'G') setFrustumOffset(MENU_ROW_PICKED); // toggle view frustum offset debugging
    if (k == 'f') setFullscreen(!::fullscreen);
    if (k == 'o') setOculus(!::oculusOn);
    if (k == 'p') setRenderFirstPerson(MENU_ROW_PICKED);
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

//    if (k == '|') ViewFrustum::fovAngleAdust   -= 0.05;
//    if (k == '\\') ViewFrustum::fovAngleAdust  += 0.05;

    if (k == 'R') setFrustumRenderMode(MENU_ROW_PICKED);

    if (k == '&') {
        ::paintOn = !::paintOn;        // toggle paint
        ::setupPaintingVoxel();        // also randomizes colors
    }
    if (k == '^')  ::shiftPaintingColor();        // shifts randomize color between R,G,B dominant
    if (k == '-')  ::sendVoxelServerEraseAll();    // sends erase all command to voxel server
    if (k == '%')  ::sendVoxelServerAddScene();    // sends add scene command to voxel server
    if (k == '1')  ::mouseMode = (::mouseMode == ADD_VOXEL_MODE) ? NO_EDIT_MODE : ADD_VOXEL_MODE;
    if (k == '2')  ::mouseMode = (::mouseMode == DELETE_VOXEL_MODE) ? NO_EDIT_MODE : DELETE_VOXEL_MODE;
    if (k == '3')  ::mouseMode = (::mouseMode == COLOR_VOXEL_MODE) ? NO_EDIT_MODE : COLOR_VOXEL_MODE;
    if (k == '4')  addVoxelInFrontOfAvatar();
    if (k == '5')  ::mouseVoxelScale /= 2;
    if (k == '6')  ::mouseVoxelScale *= 2; 
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
        ::lookingInMirror = !::lookingInMirror;
        #ifndef _WIN32
        audio.setMixerLoopbackFlag(::lookingInMirror);
        
        if (::lookingInMirror) {
            Camera::CameraFollowingAttributes a;
            a.upShift   = 0.0f;
            a.distance  = 0.2f;
            a.tightness = 100.0f;
            myCamera.setMode(CAMERA_MODE_MIRROR, a);
            myAvatar.setDisplayingHead(true);  
        } else {
            Camera::CameraFollowingAttributes a;
            a.upShift   = -0.2f;
            a.distance  = 1.5f;
            a.tightness = 8.0f;
            myCamera.setMode(CAMERA_MODE_THIRD_PERSON, a);
            myAvatar.setDisplayingHead(true);  
        }
        #endif
    }
    
    if (k == 'm' || k == 'M') setMenu(MENU_ROW_PICKED);
    
    if (k == 'l') displayLevels = !displayLevels;
    if (k == 'e') myAvatar.setDriveKeys(UP, 1);
    if (k == 'c') myAvatar.setDriveKeys(DOWN, 1);
    if (k == 'w') myAvatar.setDriveKeys(FWD, 1);
    if (k == 's') myAvatar.setDriveKeys(BACK, 1);
    if (k == ' ') resetSensors();
    if (k == 'a') myAvatar.setDriveKeys(ROT_LEFT, 1);
    if (k == 'd') myAvatar.setDriveKeys(ROT_RIGHT, 1);
    
    if (k == '\r') {
        ::chatEntryOn = true;
        myAvatar.setKeyState(NO_KEY_DOWN);
        myAvatar.setChatMessage(string());
    }
}

//  Receive packets from other agents/servers and decide what to do with them!
void* networkReceive(void* args) {
    sockaddr senderAddress;
    ssize_t bytesReceived;
    
    while (!stopNetworkReceiveThread) {
        // check to see if the UI thread asked us to kill the voxel tree. since we're the only thread allowed to do that
        if (::wantToKillLocalVoxels) {
            ::voxels.killLocalVoxels();
            ::wantToKillLocalVoxels = false;
        }
    
        if (AgentList::getInstance()->getAgentSocket().receive(&senderAddress, incomingPacket, &bytesReceived)) {
            packetCount++;
            bytesCount += bytesReceived;
            
            switch (incomingPacket[0]) {
                case PACKET_HEADER_TRANSMITTER_DATA:
                    //  Process UDP packets that are sent to the client from local sensor devices 
                    myAvatar.processTransmitterData(incomingPacket, bytesReceived);
                    break;
                case PACKET_HEADER_VOXEL_DATA:
                case PACKET_HEADER_VOXEL_DATA_MONOCHROME:
                case PACKET_HEADER_Z_COMMAND:
                case PACKET_HEADER_ERASE_VOXEL:
                    voxels.parseData(incomingPacket, bytesReceived);
                    break;
                case PACKET_HEADER_ENVIRONMENT_DATA:
                    environment.parseData(incomingPacket, bytesReceived);
                    break;
                case PACKET_HEADER_BULK_AVATAR_DATA:
                    AgentList::getInstance()->processBulkAgentData(&senderAddress,
                                                                   incomingPacket,
                                                                   bytesReceived);
                    break;
                default:
                    AgentList::getInstance()->processAgentData(&senderAddress, incomingPacket, bytesReceived);
                    break;
            }
        } else if (!enableNetworkThread) {
            break;
        }
    }
    
    if (enableNetworkThread) {
        pthread_exit(0); 
    }
    return NULL; 
}

glm::vec3 getFaceVector(BoxFace face) {
    switch (face) {
        case MIN_X_FACE:
            return glm::vec3(-1, 0, 0);
        
        case MAX_X_FACE:
            return glm::vec3(1, 0, 0);
        
        case MIN_Y_FACE:
            return glm::vec3(0, -1, 0);
        
        case MAX_Y_FACE:
            return glm::vec3(0, 1, 0);
        
        case MIN_Z_FACE:
            return glm::vec3(0, 0, -1);
            
        case MAX_Z_FACE:
            return glm::vec3(0, 0, 1);
    }
}

void idle(void) {
    timeval check;
    gettimeofday(&check, NULL);
    
    //  Only run simulation code if more than IDLE_SIMULATE_MSECS have passed since last time
    
    if (diffclock(&lastTimeIdle, &check) > IDLE_SIMULATE_MSECS) {
        
        float deltaTime = 1.f/FPS;

        // update behaviors for avatar hand movement: handControl takes mouse values as input,
        // and gives back 3D values modulated for smooth transitioning between interaction modes.
        handControl.update(mouseX, mouseY);
        myAvatar.setHandMovementValues(handControl.getValues());        
        
        // tell my avatar if the mouse is being pressed...
        myAvatar.setMousePressed(mousePressed);
        
        // check what's under the mouse and update the mouse voxel
        glm::vec3 mouseRayOrigin, mouseRayDirection;
        viewFrustum.computePickRay(mouseX / (float)::screenWidth, mouseY / (float)::screenHeight, mouseRayOrigin, mouseRayDirection);

        // tell my avatar the posiion and direction of the ray projected ino the world based on the mouse position        
        myAvatar.setMouseRay(mouseRayOrigin, mouseRayDirection); 

        float distance;
        BoxFace face;
        ::mouseVoxel.s = 0.0f;
        if (voxels.findRayIntersection(mouseRayOrigin, mouseRayDirection, ::mouseVoxel, distance, face)) {
            // find the nearest voxel with the desired scale
            if (::mouseVoxelScale > ::mouseVoxel.s) {
                // choose the larger voxel that encompasses the one selected
                ::mouseVoxel.x = ::mouseVoxelScale * floorf(::mouseVoxel.x / ::mouseVoxelScale); 
                ::mouseVoxel.y = ::mouseVoxelScale * floorf(::mouseVoxel.y / ::mouseVoxelScale); 
                ::mouseVoxel.z = ::mouseVoxelScale * floorf(::mouseVoxel.z / ::mouseVoxelScale);
                ::mouseVoxel.s = ::mouseVoxelScale;
            
            } else if (::mouseVoxelScale < ::mouseVoxel.s) {
                glm::vec3 pt = (mouseRayOrigin + mouseRayDirection * distance) / (float)TREE_SCALE -
                    getFaceVector(face) * (::mouseVoxelScale * 0.5f);
                ::mouseVoxel.x = ::mouseVoxelScale * floorf(pt.x / ::mouseVoxelScale); 
                ::mouseVoxel.y = ::mouseVoxelScale * floorf(pt.y / ::mouseVoxelScale); 
                ::mouseVoxel.z = ::mouseVoxelScale * floorf(pt.z / ::mouseVoxelScale);
                ::mouseVoxel.s = ::mouseVoxelScale;
            }
                
            if (::mouseMode == COLOR_VOXEL_MODE) {
                ::mouseVoxel.red = 0;
                ::mouseVoxel.green = 255;
                ::mouseVoxel.blue = 0;
                
            } else if (::mouseMode == DELETE_VOXEL_MODE) {
                // red indicates deletion
                ::mouseVoxel.red = 255;
                ::mouseVoxel.green = ::mouseVoxel.blue = 0;
            }
        }
                
        // when walking handControl stops
        if (myAvatar.getMode() == AVATAR_MODE_WALKING) {
            handControl.stop();
            mouseViewShiftYaw   *= 0.9;
            mouseViewShiftPitch *= 0.9;
        }
        
        if (myCamera.getMode() != CAMERA_MODE_MIRROR) {
            if (myAvatar.getIsNearInteractingOther()) {
                myAvatar.setDisplayingHead(false);
                Camera::CameraFollowingAttributes a;
                a.upShift   = 0.0f;
                a.distance  = 0.0f;
                a.tightness = 100.0f;
                myCamera.setMode(CAMERA_MODE_FIRST_PERSON, a);
            } else {
                if (myCamera.getMode() != CAMERA_MODE_THIRD_PERSON) {
                    myAvatar.setDisplayingHead(true);
                    Camera::CameraFollowingAttributes a;            
                    a.upShift   = -0.2f;
                    a.distance  = 1.5f;
                    a.tightness = 8.0f;
                    myCamera.setMode(CAMERA_MODE_THIRD_PERSON, a);    
                }
            }
        }
        
        //  Read serial port interface devices
        if (serialPort.active) {
            serialPort.readData(); 
        }
        
        //  Sample hardware, update view frustum if needed, and send avatar data to mixer/agents
        updateAvatar(deltaTime);

        // read incoming packets from network
        if (!enableNetworkThread) {
            networkReceive(0);
        }
        
        //loop through all the remote avatars and simulate them...
        AgentList* agentList = AgentList::getInstance();
        agentList->lock();
        for(AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            if (agent->getLinkedData() != NULL) {
                Avatar *avatar = (Avatar *)agent->getLinkedData();
                avatar->simulate(deltaTime);
                avatar->setMouseRay(mouseRayOrigin, mouseRayDirection); 
            }
        }
        agentList->unlock();
    
        myAvatar.setGravity(getGravity(myAvatar.getPosition()));
        myAvatar.simulate(deltaTime);
        
        //  Update audio stats for procedural sounds
        audio.setLastAcceleration(myAvatar.getThrust());
        audio.setLastVelocity(myAvatar.getVelocity());
        
        glutPostRedisplay();
        lastTimeIdle = check;
    }
}

void reshape(int width, int height) {
    ::screenWidth = width;
    ::screenHeight = height; 
    aspectRatio = ((float)width/(float)height); // based on screen resize

    // get the lens details from the current camera
    Camera& camera = ::viewFrustumFromOffset ? (::viewFrustumOffsetCamera) : (::myCamera);
    float nearClip = camera.getNearClip();
    float farClip = camera.getFarClip();
    float fov;

    if (::oculusOn) {
        // more magic numbers; see Oculus SDK docs, p. 32
        camera.setAspectRatio(aspectRatio *= 0.5);
        camera.setFieldOfView(fov = 2 * atan((0.0468 * ::oculusDistortionScale) / 0.041) * (180 / PI));
        
        // resize the render texture
        if (::oculusTextureID != 0) {
            glBindTexture(GL_TEXTURE_2D, ::oculusTextureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ::screenWidth, ::screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    } else {
        camera.setAspectRatio(aspectRatio);
        camera.setFieldOfView(fov = 60);
    }

    // Tell our viewFrustum about this change
    ::viewFrustum.setAspectRatio(aspectRatio);

    glViewport(0, 0, width, height); // shouldn't this account for the menu???

    glMatrixMode(GL_PROJECTION);
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

//Find and return the gravity vector at this location
glm::vec3 getGravity(glm::vec3 pos) {
    //
    //  For now, we'll test this with a simple global lookup, but soon we will add getting this
    //  from the domain/voxelserver (or something similar)
    //
    if ((pos.x >  0.f) &&
        (pos.x < 10.f) &&
        (pos.z >  0.f) &&
        (pos.z < 10.f) &&
        (pos.y >  0.f) &&
        (pos.y <  3.f)) {
        //  If above ground plane, turn gravity on
        return glm::vec3(0.f, -1.f, 0.f);
    } else {
        //  If flying in space, turn gravity OFF
        return glm::vec3(0.f, 0.f, 0.f);
    }
}

bool menuDisplayed = false;
void mouseFunc(int button, int state, int x, int y) {
    bool menuFound = menu.mouseClick(x, y);

    // If we didn't previously have the menu displayed, and we did just click on the menu, then
    // go into menuDisplayed mode....
    if (!::menuDisplayed && menuFound) {
        ::menuDisplayed = true;
    }

    // If the menu was displayed, and we're not over a menu, then leave menu mode
    if (::menuDisplayed && !menuFound) {
        ::menuDisplayed = false;
        menu.hidePopupMenu();
    }
    
    // In menu displayed mode use old logic
    if (::menuDisplayed) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            if (state == GLUT_DOWN && !menu.mouseClick(x, y)) {
                mouseX = x;
                mouseY = y;
                mousePressed = 1;
            } else if (state == GLUT_UP) {
                mouseX = x;
                mouseY = y;
                mousePressed = 0;
            }
        }
    } else {
        if (button == GLUT_LEFT_BUTTON) {
            mouseX = x;
            mouseY = y;
            
            if (state == GLUT_DOWN) {
                mousePressed = 1;
                if (::mouseMode == ADD_VOXEL_MODE || ::mouseMode == COLOR_VOXEL_MODE) {
                    addVoxelUnderCursor();
                
                } else if (::mouseMode == DELETE_VOXEL_MODE) {
                    deleteVoxelUnderCursor();    
                }
            } else if (state == GLUT_UP) {
                mousePressed = 0;
            }
        } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN && ::mouseMode != NO_EDIT_MODE) {
            deleteVoxelUnderCursor();
        }
    }
}


void motionFunc(int x, int y) {
    mouseX = x;
    mouseY = y;
}

void mouseoverFunc(int x, int y){
    menu.mouseOver(x, y);

    mouseX = x;
    mouseY = y;
}

void attachNewHeadToAgent(Agent *newAgent) {
    if (newAgent->getLinkedData() == NULL) {
        newAgent->setLinkedData(new Avatar(false));
    }
}

#ifndef _WIN32
void audioMixerUpdate(in_addr_t newMixerAddress, in_port_t newMixerPort) {
    audio.updateMixerParams(newMixerAddress, newMixerPort);
}
#endif

int main(int argc, const char * argv[]) {
    
    gettimeofday(&applicationStartupTime, NULL);
    printLog("Interface Startup:\n");
    
    voxels.setViewFrustum(&::viewFrustum);

    shared_lib::printLog = & ::printLog;
    voxels_lib::printLog = & ::printLog;
    avatars_lib::printLog = & ::printLog;

    unsigned int listenPort = AGENT_SOCKET_LISTEN_PORT;
    const char* portStr = getCmdOption(argc, argv, "--listenPort");
    if (portStr) {
        listenPort = atoi(portStr);
    }
    AgentList::createInstance(AGENT_TYPE_AVATAR, listenPort);
    enableNetworkThread = !cmdOptionExists(argc, argv, "--nonblocking");
    if (!enableNetworkThread) {
        AgentList::getInstance()->getAgentSocket().setBlocking(false);
    }
    
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
    int wsaresult = WSAStartup(MAKEWORD(2,2), &WsaData);
#endif

    // start the agentList threads
    AgentList::getInstance()->startSilentAgentRemovalThread();
    AgentList::getInstance()->startDomainServerCheckInThread();
    AgentList::getInstance()->startPingUnknownAgentsThread();

    glutInit(&argc, (char**)argv);
    ::screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    ::screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(::screenWidth, ::screenHeight);
    glutCreateWindow("Interface");
    printLog( "Created Display Window.\n" );
    
    #ifdef _WIN32
    glewInit();
    printLog( "Glew Init complete.\n" );

    #endif
        
    // we need to create a QApplication instance in order to use Qt's font rendering
    Application app(argc, const_cast<char**>(argv));
    printLog( "Created QT Application.\n" );
    
    // Before we render anything, let's set up our viewFrustumOffsetCamera with a sufficiently large
    // field of view and near and far clip to make it interesting.
    //viewFrustumOffsetCamera.setFieldOfView(90.0);
    viewFrustumOffsetCamera.setNearClip(0.1);
    viewFrustumOffsetCamera.setFarClip(500.0*TREE_SCALE);

    
    initMenu();
    initDisplay();

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
    printLog( "Initialized Display.\n" );

    
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
    if (enableNetworkThread) {
        pthread_create(&networkReceiveThread, NULL, networkReceive, NULL);
        printLog("Network receive thread created.\n"); 
    }
    
    myAvatar.readAvatarDataFromFile();
    
    glutTimerFunc(1000, Timer, 0);
    glutMainLoop();

    printLog("Normal exit.\n");
    ::terminate();
    return EXIT_SUCCESS;
}   
