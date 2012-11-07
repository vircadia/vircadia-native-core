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
//  n = toggle noise in firing on/off
//  c = clear all cells and synapses to zero
//  s = clear cells to zero but preserve synapse weights
//

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/glut.h>
#endif
#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

//  These includes are for the serial port reading/writing
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "tga.h"                //  Texture loader library
#include "glm/glm.hpp"
#include <portaudio.h>

#include "SerialInterface.h"
#include "field.h"
#include "world.h"
#include "util.h"
#include "network.h"
#include "audio.h"
#include "head.h"
#include "hand.h"
#include "particle.h"

#include "texture.h"

//TGAImg Img;

using namespace std;

//   Junk for talking to the Serial Port 
int serial_on = 0;                  //  Is serial connection on/off?  System will try

//  Network Socket Stuff 
//  For testing, add milliseconds of delay for received UDP packets
int UDP_socket;
int delay = 0;         
char* incoming_packet;
timeval ping_start;
int ping_count = 0;
float ping_msecs = 0.0;  
int packetcount = 0;
int packets_per_second = 0; 
int bytes_per_second = 0;
int bytescount = 0;

//  Getting a target location from other machine (or loopback) to display
int target_x, target_y; 
int target_display = 0;

unsigned char last_key = 0; 

double ping = 0; 



//#define WIDTH 1200					//  Width,Height of simulation area in cells
//#define HEIGHT 800
int WIDTH = 1200; 
int HEIGHT = 800; 

#define BOTTOM_MARGIN 0				
#define RIGHT_MARGIN 0

#define HAND_RADIUS 0.25             //  Radius of in-world 'hand' of you
Head myHead;                        //  The rendered head of oneself or others 
Hand myHand(HAND_RADIUS, 
            glm::vec3(0,1,1));      //  My hand (used to manipulate things in world)

glm::vec3 box(WORLD_SIZE,WORLD_SIZE,WORLD_SIZE);
ParticleSystem balls(500, 
                     box, 
                     false,                     // Wrap?
                     0.0,                       // Noise
                     0.3,                        //  Size scale 
                     0.0                       // Gravity 
                     );



//  FIELD INFORMATION 
//  If the simulation 'world' is a box with 10M boundaries, the offset to a field cell is given by:
//  element = [x/10 + (y/10)*10 + (z*/10)*100] 
//
//  The vec(x,y,z) corner of a field cell at element i is:
// 
//  z = (int)( i / 100)
//  y = (int)(i % 100 / 10)
//  x = (int)(i % 10)

#define RENDER_FRAME_MSECS 10
#define SLEEP 0

#define NUM_TRIS 1000 * 100  // 20000   //000
struct {
    float vertices[NUM_TRIS * 3];
    float normals [NUM_TRIS * 3];
    float colors  [NUM_TRIS * 3];
    float vel     [NUM_TRIS * 3];
    glm::vec3 vel1[NUM_TRIS];
    glm::vec3 vel2[NUM_TRIS];
    int element[NUM_TRIS];
}tris;

float twiddles[NUM_TRIS * 3];

float yaw =0.f;                         //  The yaw, pitch for the avatar head 
float pitch = 0.f;                      //      
float start_yaw = 90.0;
float render_yaw = start_yaw;
float render_pitch = 0.f;
float render_yaw_rate = 0.f;
float render_pitch_rate = 0.f; 
float lateral_vel = 0.f;

// Manage speed and direction of motion
GLfloat fwd_vec[] = { 0.0, 0.0, 1.0};
GLfloat start_location[] = { WORLD_SIZE*1.5, -WORLD_SIZE/2.0, -WORLD_SIZE/3.0};
GLfloat location[] = {start_location[0], start_location[1], start_location[2]};
float fwd_vel = 0.0f;


#define MAX_FILE_CHARS 100000		//  Biggest file size that can be read to the system

int stats_on = 1;					//  Whether to show onscreen text overlay with stats

int noise_on = 0;					//  Whether to add random noise 
float noise = 1.0;                  //  Overall magnitude scaling for random noise levels 

int step_on = 0;                    
int display_levels = 1;
int display_head = 0;
int display_field = 0;

int display_head_mouse = 1;              //  Display sample mouse pointer controlled by head movement
int head_mouse_x, head_mouse_y;     

