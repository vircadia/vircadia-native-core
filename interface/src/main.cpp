//
//  
//  Interface
//   
//  Show a field of objects rendered in 3D, with yaw and pitch of scene driven 
//  by accelerometer data
//  serial port connected to Maple board/arduino. 
//
//  Keyboard Commands: 
//
//  / = toggle stats display
//  spacebar = reset gyros/head
//  h = render Head
//  l = show incoming gyro levels
//

#include "InterfaceConfig.h"
#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

//  These includes are for the serial port reading/writing
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <glm/glm.hpp>

#include "SerialInterface.h"
#include "Field.h"
#include "world.h"
#include "Util.h"
#include "Audio.h"
#include "Head.h"
#include "Hand.h"
#include "Particle.h"
#include "Texture.h"
#include "Cloud.h"
#include "Agent.h"
#include "Cube.h"
#include "Lattice.h"
#include "Finger.h"

using namespace std;

int audio_on = 1;                   //  Whether to turn on the audio support
int simulate_on = 1; 

//
//  Network Socket and network constants
//

const int MAX_PACKET_SIZE = 1500;
const int AGENT_UDP_PORT = 40103;
char DOMAINSERVER_IP[] = "127.0.0.1";
const int DOMAINSERVER_PORT = 40102;
UDPSocket agentSocket(AGENT_UDP_PORT);

//  For testing, add milliseconds of delay for received UDP packets
char* incoming_packet;

int packetcount = 0;
int packets_per_second = 0; 
int bytes_per_second = 0;
int bytescount = 0;

//  Getting a target location from other machine (or loopback) to display
int target_x, target_y; 
int target_display = 0;

int head_mirror = 1;                 //  Whether to mirror own head when viewing it

int WIDTH = 1200; 
int HEIGHT = 800; 
int fullscreen = 0; 

#define HAND_RADIUS 0.25            //  Radius of in-world 'hand' of you
Head myHead;                        //  The rendered head of oneself or others
Head dummyHead;                     //  Test Head to render
int showDummyHead = 0;

Hand myHand(HAND_RADIUS, 
            glm::vec3(0,1,1));      //  My hand (used to manipulate things in world)

glm::vec3 box(WORLD_SIZE,WORLD_SIZE,WORLD_SIZE);
ParticleSystem balls(0, 
                     box, 
                     false,                     //  Wrap?
                     0.02,                      //  Noise
                     0.3,                       //  Size scale 
                     0.0                        //  Gravity 
                     );

Cloud cloud(0,                             //  Particles
            box,                           //  Bounding Box
            false                          //  Wrap
            );

VoxelSystem voxels(0, box);

Lattice lattice(160,100);
Finger myFinger(WIDTH, HEIGHT);
Field field;

#define RENDER_FRAME_MSECS 8
#define SLEEP 0
int steps_per_frame = 0;

float yaw =0.f;                         //  The yaw, pitch for the avatar head
float pitch = 0.f;                      //      
float start_yaw = 122;
float render_pitch = 0.f;
float render_yaw_rate = 0.f;
float render_pitch_rate = 0.f; 
float lateral_vel = 0.f;

// Manage speed and direction of motion
GLfloat fwd_vec[] = {0.0, 0.0, 1.0};
//GLfloat start_location[] = { WORLD_SIZE*1.5, -WORLD_SIZE/2.0, -WORLD_SIZE/3.0};
//GLfloat start_location[] = { 0.1, -0.15, 0.1};

GLfloat start_location[] = {6.1, 0, 1.4};

GLfloat location[] = {start_location[0], start_location[1], start_location[2]};
float fwd_vel = 0.0f;


#define MAX_FILE_CHARS 100000		//  Biggest file size that can be read to the system

int stats_on = 0;					//  Whether to show onscreen text overlay with stats

int noise_on = 0;					//  Whether to add random noise 
float noise = 1.0;                  //  Overall magnitude scaling for random noise levels 

int step_on = 0;                    
int display_levels = 0;
int display_head = 0;
int display_hand = 0;
int display_field = 0;