int mouse_x, mouse_y;				//  Where is the mouse 
int mouse_pressed = 0;				//  true if mouse has been pressed (clear when finished)

int accel_x, accel_y;

int speed;

float mag_imbalance = 0.f;

//  
//  Serial I/O channel mapping:
//  
//  0   Head Gyro Pitch 
//  1   Head Gyro Yaw 
//  2   Head Accelerometer X
//  3   Head Accelerometer Z 
//  4   Hand Accelerometer X 
//  5   Hand Accelerometer Y
//  6   Hand Accelerometer Z 
// 

int adc_channels[NUM_CHANNELS];                
float avg_adc_channels[NUM_CHANNELS];
int first_measurement = 1;
int samplecount = 0;

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
char texture_filename[] = "grayson-particle.png";
//unsigned int texture_width = 256;
//unsigned int texture_height = 256;

float particle_attenuation_quadratic[] =  { 0.0f, 0.0f, 2.0f }; // larger Z = smaller particles



//  Every second, check the frame rates and other stuff
void Timer(int extra)
{
    gettimeofday(&timer_end, NULL);
    FPS = (float)framecount / ((float)diffclock(timer_start,timer_end) / 1000.f);
    packets_per_second = (float)packetcount / ((float)diffclock(timer_start,timer_end) / 1000.f);
    bytes_per_second = (float)bytescount / ((float)diffclock(timer_start,timer_end) / 1000.f);
   	framecount = 0;
    samplecount = 0; 
    packetcount = 0;
    bytescount = 0;
    
	glutTimerFunc(1000,Timer,0);
    gettimeofday(&timer_start, NULL);
}

void display_stats(void)
{
	//  bitmap chars are about 10 pels high 
    char legend[] = "/ - toggle this display, Q - exit, N - toggle noise, M - toggle map, T - test audio";
    drawtext(10, 15, 0.10, 0, 1.0, 0, legend);
    
    char stats[200];
    sprintf(stats, "FPS = %3.0f, Ping = %4.1f Packets/Sec = %d, Bytes/sec = %d", 
            FPS, ping_msecs, packets_per_second,  bytes_per_second);
    drawtext(10, 30, 0.10, 0, 1.0, 0, stats); 
    
    char adc[200];
	sprintf(adc, "pitch_rate = %i, yaw_rate = %i, accel_lat = %i, accel_fwd = %i, loc[0] = %3.1f loc[1] = %3.1f, loc[2] = %3.1f", 
            (int)(adc_channels[0] - avg_adc_channels[0]),
            (int)(adc_channels[1] - avg_adc_channels[1]),
            (int)(adc_channels[2] - avg_adc_channels[2]),
            (int)(adc_channels[3] - avg_adc_channels[3]),
            location[0], location[1], location[2] 
            );
    drawtext(10, 50, 0.10, 0, 1.0, 0, adc);
	
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
}

void init(void)
{
    int i, j; 

    Audio::init();
    printf( "Audio started.\n" );

    //  Clear serial channels 
    for (i = i; i < NUM_CHANNELS; i++)
    {
        adc_channels[i] = 0;
        avg_adc_channels[i] = 0.0;
    }

    head_mouse_x = WIDTH/2;
    head_mouse_y = HEIGHT/2; 
    
    //  Initialize Field values 
    field_init();
    printf( "Field Initialized.\n" );

    if (noise_on) 
    {   
        myHand.setNoise(noise);
        myHead.setNoise(noise);
    }
    
    //  Init particles
    float tri_scale, r;
    const float VEL_SCALE = 0.00;
    for (i = 0; i < NUM_TRIS; i++)
    {
        r = randFloat();
        if (r > .999) tri_scale = 0.7; 
        else if (r > 0.90) tri_scale = 0.1;
        else tri_scale = 0.05; 
        

        glm::vec3 pos (randFloat() * WORLD_SIZE,
                       randFloat() * WORLD_SIZE,
                       randFloat() * WORLD_SIZE);
        glm::vec3 verts[3];
        verts[j].x = pos.x + randFloat() * tri_scale - tri_scale/2.f;
        verts[j].y = pos.y + randFloat() * tri_scale - tri_scale/2.f;
        verts[j].z = pos.z + randFloat() * tri_scale - tri_scale/2.f;
        tris.vertices[i*3] = verts[j].x;
        tris.vertices[i*3 + 1] = verts[j].y;
        tris.vertices[i*3 + 2] = verts[j].z;
        // reuse pos for the normal
        glm::normalize((pos += glm::cross(verts[1] - verts[0], verts[2] - verts[0])));
        tris.normals[i*3] = pos.x;
        tris.normals[i*3+1] = pos.y;
        tris.normals[i*3+2] = pos.z;
        
        //  Decide what kind of element this particle is to be, color accordingly
        if (randFloat() < 0.10)
        {
            //  Fixed - blue
            tris.element[i] = 0;
            tris.colors[i*3] = 0.0;  tris.colors[i*3+1] = 0.0; tris.colors[i*3+2] = 1.0;
            tris.vel[i*3] = tris.vel[i*3+1] = tris.vel[i*3+2] = 0.0;
        }
        else
        {
            //  Moving - white
            tris.element[i] = 1;
            tris.colors[i*3] = 1.0;  tris.colors[i*3+1] = 1.0; tris.colors[i*3+2] = 1.0;
            tris.vel[i*3] = (randFloat() - 0.5)*VEL_SCALE;
            tris.vel[i*3+1] = (randFloat() - 0.5)*VEL_SCALE;
            tris.vel[i*3+2] = (randFloat() - 0.5)*VEL_SCALE;

        }
        
    }
    
    const float TWIDDLE_SCALE = 0.01;
    for (i = 0; i < NUM_TRIS; i++)
    {
        twiddles[i*3] = (randFloat() - 0.5)*TWIDDLE_SCALE;
        twiddles[i*3 + 1] = (randFloat() - 0.5)*TWIDDLE_SCALE;
        twiddles[i*3 + 2] = (randFloat() - 0.5)*TWIDDLE_SCALE;
    }
     
    if (serial_on)
    {
        //  Call readsensors for a while to get stable initial values on sensors    
        printf( "Stabilizing sensors... " );
        gettimeofday(&timer_start, NULL);
        read_sensors(1, &avg_adc_channels[0], &adc_channels[0]);
        int done = 0;
        while (!done)
        {
            read_sensors(0, &avg_adc_channels[0], &adc_channels[0]);
            gettimeofday(&timer_end, NULL);
            if (diffclock(timer_start,timer_end) > 1000) done = 1;
        }
        printf( "Done.\n" );

    }
    
    gettimeofday(&timer_start, NULL);
    gettimeofday(&last_frame, NULL);
}

void terminate () {
    // Close serial port
    //close(serial_fd);

    Audio::terminate();
    exit(EXIT_SUCCESS);
}

const float SCALE_SENSORS = 0.3f;
const float SCALE_X = 2.f;
const float SCALE_Y = 1.f;


void update_tris()
{
    int i;
    float field_val[3];
    
    for (i = 0; i < NUM_TRIS; i++)
    {
        if (tris.element[i] == 1)          //  If moving object, move and drag
        {
            // Update position
            tris.vertices[i*3+0] += tris.vel[i*3];
            tris.vertices[i*3+1] += tris.vel[i*3+1];
            tris.vertices[i*3+2] += tris.vel[i*3+2];
            
            // Add a little gravity 
            //tris.vel[i*3+1] -= 0.0001;
           
            const float DRAG = 0.99;
            // Drag:  Decay velocity
            tris.vel[i*3] *= DRAG;
            tris.vel[i*3+1] *= DRAG;
            tris.vel[i*3+2] *= DRAG;
        }
                 
        if (tris.element[i] == 1) 
        {
            // Read and add velocity from field 
            field_value(field_val, &tris.vertices[i*3]);
            tris.vel[i*3] += field_val[0];
            tris.vel[i*3+1] += field_val[1];
            tris.vel[i*3+2] += field_val[2];
        }

        // bounce at edge of world 
        // X-Direction
        if ((tris.vertices[i*3+0] > WORLD_SIZE) || (tris.vertices[i*3+0] < 0.0))
            tris.vel[i*3]*= -1.0;
        // Y-direction
        if ((tris.vertices[i*3+1] > WORLD_SIZE) || (tris.vertices[i*3+1] < 0.0))
        { 
            tris.vel[i*3+1]*= -1.0;
        }
        // Z-Direction
        if ((tris.vertices[i*3+2] > WORLD_SIZE) || (tris.vertices[i*3+2] < 0.0))
            tris.vel[i*3+2]*= -1.0;
    }
}