int display_head_mouse = 1;         //  Display sample mouse pointer controlled by head movement
int head_mouse_x, head_mouse_y; 
int head_lean_x, head_lean_y;

int mouse_x, mouse_y;				//  Where is the mouse 
int mouse_pressed = 0;				//  true if mouse has been pressed (clear when finished)

int nearbyAgents = 0;               //  How many other people near you is the domain server reporting?

int speed;

//  
//  Serial USB Variables
// 

SerialInterface serialPort;
char serial_portname[] = "/dev/tty.usbmodem411";

int serial_on = 0; 
int latency_display = 1;
//int adc_channels[NUM_CHANNELS];
//float avg_adc_channels[NUM_CHANNELS];
//int sensor_samples = 0;
//int sensor_LED = 0;

glm::vec3 gravity;
int first_measurement = 1;
//int samplecount = 0;

//  Frame rate Measurement

int framecount = 0;                  
float FPS = 120.f;
timeval timer_start, timer_end;
timeval last_frame;
double elapsedTime;

// Particles

// To add a new texture:
// 1. Add to the XCode project in the Resources/images group
//    (ensure "Copy file" is checked
// 2. Add to the "Copy files" build phase in the project
char texture_filename[] = "./int-texture256-v4.png";
unsigned int texture_width = 256;
unsigned int texture_height = 256;


float particle_attenuation_quadratic[] =  { 0.0f, 0.0f, 2.0f }; // larger Z = smaller particles
float pointer_attenuation_quadratic[] =  { 1.0f, 0.0f, 0.0f }; // for 2D view



#ifdef MARKER_CAPTURE

    /***  Marker Capture ***/
    #define MARKER_CAPTURE_INTERVAL 1
    MarkerCapture marker_capturer(CV_CAP_ANY); // Create a new marker capturer, attached to any valid camera.
    MarkerAcquisitionView marker_acq_view(&marker_capturer);
    bool marker_capture_enabled = true;
    bool marker_capture_display = true;
    IplImage* marker_capture_frame;
    IplImage* marker_capture_blob_frame;
    pthread_mutex_t frame_lock;

#endif


//  Every second, check the frame rates and other stuff
void Timer(int extra)
{
    gettimeofday(&timer_end, NULL);
    FPS = (float)framecount / ((float)diffclock(&timer_start, &timer_end) / 1000.f);
    packets_per_second = (float)packetcount / ((float)diffclock(&timer_start, &timer_end) / 1000.f);
    bytes_per_second = (float)bytescount / ((float)diffclock(&timer_start, &timer_end) / 1000.f);
   	framecount = 0;
    packetcount = 0;
    bytescount = 0;
    
	glutTimerFunc(1000,Timer,0);
    gettimeofday(&timer_start, NULL);
    
    //  Send a message to the domainserver telling it we are ALIVE
    char output[100];
    sprintf(output, "%f,%f,%f", location[0], location[1], location[2]);
    int packet_size = strlen(output);
    agentSocket.send(DOMAINSERVER_IP, DOMAINSERVER_PORT, output, packet_size);
    
    //  Send a message to ourselves
    //char test[]="T";
    //agentSocket.send("127.0.0.1", AGENT_UDP_PORT, test, 1);
}

void display_stats(void)
{
	//  bitmap chars are about 10 pels high 
    char legend[] = "/ - toggle this display, Q - exit, H - show head, M - show hand, T - test audio";
    drawtext(10, 15, 0.10, 0, 1.0, 0, legend);
    
    char stats[200];
    sprintf(stats, "FPS = %3.0f,  Pkts/s = %d, Bytes/s = %d", 
            FPS, packets_per_second,  bytes_per_second);
    drawtext(10, 30, 0.10, 0, 1.0, 0, stats); 
    if (serial_on) {
        sprintf(stats, "ADC samples = %d, LED = %d", 
                serialPort.getNumSamples(), serialPort.getLED());
        drawtext(500, 30, 0.10, 0, 1.0, 0, stats);
    }
    
    char adc[200];
	sprintf(adc, "location = %3.1f,%3.1f,%3.1f, angle_to(origin) = %3.1f, head yaw = %3.1f, render_yaw = %3.1f",
            -location[0], -location[1], -location[2],
            angle_to(myHead.getPos()*-1.f, glm::vec3(0,0,0), myHead.getRenderYaw(), myHead.getYaw()),
            myHead.getYaw(), myHead.getRenderYaw());
    drawtext(10, 50, 0.10, 0, 1.0, 0, adc);
    
    char agents[100];
    sprintf(agents, "Nearby: %d\n", nearbyAgents);
    drawtext(WIDTH-200,20, 0.10, 0, 1.0, 0, agents, 1, 1, 0);
	
}