void reset_sensors()
{
    //  
    //   Reset serial I/O sensors 
    // 
    render_yaw = start_yaw;
    yaw = render_yaw_rate = 0; 
    pitch = render_pitch = render_pitch_rate = 0;
    lateral_vel = 0;
    location[0] = start_location[0];
    location[1] = start_location[1];
    location[2] = start_location[2];
    fwd_vel = 0.0;
    head_mouse_x = WIDTH/2;
    head_mouse_y = HEIGHT/2; 
    myHead.reset();
    myHand.reset();
    if (serial_on) read_sensors(1, &avg_adc_channels[0], &adc_channels[0]);
}

void update_pos(float frametime)
//  Using serial data, update avatar/render position and angles
{
    float measured_pitch_rate = adc_channels[0] - avg_adc_channels[0];
    float measured_yaw_rate = adc_channels[1] - avg_adc_channels[1];
    float measured_lateral_accel = adc_channels[2] - avg_adc_channels[2];
    float measured_fwd_accel = avg_adc_channels[3] - adc_channels[3];
    
    //  Update avatar head position based on measured gyro rates
    myHead.addYaw(measured_yaw_rate * 1.20 * frametime);
    myHead.addPitch(measured_pitch_rate * -1.0 * frametime);
    //  Decay avatar head back toward zero
    //pitch *= (1.f - 5.0*frametime); 
    //yaw *= (1.f - 7.0*frametime);

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
    const float MIN_HAND_ACCEL = 30.0;
    const float HAND_FORCE_SCALE = 0.5;
    glm::vec3 hand_accel(-(avg_adc_channels[6] - adc_channels[6]),
                         -(avg_adc_channels[7] - adc_channels[7]),
                         -(avg_adc_channels[5] - adc_channels[5]));
    
    if (glm::length(hand_accel) > MIN_HAND_ACCEL)
    {
        myHand.addVel(frametime*hand_accel*HAND_FORCE_SCALE);
    }
                       
    //  Update render direction (pitch/yaw) based on measured gyro rates
    const int MIN_YAW_RATE = 300;
    const float YAW_SENSITIVITY = 0.03;
    const int MIN_PITCH_RATE = 300;
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
    render_yaw += render_yaw_rate;
    render_pitch += render_pitch_rate;
    
    // Decay render_pitch toward zero because we never look constantly up/down 
    render_pitch *= (1.f - 2.0*frametime);

    //  Decay angular rates toward zero 
    render_pitch_rate *= (1.f - 5.0*frametime);
    render_yaw_rate *= (1.f - 7.0*frametime);
    
    //  Update slide left/right based on accelerometer reading
    const int MIN_LATERAL_ACCEL = 20;
    const float LATERAL_SENSITIVITY = 0.001;
    if (fabs(measured_lateral_accel) > MIN_LATERAL_ACCEL) 
    {
        if (measured_lateral_accel > 0)
            lateral_vel += (measured_lateral_accel - MIN_LATERAL_ACCEL) * LATERAL_SENSITIVITY * frametime;
        else 
            lateral_vel += (measured_lateral_accel + MIN_LATERAL_ACCEL) * LATERAL_SENSITIVITY * frametime;
    }
 
    //slide += lateral_vel;
    lateral_vel *= (1.f - 4.0*frametime);
    
    //  Update fwd/back based on accelerometer reading
    const int MIN_FWD_ACCEL = 20;
    const float FWD_SENSITIVITY = 0.001;
    
    if (fabs(measured_fwd_accel) > MIN_FWD_ACCEL) 
    {
        if (measured_fwd_accel > 0)
            fwd_vel += (measured_fwd_accel - MIN_FWD_ACCEL) * FWD_SENSITIVITY * frametime;
        else 
            fwd_vel += (measured_fwd_accel + MIN_FWD_ACCEL) * FWD_SENSITIVITY * frametime;

    }
    //  Decrease forward velocity
    fwd_vel *= (1.f - 4.0*frametime);

    //  Update forward vector based on pitch and yaw 
    fwd_vec[0] = -sinf(render_yaw*PI/180);
    fwd_vec[1] = sinf(render_pitch*PI/180);
    fwd_vec[2] = cosf(render_yaw*PI/180);
    
    //  Advance location forward
    location[0] += fwd_vec[0]*fwd_vel;
    location[1] += fwd_vec[1]*fwd_vel;
    location[2] += fwd_vec[2]*fwd_vel;
    
    //  Slide location sideways
    location[0] += fwd_vec[2]*-lateral_vel;
    location[2] += fwd_vec[0]*lateral_vel;
    
    //  Update head and manipulator objects with object with current location
    myHead.setPos(glm::vec3(location[0], location[1], location[2]));
    balls.updateHand(myHead.getPos() + myHand.getPos(), glm::vec3(0,0,0), myHand.getRadius());
}

void display(void)
{
    
    int i;
    
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
        glRotatef(render_pitch, 1, 0, 0);
        glRotatef(render_yaw, 0, 1, 0);
        glTranslatef(location[0], location[1], location[2]);

        //glEnable(GL_DEPTH_TEST);
        //  TEST:  Draw a reference object in world space coordinates! 
        //glPushMatrix();
        //    glTranslatef(1,0,0);
            //glTranslatef(myHead.getPos().x, myHead.getPos().y, myHead.getPos().z);
        //    glColor3f(1,0,0);
        //    glutSolidCube(0.4);
        //glPopMatrix();
    
        // TEST: Draw a textured square (Yoz)

        /* create a square on the XY
         note that OpenGL origin is at the lower left
         but texture origin is at upper left
         => it has to be mirrored  */
    
        int error = load_png_as_texture(texture_filename);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glNormal3f(0.0, 0.0, 1.0);
        glTexCoord2d(1, 1); glVertex3f(0.0, 0.0, 0.0);
        glTexCoord2d(1, 0); glVertex3f(0.0, 2.0, 0.0);
        glTexCoord2d(0, 0); glVertex3f(1.0, 2.0, 0.0);
        glTexCoord2d(0, 1); glVertex3f(1.0, 0.0, 0.0);
        glEnd();
        glDisable(GL_TEXTURE_2D);

    
        // Draw Point Sprites
    
        /* assuming you have setup a 32-bit RGBA texture with a legal name */
        glActiveTexture(GL_TEXTURE0);
        glEnable( GL_TEXTURE_2D );
        glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
        glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, particle_attenuation_quadratic );
        float maxSize = 0.0f;
        glGetFloatv( GL_POINT_SIZE_MAX_ARB, &maxSize );
        glPointSize( maxSize );
        glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, maxSize );
        // glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 0.001f );
        glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );
        glEnable( GL_POINT_SPRITE_ARB );
        glBegin( GL_POINTS );
        {
            for (i = 0; i < NUM_TRIS; i++)
            {
    //            glColor3f(tris.colors[i*3],
    //                      tris.colors[i*3+1],
    //                      tris.colors[i*3+2]);
    //            for (j = 0; j < 3; j++)
    //            {
    //                glVertex3f(tris.vertices[i*9 + j*3],
    //                           tris.vertices[i*9 + j*3 + 1],
    //                           tris.vertices[i*9 + j*3 + 2]);
    //            }
    //            glNormal3f(tris.normals[i*3],
    //                       tris.normals[i*3 + 1],
    //                       tris.normals[i*3 + 2]);
                glVertex3f(tris.vertices[i*3],
                           tris.vertices[i*3+1],
                           tris.vertices[i*3+2]);
                
            }
        }
        glEnd();
        glDisable( GL_TEXTURE_2D );
        glDisable( GL_POINT_SPRITE_ARB );

        //  Show field vectors
        if (display_field) field_render(); 
        
            
        
        // Display floating head in front of viewer
        if (display_head)
        {
            myHead.render();
        }
        myHand.render();
    
        // balls.render();
            
        //  Render the world box 
        render_world_box();

    glPopMatrix();

    //  Render 2D overlay:  I/O level bar graphs and text  
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity(); 
        gluOrtho2D(0, WIDTH, HEIGHT, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        drawvec3(100, 100, 0.15, 0, 1.0, 0, myHead.getPos(), 0, 1, 0);
    
        if (mouse_pressed == 1)
        {
            glPointSize(20.f);
            glColor3f(1,1,1);
            glEnable(GL_POINT_SMOOTH);
            glBegin(GL_POINTS);
            glVertex2f(target_x, target_y);
            glEnd();
            char val[20];
            sprintf(val, "%d,%d", target_x, target_y); 
            drawtext(target_x, target_y-20, 0.08, 0, 1.0, 0, val, 0, 1, 0);

        }
        if (display_head_mouse)
        {
            glPointSize(20.f);
            glColor4f(1.0, 1.0, 0.0, 0.8);
            glEnable(GL_POINT_SMOOTH);
            glBegin(GL_POINTS);
            glVertex2f(head_mouse_x, head_mouse_y);
            glEnd();
        }
        
        //  Show detected levels from the serial I/O ADC channel sensors
        if (display_levels)
        {
            int i;
            int disp_x = 10;
            const int GAP = 16;
            char val[10];
            for(i = 0; i < NUM_CHANNELS; i++)
            {
                //  Actual value 
                glColor4f(1, 1, 1, 1);
                glBegin(GL_LINES);
                    glVertex2f(disp_x, HEIGHT*0.95);
                    glVertex2f(disp_x, HEIGHT*(0.25 + 0.75f*adc_channels[i]/4096));
                glEnd();
                //  Trailing Average value 
                glColor4f(0, 0, 0.8, 1);
                glBegin(GL_LINES);
                    glVertex2f(disp_x + 2, HEIGHT*0.95);
                    glVertex2f(disp_x + 2, HEIGHT*(0.25 + 0.75f*avg_adc_channels[i]/4096));
                glEnd();

                sprintf(val, "%d", adc_channels[i]); 
                drawtext(disp_x-GAP/2, (HEIGHT*0.95)+2, 0.08, 90, 1.0, 0, val, 0, 1, 0);

                disp_x += GAP;
            }
        }

        if (stats_on) display_stats(); 
    
    glPopMatrix();
    
    glutSwapBuffers();
    framecount++;
}