void initDisplay(void)
{
    //  Set up blending function so that we can NOT clear the display
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel (GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    
    load_png_as_texture(texture_filename);

    if (fullscreen) glutFullScreen();
}

void init(void)
{
    myHead.setRenderYaw(start_yaw);
    
    dummyHead.setPitch(0);
    dummyHead.setRoll(0);
    dummyHead.setYaw(0);
    dummyHead.setScale(0.25); 
    
    dummyHead.setPos(glm::vec3(0,0,0));
    
    if (audio_on) {
        if (serial_on) {
            Audio::init(&myHead);
        } else {
            Audio::init();
        }
        printf( "Audio started.\n" );
    }

    head_mouse_x = WIDTH/2;
    head_mouse_y = HEIGHT/2; 
    head_lean_x = WIDTH/2;
    head_lean_y = HEIGHT/2;
    
    //  Initialize Field values
    field = Field();
    printf( "Field Initialized.\n" );

    if (noise_on) 
    {   
        myHand.setNoise(noise);
        myHead.setNoise(noise);
        dummyHead.setNoise(noise);
    }
    
    if (serial_on)
    {
        //  Call readsensors for a while to get stable initial values on sensors    
        printf( "Stabilizing sensors... " );
        gettimeofday(&timer_start, NULL);
        int done = 0;
        while (!done)
        {
            serialPort.readData();
            gettimeofday(&timer_end, NULL);
            if (diffclock(&timer_start, &timer_end) > 1000) done = 1;
        }
        gravity.x = serialPort.getValue(ACCEL_X);
        gravity.y = serialPort.getValue(ACCEL_Y);
        gravity.z = serialPort.getValue(ACCEL_Z);
        
        std::cout << "Gravity:  " << gravity.x << "," << gravity.y << "," << gravity.z << "\n";
        printf( "Done.\n" );

    }
    
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
    
    
    gettimeofday(&timer_start, NULL);
    gettimeofday(&last_frame, NULL);
}

void terminate () {
    // Close serial port
    //close(serial_fd);

    if (audio_on) { 
        Audio::terminate();
    }
    exit(EXIT_SUCCESS);
}

void reset_sensors()
{
    //  
    //   Reset serial I/O sensors 
    // 
    myHead.setRenderYaw(start_yaw);
    dummyHead.setRenderPitch(0);
    dummyHead.setRenderYaw(0);
    
    yaw = render_yaw_rate = 0; 
    pitch = render_pitch = render_pitch_rate = 0;
    lateral_vel = 0;
    location[0] = start_location[0];
    location[1] = start_location[1];
    location[2] = start_location[2];
    fwd_vel = 0.0;
    head_mouse_x = WIDTH/2;
    head_mouse_y = HEIGHT/2; 
    head_lean_x = WIDTH/2;
    head_lean_y = HEIGHT/2; 
    
    myHead.reset();
    myHand.reset();
    if (serial_on) serialPort.resetTrailingAverages();
}

void update_pos(float frametime)
//  Using serial data, update avatar/render position and angles
{
    float measured_pitch_rate = serialPort.getRelativeValue(PITCH_RATE);
    float measured_yaw_rate = serialPort.getRelativeValue(YAW_RATE);
    //float measured_lateral_accel = serialPort.getRelativeValue(ACCEL_X);
    //float measured_fwd_accel = serialPort.getRelativeValue(ACCEL_Z);
    
    myHead.UpdatePos(frametime, &serialPort, head_mirror, &gravity);
    
    //  Update head_mouse model 
    const float MIN_MOUSE_RATE = 30.0;
    const float MOUSE_SENSITIVITY = 0.1;
    if (powf(measured_yaw_rate*measured_yaw_rate + 
             measured_pitch_rate*measured_pitch_rate, 0.5) > MIN_MOUSE_RATE)
    {
        head_mouse_x -= measured_yaw_rate*MOUSE_SENSITIVITY;
        head_mouse_y += measured_pitch_rate*MOUSE_SENSITIVITY*(float)HEIGHT/(float)WIDTH; 
    }
    head_mouse_x = max(head_mouse_x, 0);
    head_mouse_x = min(head_mouse_x, WIDTH);
    head_mouse_y = max(head_mouse_y, 0);
    head_mouse_y = min(head_mouse_y, HEIGHT);
    
    //  Update hand/manipulator location for measured forces from serial channel
    /*
    const float MIN_HAND_ACCEL = 30.0;
    const float HAND_FORCE_SCALE = 0.5;
    glm::vec3 hand_accel(-(avg_adc_channels[6] - adc_channels[6]),
                         -(avg_adc_channels[7] - adc_channels[7]),
                         -(avg_adc_channels[5] - adc_channels[5]));
    
    if (glm::length(hand_accel) > MIN_HAND_ACCEL)
    {
        myHand.addVel(frametime*hand_accel*HAND_FORCE_SCALE);
    }
    */
                       
    //  Update render direction (pitch/yaw) based on measured gyro rates
    const int MIN_YAW_RATE = 3000;
    const float YAW_SENSITIVITY = 0.03;
    const int MIN_PITCH_RATE = 3000;
    
    const float PITCH_SENSITIVITY = 0.04;
    
    if (fabs(measured_yaw_rate) > MIN_YAW_RATE)  
    {   
        if (measured_yaw_rate > 0)
            render_yaw_rate -= (measured_yaw_rate - MIN_YAW_RATE) * YAW_SENSITIVITY * frametime;
        else 
            render_yaw_rate -= (measured_yaw_rate + MIN_YAW_RATE) * YAW_SENSITIVITY * frametime;
    }
    if (fabs(measured_pitch_rate) > MIN_PITCH_RATE) 
    {
        if (measured_pitch_rate > 0)
            render_pitch_rate += (measured_pitch_rate - MIN_PITCH_RATE) * PITCH_SENSITIVITY * frametime;
        else 
            render_pitch_rate += (measured_pitch_rate + MIN_PITCH_RATE) * PITCH_SENSITIVITY * frametime;
    }
         
    render_pitch += render_pitch_rate;
    
    // Decay render_pitch toward zero because we never look constantly up/down 
    render_pitch *= (1.f - 2.0*frametime);

    //  Decay angular rates toward zero 
    render_pitch_rate *= (1.f - 5.0*frametime);
    render_yaw_rate *= (1.f - 7.0*frametime);
    
    //  Update slide left/right based on accelerometer reading
    /*
    const int MIN_LATERAL_ACCEL = 20;
    const float LATERAL_SENSITIVITY = 0.001;
    if (fabs(measured_lateral_accel) > MIN_LATERAL_ACCEL) 
    {
        if (measured_lateral_accel > 0)
            lateral_vel += (measured_lateral_accel - MIN_LATERAL_ACCEL) * LATERAL_SENSITIVITY * frametime;
        else 
            lateral_vel += (measured_lateral_accel + MIN_LATERAL_ACCEL) * LATERAL_SENSITIVITY * frametime;
    }*/
 
    //slide += lateral_vel;
    lateral_vel *= (1.f - 4.0*frametime);
    
    //  Update fwd/back based on accelerometer reading
    /*
    const int MIN_FWD_ACCEL = 20;
    const float FWD_SENSITIVITY = 0.001;
    
    if (fabs(measured_fwd_accel) > MIN_FWD_ACCEL) 
    {
        if (measured_fwd_accel > 0)
            fwd_vel += (measured_fwd_accel - MIN_FWD_ACCEL) * FWD_SENSITIVITY * frametime;
        else 
            fwd_vel += (measured_fwd_accel + MIN_FWD_ACCEL) * FWD_SENSITIVITY * frametime;

    }*/
    //  Decrease forward velocity
    fwd_vel *= (1.f - 4.0*frametime);
    

    //  Update forward vector based on pitch and yaw 
    fwd_vec[0] = -sinf(myHead.getRenderYaw()*PI/180);
    fwd_vec[1] = sinf(render_pitch*PI/180);
    fwd_vec[2] = cosf(myHead.getRenderYaw()*PI/180);
    
    //  Advance location forward
    location[0] += fwd_vec[0]*fwd_vel;
    location[1] += fwd_vec[1]*fwd_vel;
    location[2] += fwd_vec[2]*fwd_vel;
    
    //  Slide location sideways
    location[0] += fwd_vec[2]*-lateral_vel;
    location[2] += fwd_vec[0]*lateral_vel;
    
    //  Update manipulator objects with object with current location
    balls.updateHand(myHead.getPos() + myHand.getPos(), glm::vec3(0,0,0), myHand.getRadius());
    
    //  Update own head data
    myHead.setRenderYaw(myHead.getRenderYaw() + render_yaw_rate);
    myHead.setRenderPitch(render_pitch);
    myHead.setPos(glm::vec3(location[0], location[1], location[2]));
    
    //  Get audio loudness data from audio input device
    float loudness, averageLoudness;
    Audio::getInputLoudness(&loudness, &averageLoudness);
    myHead.setLoudness(loudness);
    myHead.setAverageLoudness(averageLoudness);

    //  Send my streaming head data to agents that are nearby and need to see it!
    
    const int MAX_BROADCAST_STRING = 200;
    char broadcast_string[MAX_BROADCAST_STRING];
    int broadcast_bytes = myHead.getBroadcastData(broadcast_string);
    broadcast_to_agents(&agentSocket, broadcast_string, broadcast_bytes);

    //printf("-> %s\n", broadcast_string);
    //dummyHead.recvBroadcastData(broadcast_string, broadcast_bytes);
    //printf("head bytes:  %d\n", broadcast_bytes);
    
}

int render_test_spot = WIDTH/2;
int render_test_direction = 1; 

void display(void)
{
    glEnable (GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LINE_SMOOTH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
        glLoadIdentity();
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        
        GLfloat light_position0[] = { 1.0, 1.0, 0.0, 0.0 };
        glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
        GLfloat ambient_color[] = { 0.125, 0.305, 0.5 };  
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color);
        GLfloat diffuse_color[] = { 0.5, 0.42, 0.33 };
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color);
        GLfloat specular_color[] = { 1.0, 1.0, 1.0, 1.0};
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular_color);
        
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular_color);
        glMateriali(GL_FRONT, GL_SHININESS, 96);
           
        //  Rotate, translate to camera location
        glRotatef(myHead.getRenderPitch(), 1, 0, 0);
        glRotatef(myHead.getRenderYaw(), 0, 1, 0);
        glTranslatef(location[0], location[1], location[2]);
    
        //  Draw cloud of dots
        glDisable( GL_POINT_SPRITE_ARB );
        glDisable( GL_TEXTURE_2D );
        if (!display_head) cloud.render();
    
        //  Draw voxels
        voxels.render();
    
        //  Draw field vectors
        if (display_field) field.render();
        
        //  Render my own head 
        if (display_head) {
            glPushMatrix();
            glLoadIdentity();
            glTranslatef(0.f, 0.f, -7.f);
            myHead.render(1);
            glPopMatrix();
        }
    
        //  Render dummy head
        if (showDummyHead) dummyHead.render(0);
    
        //  Render heads of other agents 
        if (!display_head) render_agents();
        
        if (display_hand) myHand.render();   
     
        if (!display_head) balls.render();
    
        //  Render the world box 
        if (!display_head && stats_on) render_world_box();
    
        // render audio sources and start them
        if (audio_on) {
            Audio::render();
        }
    
        //glm::vec3 test(0.5, 0.5, 0.5); 
        //render_vector(&test);

    glPopMatrix();

    //  Render 2D overlay:  I/O level bar graphs and text  
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity(); 
        gluOrtho2D(0, WIDTH, HEIGHT, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        //lattice.render(WIDTH, HEIGHT);
        Audio::render(WIDTH, HEIGHT);

        //drawvec3(100, 100, 0.15, 0, 1.0, 0, myHead.getPos(), 0, 1, 0);
        glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, pointer_attenuation_quadratic );