void key(unsigned char k, int x, int y)
{
	//  Process keypresses 
        
    last_key = k;
	
	if (k == 'q')  ::terminate();
	if (k == '/')  stats_on = !stats_on;		// toggle stats
	if (k == 'n') 
    {
        noise_on = !noise_on;			// Toggle noise 
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
    if (k == 'f') display_field = !display_field;
    if (k == 'e') location[1] -= WORLD_SIZE/100.0;
    if (k == 'c') location[1] += WORLD_SIZE/100.0;
    if (k == 'w') fwd_vel += 0.05;
    if (k == 's') fwd_vel -= 0.05;
    if (k == ' ') reset_sensors();
    if (k == 'a') render_yaw_rate -= 0.25;
    if (k == 'd') render_yaw_rate += 0.25;
    if (k == 'p') 
    {
        // Add to field vector 
        float pos[] = {5,5,5};
        float add[] = {0.001, 0.001, 0.001};
        field_add(add, pos);
    }
    if (k == 't') {
        Audio::writeTone(0, 400, 1.0f, 0.5f);
    }
    if (k == '1')
    {
        myHead.SetNewHeadTarget((randFloat()-0.5)*20.0, (randFloat()-0.5)*20.0);
    }
}

void read_network()
{
    //  Receive packets 
    int bytes_recvd = network_receive(UDP_socket, incoming_packet, delay);
    if (bytes_recvd > 0)
    {
        packetcount++;
        bytescount += bytes_recvd;
        //  If packet is a Mouse data packet, copy it over 
        if (incoming_packet[0] == 'M') {
            sscanf(incoming_packet, "M %d %d", &target_x, &target_y);
            target_display = 1;
            printf("X = %d Y = %d\n", target_x, target_y);
        } else if (incoming_packet[0] == 'P') {
        //  Ping packet - check time and record
            timeval check; 
            gettimeofday(&check, NULL);
            ping_msecs = (float)diffclock(ping_start, check);

        }
    }
}

void idle(void)
{
    timeval check;
    gettimeofday(&check, NULL);
    
    //  Check and render display frame 
    if (diffclock(last_frame,check) > RENDER_FRAME_MSECS) 
    {
        //  Simulation
        update_pos(1.f/FPS); 
        update_tris();
        field_simulate(1.f/FPS);
        myHead.simulate(1.f/FPS);
        myHand.simulate(1.f/FPS);
        // balls.simulate(1.f/FPS);

        if (!step_on) glutPostRedisplay();
        last_frame = check;
        
        //  Every 30 frames or so, check ping time 
        ping_count++;
        if (ping_count >= 30) {
            ping_start = network_send_ping(UDP_socket);
            ping_count = 0;
        }
    }
    
    //  Read network packets
    read_network();
    //  Read serial data 
    if (serial_on) samplecount += read_sensors(0, &avg_adc_channels[0], &adc_channels[0]);
    
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
                   1.0, //near clip
                   200.0);//far clip
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
        network_send(UDP_socket, mouse_string, strlen(mouse_string));
    }
	
}