//        myFinger.render();

        if (mouse_pressed == 1)
        {
            glPointSize( 10.0f );
            glColor3f(1,1,1);
            //glEnable(GL_POINT_SMOOTH);
            glBegin(GL_POINTS);
            glVertex2f(target_x, target_y);
            glEnd();
            char val[20];
            sprintf(val, "%d,%d", target_x, target_y); 
            drawtext(target_x, target_y-20, 0.08, 0, 1.0, 0, val, 0, 1, 0);
        }
        if (display_head_mouse && !display_head && stats_on)
        {
            glPointSize(10.0f);
            glColor4f(1.0, 1.0, 0.0, 0.8);
            glEnable(GL_POINT_SMOOTH);
            glBegin(GL_POINTS);
            glVertex2f(head_mouse_x, head_mouse_y);
            glEnd();
        }
        //  Spot bouncing back and forth on bottom of screen
        if (0)
        {
            glPointSize(50.0f);
            glColor4f(1.0, 1.0, 1.0, 1.0);
            glEnable(GL_POINT_SMOOTH);
            glBegin(GL_POINTS);
            glVertex2f(render_test_spot, HEIGHT-100);
            glEnd(); 
            render_test_spot += render_test_direction*50; 
            if ((render_test_spot > WIDTH-100) || (render_test_spot < 100)) render_test_direction *= -1.0;
            
        }
        
    //  Show detected levels from the serial I/O ADC channel sensors
    if (display_levels) serialPort.renderLevels(WIDTH,HEIGHT);
    
    //  Display miscellaneous text stats onscreen
    if (stats_on) display_stats();