int main(int argc, char** argv)
{
    //  Create network socket and buffer
    UDP_socket = network_init(); 
    if (UDP_socket) printf( "Created UDP socket.\n" ); 
    incoming_packet = new char[MAX_PACKET_SIZE];

    //  Test network loopback
    char test_data[] = "Test!";
    int bytes_sent = network_send(UDP_socket, test_data, 5);
    if (bytes_sent) printf("%d bytes sent.", bytes_sent);
    int test_recv = network_receive(UDP_socket, incoming_packet, delay);
    printf("Received %i bytes\n", test_recv);
    
       //  Load textures 
    //Img.Load("/Users/philip/Downloads/galaxy1.tga");

    //
    //  Try to connect the serial port I/O
    //
    if(init_port(115200) == -1) {				
        perror("Unable to open serial port\n");
        serial_on = 0;
    } 
    else 
    {
        printf("Serial Port Initialized\n");
        serial_on = 1; 
    }


    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(RIGHT_MARGIN + WIDTH, BOTTOM_MARGIN + HEIGHT);
    glutCreateWindow("Interface");
    
    printf( "Created Display Window.\n" );
    
    initDisplay();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutMotionFunc(motionFunc);
	glutMouseFunc(mouseFunc);
    glutIdleFunc(idle);
	
    printf( "Initialized Display.\n" );

    init();
    
    printf( "Init() complete.\n" );
    
    glutTimerFunc(1000,Timer,0);
    
    glutMainLoop();
    
    ::terminate();
    
    return EXIT_SUCCESS;
}   


/*
 //Create the texture using the hard-coded bitmap data
 glTexImage2D(GL_TEXTURE_2D,0,3,Img.GetWidth(),Img.GetHeight(),0,GL_RGB,GL_UNSIGNED_BYTE,Img.GetImg());
 //Set the magnification and minimization filtering to GL_NEAREST
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 
 // Trails - Draw a single Quad to blend instead of clear screen 
 glColor4f(0.f, 0.f, 0.f, 0.9f);   //glColor4f(0.f, 0.f, 0.f, 0.01f);
 //glEnable(GL_TEXTURE_2D);	//Enable the texture to draw the polygon
 glBegin(GL_QUADS);                    
 glTexCoord2f(0, 1);  glVertex2f(0.f, HEIGHT);              
 glTexCoord2f(1, 1);  glVertex2f(WIDTH, HEIGHT);            
 glTexCoord2f(1, 0);  glVertex2f( WIDTH,0.f);               
 glTexCoord2f(0, 0);  glVertex2f(0.f,0.f); 
 glEnd();
 //glDisable(GL_TEXTURE_2D);
 
 //glTexCoord2f(1, 0); glVertex2f(1, -1);
 //glTexCoord2f(1, 1); glVertex2f(1,1);
 //glTexCoord2f(0, 1); glVertex2f(-1, 1);
 
 
 //  But totally clear stats display area
 glBegin(GL_QUADS);
 glColor4f(0.f, 0.f, 0.f, 1.f);
 glVertex2f(0.f, HEIGHT/10.f);              
 glVertex2f(WIDTH, HEIGHT/10.f);              
 glVertex2f( WIDTH,0.f);              
 glVertex2f(0.f,0.f);              
 
 glVertex2f(0.f, HEIGHT);              
 glVertex2f(WIDTH/20.f, HEIGHT);              
 glVertex2f( WIDTH/20.f,0.f);              
 glVertex2f(0.f,0.f);              
 glEnd();    
 
 */