#ifdef MARKER_CAPTURE
        /* Render marker acquisition stuff */
        pthread_mutex_lock(&frame_lock);
        if(marker_capture_frame){
            marker_acq_view.render(marker_capture_frame); // render the acquisition view, if it's visible.
        }
        // Draw marker images, if requested.
        if (marker_capture_enabled && marker_capture_display && marker_capture_frame){
            marker_capturer.glDrawIplImage(marker_capture_frame, WIDTH - 140, 10, 0.1, -0.1);
            marker_capturer.glDrawIplImage(marker_capture_blob_frame, WIDTH - 280, 10, 0.1, -0.1);
        }
        pthread_mutex_unlock(&frame_lock);
        /* Done rendering marker acquisition stuff */
#endif
    
    glPopMatrix();
    
    glutSwapBuffers();
    framecount++;
}
void specialkey(int k, int x, int y)
{
    if (k == GLUT_KEY_UP) fwd_vel += 0.05;
    if (k == GLUT_KEY_DOWN) fwd_vel -= 0.05;
    if (k == GLUT_KEY_LEFT) {
        if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) lateral_vel -= 0.02;
            else render_yaw_rate -= 0.25;
    }
    if (k == GLUT_KEY_RIGHT) {
        if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) lateral_vel += 0.02;
        else render_yaw_rate += 0.25;        
    }
    
}
void key(unsigned char k, int x, int y)
{
    
	//  Process keypresses 
 	if (k == 'q')  ::terminate();
    
    // marker capture
#ifdef MARKER_CAPTURE
    if(k == 'x'){
        printf("Toggling marker capture display.\n");
        marker_capture_display = !marker_capture_display;
        marker_capture_display ? marker_acq_view.show() : marker_acq_view.hide();
    }
	
    // delegate keypresses to the marker acquisition view when it's visible;
    // override other key mappings by returning...
    if(marker_acq_view.visible){
        marker_acq_view.handle_key(k);
        return;
    }
#endif
    
    if (k == '/')  stats_on = !stats_on;		// toggle stats
	if (k == 'n') 
    {
        noise_on = !noise_on;                   // Toggle noise 
        if (noise_on)
        {
            myHand.setNoise(noise);
            myHead.setNoise(noise);
        }
        else 
        {
            myHand.setNoise(0);
            myHead.setNoise(0);
        }

    }
    if (k == 'h') display_head = !display_head;
    if (k == 'b') display_hand = !display_hand;
    if (k == 'm') head_mirror = !head_mirror;
    
    if (k == 'f') display_field = !display_field;
    if (k == 'l') display_levels = !display_levels;
    
    
    if (k == 'e') location[1] -= WORLD_SIZE/100.0;
    if (k == 'c') location[1] += WORLD_SIZE/100.0;
    if (k == 'w') fwd_vel += 0.05;
    if (k == 's') fwd_vel -= 0.05;
    if (k == ' ') reset_sensors();
    if (k == 'a') render_yaw_rate -= 0.25;
    if (k == 'd') render_yaw_rate += 0.25;
    if (k == 'o') simulate_on = !simulate_on;
    if (k == 'p') 
    {
        // Add to field vector 
        float pos[] = {5,5,5};
        float add[] = {0.001, 0.001, 0.001};
        field.add(add, pos);
    }
    if (k == '1')
    {
        myHead.SetNewHeadTarget((randFloat()-0.5)*20.0, (randFloat()-0.5)*20.0);
    }
}

//
//  Receive packets from other agents/servers and decide what to do with them!
//
void read_network()
{
    sockaddr_in senderAddress;
    int bytes_recvd;
    agentSocket.receive(&senderAddress, (void *)incoming_packet, &bytes_recvd);
    
    if (bytes_recvd > 0)
    {
        packetcount++;
        bytescount += bytes_recvd;
        //  If packet is a Mouse data packet, copy it over 
        if (incoming_packet[0] == 'M') {
            // 
            //  mouse location packet 
            //
            sscanf(incoming_packet, "M %d %d", &target_x, &target_y);
            target_display = 1;
            //printf("X = %d Y = %d\n", target_x, target_y);
        } else if (incoming_packet[0] == 'D') {
            //
            //  Message from domainserver
            //
            //printf("agent list received!\n");
            nearbyAgents = update_agents(&incoming_packet[1], bytes_recvd - 1);
        } else if (incoming_packet[0] == 'H') {
            //
            //  Broadcast packet from another agent 
            //
            //printf("Got broadcast!\n");
            update_agent(inet_ntoa(senderAddress.sin_addr), senderAddress.sin_port,  &incoming_packet[1], bytes_recvd - 1);
        } else if (incoming_packet[0] == 'T') {
            printf("Got test!  From port %d\n", senderAddress.sin_port);
        }
    }
}

void idle(void)
{
    timeval check;
    gettimeofday(&check, NULL);
    
    //  Check and render display frame 
    if (diffclock(&last_frame, &check) > RENDER_FRAME_MSECS)
    {
        steps_per_frame++;
        //  Simulation
        update_pos(1.f/FPS);
        if (simulate_on) {
            field.simulate(1.f/FPS);
            myHead.simulate(1.f/FPS);
            myHand.simulate(1.f/FPS);
            balls.simulate(1.f/FPS);
            cloud.simulate(1.f/FPS);
            lattice.simulate(1.f/FPS);
            myFinger.simulate(1.f/FPS);
        }

        if (!step_on) glutPostRedisplay();
        last_frame = check;
        
    }
    
    //  Read network packets
    read_network();
    //  Read serial data 
    if (serial_on) serialPort.readData();
    if (SLEEP)
    {
        usleep(SLEEP);
    }
}

void reshape(int width, int height)
{
    WIDTH = width;
    HEIGHT = height; 
    
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION); //hello
    glLoadIdentity();
    gluPerspective(45, //view angle
                   1.0, //aspect ratio
                   0.1, //near clip
                   50.0);//far clip
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

     
}

void mouseFunc( int button, int state, int x, int y ) 
{
    if( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
		mouse_x = x;
		mouse_y = y;
		mouse_pressed = 1;
//        lattice.mouseClick((float)x/(float)WIDTH,(float)y/(float)HEIGHT);
    }
	if( button == GLUT_LEFT_BUTTON && state == GLUT_UP )
    {
		mouse_x = x;
		mouse_y = y;
		mouse_pressed = 0;
    }
	
}

void motionFunc( int x, int y)
{
	mouse_x = x;
	mouse_y = y;
    if (mouse_pressed == 1)
    {
        //  Send network packet containing mouse location
        char mouse_string[20];
        sprintf(mouse_string, "M %d %d\n", mouse_x, mouse_y);
        //network_send(UDP_socket, mouse_string, strlen(mouse_string));
    }
    
    lattice.mouseClick((float)x/(float)WIDTH,(float)y/(float)HEIGHT);
	
}

void mouseoverFunc( int x, int y)
{
	mouse_x = x;
	mouse_y = y;
    if (mouse_pressed == 0)
    {
        lattice.mouseOver((float)x/(float)WIDTH,(float)y/(float)HEIGHT);
        myFinger.setTarget(mouse_x, mouse_y);
    }
}


#ifdef MARKER_CAPTURE
void *poll_marker_capture(void *threadarg){
    while(1){
        marker_capturer.tick();
    }
}
#endif

int main(int argc, char** argv)
{
    //  Create network socket and buffer
    incoming_packet = new char[MAX_PACKET_SIZE];
    
    //
    printf("Testing math... standard deviation.\n");
    StDev stdevtest;
    stdevtest.reset();
    stdevtest.addValue(1345);
    stdevtest.addValue(1301);
    stdevtest.addValue(1368);
    stdevtest.addValue(1322);
    stdevtest.addValue(1310);
    stdevtest.addValue(1370);
    stdevtest.addValue(1318);
    stdevtest.addValue(1350);
    stdevtest.addValue(1303);
    stdevtest.addValue(1299);
    
    if (stdevtest.getSamples() == 10) 
        printf("Samples=PASS ");
    else
        printf("Samples=FAIL ");
        
    if (floor(stdevtest.getAverage()*100.0) == 132859.0)
        printf("Average=PASS ");
    else
        printf("Average=FAIL, avg reported = %5.3f ", floor(stdevtest.getAverage()*100.0));

    if (floor(stdevtest.getStDev()*100.0) == 2746.0)
        printf("Stdev=PASS \n");
    else
        printf("Stdev=FAIL \n");

    //
    //  Try to connect the serial port I/O
    //
    if(serialPort.init(serial_portname, 115200) == -1) {
        printf("Unable to open serial port: %s\n", serial_portname);
        serial_on = 0;
    } 
    else 
    {
        printf("Serial Port Initialized\n");
        serial_on = 1; 
    }


    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Interface");
    
    printf( "Created Display Window.\n" );
    
    initDisplay();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
    glutSpecialFunc(specialkey);
	glutMotionFunc(motionFunc);
    glutPassiveMotionFunc(mouseoverFunc);
	glutMouseFunc(mouseFunc);
    glutIdleFunc(idle);
	
    printf( "Initialized Display.\n" );

    init();
    
    printf( "Init() complete.\n" );
    
    glutTimerFunc(1000, Timer, 0);
    
#ifdef MARKER_CAPTURE
    
    if (marker_capture_enabled) {
        // start capture thread
        if (pthread_mutex_init(&frame_lock, NULL) != 0){
            printf("Frame lock mutext init failed. Exiting.\n");
            return 1;
        }
        pthread_t capture_thread;
        pthread_create(&capture_thread, NULL, poll_marker_capture, NULL);
    }
#endif
    
    glutMainLoop();
    
#ifdef MARKER_CAPTURE
    if (marker_capture_enabled) {
        pthread_mutex_destroy(&frame_lock);
        pthread_exit(NULL);
    }
#endif
    
    ::terminate();
    return EXIT_SUCCESS;
}   

