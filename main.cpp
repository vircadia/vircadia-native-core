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

#include <stdlib.h>
#include <GLUT/glut.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>
#include <stdio.h>
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

//TGAImg Img;

using namespace std;

//   Junk for talking to the Serial Port 
int serial_fd;
const int MAX_BUFFER = 100;
char serial_buffer[MAX_BUFFER];
int serial_buffer_pos = 0;
int serial_on = 0;                  //  Are we using serial port for I/O
int ping_test = 1; 
int display_ping = 0; 

timeval begin_ping, end_ping;
timeval timer_start, timer_end;
double elapsedTime;

//  Socket operation stuff 
int UDP_socket;
char* incoming_packet;

//  Getting a target location from other machine (or loopback) to display
int target_x, target_y; 
int target_display = 0;
int bytes_in = 0;


unsigned char last_key = 0; 

double ping = 0; 
//clock_t begin_ping, end_ping;


#define WIDTH 1000					//  Width,Height of simulation area in cells
#define HEIGHT 600
#define BOTTOM_MARGIN 0				
#define RIGHT_MARGIN 0
#define TEXT_HEIGHT 14


//#define NUM_TRIS 1000000
//float tris[NUM_TRIS * 9];
//float tri_colors[NUM_TRIS * 3];

//  Test data for creating fields that affect particles 
//  If the simulation 'world' is a box with 10M boundaries, the offset to a field cell is given by:
//  element = [x/10 + (y/10)*10 + (z*/10)*100] 
//
//  The vec(x,y,z) corner of a field cell at element i is:
// 
//  z = (int)( i / 100)
//  y = (int)(i % 100 / 10)
//  x = (int)(i % 10)

#define SLEEP 0
#define NUM_TRIS 100000
struct {
    float vertices[NUM_TRIS * 9];
    float normals [NUM_TRIS * 3];
    float colors  [NUM_TRIS * 3];
    float vel     [NUM_TRIS * 3];
    glm::vec3 vel1[NUM_TRIS];
    glm::vec3 vel2[NUM_TRIS];
}tris;

float twiddles[NUM_TRIS * 9];

float yaw =0.f;
float pitch = 0.f;
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
int noise_on = 0;					//  Whether to fire randomly
int step_on = 0;                    
int display_levels = 1;
int display_head = 1;
int display_field = 0;
int reset_sensors = 0;

int mouse_x, mouse_y;				//  Where is the mouse 
int mouse_pressed = 0;				//  true if mouse has been pressed (clear when finished)

int accel_x, accel_y;

int speed;

float mag_imbalance = 0.f;

int adc_channels[4];                //  Measured input values for gyros, accelerometers
float avg_adc_channels[4];

int first_measurement = 1;

int framecount = 0;                 //  Measure timing for framerate 
int samplecount = 0;
float FPS = 120.f;

void output(int x, int y, char *string)
{
	//  Writes a text string to the screen as a bitmap at location x,y
	int len, i;
	glRasterPos2f(x, y);
	len = (int) strlen(string);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
	}
}

float randFloat () {
    return (rand()%10000)/10000.f;
}

double diffclock(timeval clock1,timeval clock2)
{
	double diffms = (clock2.tv_sec - clock1.tv_sec) * 1000.0;
    diffms += (clock2.tv_usec - clock1.tv_usec) / 1000.0;   // us to ms

	return diffms;
}

// Collect sensor data from serial port, return number of lines of I/O data read 
int read_sensors(int first_measurement)
{
    int lines_read = 0;
    const float AVG_RATE = 0.00001;
    if (serial_on)
    {
        char bufchar[1];
        while (read(serial_fd, bufchar, 1) > 0)
        {
            serial_buffer[serial_buffer_pos] = bufchar[0];
            serial_buffer_pos++;
            //  Have we reached end of a line of input?
            if ((bufchar[0] == '\n') || (serial_buffer_pos >= MAX_BUFFER))
            {
                lines_read++;
                //  At end - Extract value from string to variables
                if (serial_buffer[0] != 'p')
                {
                    samplecount++;
                    sscanf(serial_buffer, "%d %d %d %d", &adc_channels[0], 
                           &adc_channels[1], 
                           &adc_channels[2], 
                           &adc_channels[3]);
                    for (int i = 0; i < 4; i++)
                    {
                        if (!first_measurement)
                            avg_adc_channels[i] = (1.f - AVG_RATE)*avg_adc_channels[i] + 
                            AVG_RATE*(float)adc_channels[i];
                        else
                        {
                            avg_adc_channels[i] = (float)adc_channels[i];
                        }
                    }
                }
                //  Clear rest of string for printing onscreen
                while(serial_buffer_pos++ < MAX_BUFFER) serial_buffer[serial_buffer_pos] = ' ';
                serial_buffer_pos = 0;
            }
            if (bufchar[0] == 'p')
            {
                gettimeofday(&end_ping, NULL);
                ping = diffclock(begin_ping,end_ping);
                display_ping = 1; 
            }
        }
    }
    return lines_read;
}

//  Every second, check the frame rates and other stuff 
void Timer(int extra)
{
	char title[100];
    
    gettimeofday(&timer_end, NULL);
    FPS = (float)framecount / ((float)diffclock(timer_start,timer_end) / 1000.f);
    
    //  Calculate exact FPS 
    
	sprintf(title, "FPS = %4.4f, IO/sec = %d, IOpng = %4.4f, bytes/sec = %d", 
            FPS, samplecount, ping, bytes_in);
	glutSetWindowTitle(title);
	framecount = 0;
    samplecount = 0; 
    bytes_in = 0;
    
    if (serial_on && ping_test)
    {
        char buf[] = "ping";
        write(serial_fd,buf,4);
        write(serial_fd, "\r", 1);
        gettimeofday(&begin_ping, NULL);
        display_ping = 2;
    }
	glutTimerFunc(1000,Timer,0);
    gettimeofday(&timer_start, NULL);
}

void display_stats(void)
{
	//  bitmap chars are about 10 pels high 
	glColor3f(1.0f, 1.0f, 1.0f);
    char legend[] = "/ - toggle this display, Q - exit, N - toggle noise, M - toggle map, T - test audio";
	output(10,15,legend);
    if (serial_on)
    {
        /*char stuff[50];
        if (bytes_read > 0)
        {
            sprintf(stuff, "bytes=%i", bytes_read);
            output(10,60,stuff);
        }*/
        //output(100,60,serial_buffer);
    }
	char mouse[50];
	sprintf(mouse, "mouse_x = %i, mouse_y = %i, pressed = %i, key = %i", mouse_x, mouse_y, mouse_pressed, last_key);
	output(10,35,mouse);
    char adc[200];
	sprintf(adc, "pitch_rate = %i, yaw_rate = %i, accel_lat = %i, accel_fwd = %i, loc[0] = %3.1f loc[1] = %3.1f, loc[2] = %3.1f", 
            (int)(adc_channels[0] - avg_adc_channels[0]),
            (int)(adc_channels[1] - avg_adc_channels[1]),
            (int)(adc_channels[2] - avg_adc_channels[2]),
            (int)(adc_channels[3] - avg_adc_channels[3]),
            location[0], location[1], location[2] 
            );
	output(10,50,adc);

	
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
    Audio::init();

    avg_adc_channels[0] = avg_adc_channels[1] = avg_adc_channels[2] = avg_adc_channels[3] = 0.f;
        
    int i, j;
    
    //  Initialize Field values 
    field_init();
    
    /*
    const float FIELD_SCALE = 0.00005;
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        field[i].x = 0.001;  //(randFloat() - 0.5)*FIELD_SCALE;
        field[i].y = 0.001;  //(randFloat() - 0.5)*FIELD_SCALE;
        field[i].z = 0.001; //(randFloat() - 0.5)*FIELD_SCALE;
    }*/

    
    float tri_scale, r;
    const float VEL_SCALE = 0.00;
    for (i = 0; i < NUM_TRIS; i++)
    {
        r = randFloat();
        if (r > .999) tri_scale = 0.7; 
        else if (r > 0.90) tri_scale = 0.1;
        else tri_scale = 0.05; 
        
        tris.colors[i*3] = randFloat();
        tris.colors[i*3+1] = randFloat();
        tris.colors[i*3+2] = randFloat();

        tris.vel[i*3] = (randFloat() - 0.5)*VEL_SCALE;
        tris.vel[i*3+1] = (randFloat() - 0.5)*VEL_SCALE;
        tris.vel[i*3+2] = (randFloat() - 0.5)*VEL_SCALE;

        glm::vec3 pos (randFloat() * WORLD_SIZE,
                       randFloat() * WORLD_SIZE,
                       randFloat() * WORLD_SIZE);
        glm::vec3 verts[3];
        for (j = 0; j < 3; j++) {
            verts[j].x = pos.x + randFloat() * tri_scale - tri_scale/2.f;
            verts[j].y = pos.y + randFloat() * tri_scale - tri_scale/2.f;
            verts[j].z = pos.z + randFloat() * tri_scale - tri_scale/2.f;
            tris.vertices[i*9 + j*3] = verts[j].x;
            tris.vertices[i*9 + j*3 + 1] = verts[j].y;
            tris.vertices[i*9 + j*3 + 2] = verts[j].z;
        }
        // reuse pos for the normal
        glm::normalize((pos += glm::cross(verts[1] - verts[0], verts[2] - verts[0])));
        tris.normals[i*3] = pos.x;
        tris.normals[i*3+1] = pos.y;
        tris.normals[i*3+2] = pos.z;

    }
    
    const float TWIDDLE_SCALE = 0.01;
    for (i = 0; i < NUM_TRIS; i++)
    {
        twiddles[i*3] = (randFloat() - 0.5)*TWIDDLE_SCALE;
        twiddles[i*3 + 1] = (randFloat() - 0.5)*TWIDDLE_SCALE;
        twiddles[i*3 + 2] = (randFloat() - 0.5)*TWIDDLE_SCALE;
    }
     
    //  Call readsensors for a while to get stable initial values on sensors    
    gettimeofday(&timer_start, NULL);
    read_sensors(1);
    int done = 0;
    while (!done)
    {
        read_sensors(0);
        gettimeofday(&timer_end, NULL);
        if (diffclock(timer_start,timer_end) > 1000) done = 1;
    }
    
    gettimeofday(&timer_start, NULL);
}

void terminate () {
    // Close serial port
    close(serial_fd);

    Audio::terminate();
    exit(EXIT_SUCCESS);
}

const float SCALE_SENSORS = 0.3f;
const float SCALE_X = 2.f;
const float SCALE_Y = 1.f;


void update_tris()
{
    int i;
    float dist_sqrd;
    float field_val[3];
    
    for (i = 0; i < NUM_TRIS; i++)
    {
        // Update position
        tris.vertices[i*9+0] += tris.vel[i*3];
        tris.vertices[i*9+3] += tris.vel[i*3];
        tris.vertices[i*9+6] += tris.vel[i*3];
        
        tris.vertices[i*9+1] += tris.vel[i*3+1];
        tris.vertices[i*9+4] += tris.vel[i*3+1];
        tris.vertices[i*9+7] += tris.vel[i*3+1]; 
        
        tris.vertices[i*9+2] += tris.vel[i*3+2];
        tris.vertices[i*9+5] += tris.vel[i*3+2];
        tris.vertices[i*9+8] += tris.vel[i*3+2]; 
        
        if (0)
        {
            dist_sqrd = tris.vertices[i*9+0]*tris.vertices[i*9+0] +
                        tris.vertices[i*9+1]*tris.vertices[i*9+1] + 
                        tris.vertices[i*9+2]*tris.vertices[i*9+2];
            
            if (dist_sqrd > 1.0)
            {
                glm::vec3 pos (tris.vertices[i*9+0],tris.vertices[i*9+1], tris.vertices[i*9+2]);
                glm::normalize(pos);
                pos*=-1/dist_sqrd*0.0001;
            
                tris.vel[i*3] += pos.x;
                tris.vel[i*3+1] += pos.y;
                tris.vel[i*3+2] += pos.z;
            }
        }
        
        // Add a little gravity 
        const float GRAVITY = 0.0001;
        tris.vel[i*3+1] -= GRAVITY;
       
        // Drag:  Decay velocity
        tris.vel[i*3] *= 0.99;
        tris.vel[i*3+1] *= 0.99;
        tris.vel[i*3+2] *= 0.99;
                 
        // Read and add velocity from field 
        field_value(field_val, &tris.vertices[i*9]);
        tris.vel[i*3] += field_val[0];
        tris.vel[i*3+1] += field_val[1];
        tris.vel[i*3+2] += field_val[2];

        // bounce at edge of world 
        // X-Direction
        if ((tris.vertices[i*9+0] > WORLD_SIZE) || (tris.vertices[i*9+0] < 0.0))
            tris.vel[i*3]*= -1.0;
        // Y-direction
        if ((tris.vertices[i*9+1] > WORLD_SIZE) || (tris.vertices[i*9+1] < 0.0))
        { 
            //tris.vel[i*3+1]*= -1.0;
            if (tris.vertices[i*9+1] < 0.0)
            {
                tris.vertices[i*9+1] = tris.vertices[i*9+4] = tris.vertices[i*9+7] = WORLD_SIZE;
                //tris.vel[i*3+1]*= -1.0;
            }
        }
        // Z-Direction
        if ((tris.vertices[i*9+2] > WORLD_SIZE) || (tris.vertices[i*9+2] < 0.0))
            tris.vel[i*3+2]*= -1.0;
    }
}

void update_pos(float frametime)
//  Using serial data, update avatar/render position and angles
{
    float measured_yaw_rate = adc_channels[1] - avg_adc_channels[1];
    float measured_pitch_rate = adc_channels[0] - avg_adc_channels[0];
    float measured_lateral_accel = adc_channels[2] - avg_adc_channels[2];
    float measured_fwd_accel = avg_adc_channels[3] - adc_channels[3];
    
    //  Update avatar head position based on measured gyro rates
    yaw += measured_yaw_rate * 1.20 * frametime;
    pitch += measured_pitch_rate * -1.0 * frametime;
    //  Decay avatar head back toward zero
    pitch *= (1.f - 5.0*frametime); 
    yaw *= (1.f - 7.0*frametime);

    
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
    
    if (reset_sensors)
    {
        render_yaw = start_yaw;
        yaw = render_yaw_rate = 0; 
        pitch = render_pitch = render_pitch_rate = 0;
        lateral_vel = 0;
        location[0] = start_location[0];
        location[1] = start_location[1];
        location[2] = start_location[2];
        fwd_vel = 0.0;
        reset_sensors = 0; 
        read_sensors(1);
    }
    
}

void display(void)
{
    
    int i,j;
    
    glEnable (GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    GLfloat light_position0[] = { 1.0, 1.0, 0.0, 0.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
    GLfloat ambient_color[] = { 0.125, 0.305, 0.5 };  
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color);
    //GLfloat diffuse_color[] = { 1.0, 0.84, 0.66};  
    GLfloat diffuse_color[] = { 0.5, 0.42, 0.33 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color);
    GLfloat specular_color[] = { 1.0, 1.0, 1.0, 1.0};
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular_color);
    
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular_color);
    glMateriali(GL_FRONT, GL_SHININESS, 96);

   
    glPushMatrix();
    
    //  Rotate, translate to camera location 
    glRotatef(render_pitch, 1, 0, 0);
    glRotatef(render_yaw, 0, 1, 0);
    glTranslatef(location[0], location[1], location[2]);

    glEnable(GL_DEPTH_TEST);
    
    //  Draw a few 'planets' to find and explore 
    glPushMatrix();
        glTranslatef(1.f, 1.f, 1.f);
        glColor3f(1, 0, 0); 
        glutSolidSphere(0.6336, 20, 20); 
        glTranslatef(5, 5, 5);
        glColor3f(1, 1, 0); 
        glutSolidSphere(0.4, 20, 20); 
        glTranslatef(-2.5, -2.5, 2.5);
        glColor3f(1, 0, 1); 
        glutSolidSphere(0.3, 20, 20); 
    glPopMatrix();
    
    // Draw Triangles 
    
    glBegin(GL_TRIANGLES);
    for (i = 0; i < NUM_TRIS; i++)
    {
        glColor3f(tris.colors[i*3],
                  tris.colors[i*3+1],
                  tris.colors[i*3+2]);
        for (j = 0; j < 3; j++)
        {
            glVertex3f(tris.vertices[i*9 + j*3],
                       tris.vertices[i*9 + j*3 + 1],
                       tris.vertices[i*9 + j*3 + 2]);
        }
        glNormal3f(tris.normals[i*3],
                   tris.normals[i*3 + 1],
                   tris.normals[i*3 + 2]);
    }
    glEnd();
    
    //  Show field vectors
    if (display_field) field_render(); 
    
    render_world_box();
        
    glPopMatrix();
    
    // Display floating head in front of viewer
    if (display_head)
    {
        glDisable(GL_DEPTH_TEST);
        glPushMatrix();
        glTranslatef(0.f, -4.f, -15.f);
        glRotatef(yaw/2.0, 0, 1, 0);
        glRotatef(pitch/2.0, 1, 0, 0);
        glScalef(0.5, 1.0, 1.1);
        glColor3f(1.0, 0.84, 0.66);
        glutSolidSphere(1.f, 15, 15);           //  Head
        
        glTranslatef(1.f, 0.f, 0.f);
        glPushMatrix();
        glScalef(0.5, 0.75, 1.0);
        glutSolidSphere(0.5f, 15, 15);          //  Ears
        glPopMatrix();
        glTranslatef(-2.f, 0.f, 0.f);
        glPushMatrix();
        glScalef(0.5, 0.75, 1.0);
        glutSolidSphere(0.5f, 15, 15);
        glPopMatrix();
        glTranslatef(1.f, 1.f, 0.f);
        glScalef(2.5, 0.5, 2.2);
        glColor3f(0.5, 0.5, 0.5);
        glutSolidSphere(0.25f, 15, 15);         //  Beanie
        glPopMatrix();
    }
    
    glEnd();
    glPopMatrix();
    
    
    //  Render 2D overlay:  I/O level bar graphs and text  
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity(); 
    gluOrtho2D(0, WIDTH, HEIGHT, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    if (mouse_pressed == 1)
    {
        glPointSize(20.f);
        glColor3f(1,1,1);
        glEnable(GL_POINT_SMOOTH);
        glBegin(GL_POINTS);
        glVertex2f(target_x, target_y);
        glEnd();
    }
    if (display_ping)
    {
        //  Draw a green dot to indicate receipt of ping signal 
        glPointSize(10.f);
        if (display_ping == 2)
            glColor4f(1.f, 0.f, 0.f, 1.f);
        else 
            glColor4f(0.f, 1.f, 0.f, 1.f);
        glBegin(GL_POINTS);
        glVertex2f(50, 400);
        glEnd();
        display_ping = 0;
    }
    if (display_levels)
    {
        glColor4f(1.f, 1.f, 1.f, 1.f);
        glBegin(GL_LINES);
        glVertex2f(10, HEIGHT*0.95);
        glVertex2f(10, HEIGHT*(0.25 + 0.75f*adc_channels[0]/4096));
        
        glVertex2f(20, HEIGHT*0.95);
        glVertex2f(20, HEIGHT*(0.25 + 0.75f*adc_channels[1]/4096));
        
        glVertex2f(30, HEIGHT*0.95);
        glVertex2f(30, HEIGHT*(0.25 + 0.75f*adc_channels[2]/4096));
        
        glVertex2f(40, HEIGHT*0.95);
        glVertex2f(40, HEIGHT*(0.25 + 0.75f*adc_channels[3]/4096));
        glEnd();
        
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
	if (k == 'n') noise_on = !noise_on;			//  toggle random mutation
    if (k == 'h') display_head = !display_head;
    if (k == 'f') display_field = !display_field;
    if (k == 'e') location[1] -= WORLD_SIZE/100.0;
    if (k == 'c') location[1] += WORLD_SIZE/100.0;
    if (k == 'w') fwd_vel += 0.05;
    if (k == 's') fwd_vel -= 0.05;
    if (k == ' ') reset_sensors = 1;   
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
}

void read_network()
{
    //  Receive packets 
    int bytes_recvd = network_receive(UDP_socket, incoming_packet);
    if (bytes_recvd > 0)
    {
        bytes_in += bytes_recvd;
        if (incoming_packet[0] == 'M')
        {
            sscanf(incoming_packet, "M %d %d", &target_x, &target_y);
            target_display = 1;
            printf("X = %d Y = %d\n", target_x, target_y);
        }
    }
}

void idle(void)
{
    if (!step_on) glutPostRedisplay();
    read_network();
    read_sensors(0);
    update_pos(1.f/FPS); 
    update_tris();
    if (SLEEP)
    {
        usleep(SLEEP);
    }

}

void reshape(int width, int height)
{
    
    glViewport(0, 0, width, height);
    /*
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, height, 0);
    glMatrixMode(GL_MODELVIEW);
     */
    
    glMatrixMode(GL_PROJECTION); //hello
    gluPerspective(45, //view angle
                   1.0, //aspect ratio
                   1.0, //near clip
                   200.0);//far clip
    glMatrixMode(GL_MODELVIEW);

     
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
    int test_recv = network_receive(UDP_socket, incoming_packet);
    printf("Received %i bytes\n", test_recv);
    
       //  Load textures 
    //Img.Load("/Users/philip/Downloads/galaxy1.tga");

    //  Try to setup the serial port I/O 
    if (serial_on)
    {
        serial_fd = open("/dev/tty.usbmodem411", O_RDWR | O_NOCTTY | O_NDELAY); // List usbSerial devices using Terminal ls /dev/tty.*
    
    
        if(serial_fd == -1) {				// Check for port errors
            cout << serial_fd;
            perror("Unable to open serial port\n");
            return (0);
        }
        else init_port(&serial_fd, 115200);        
    }

    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	
    glutInitWindowSize(RIGHT_MARGIN + WIDTH, BOTTOM_MARGIN + HEIGHT);
    
    glutCreateWindow("Interface Test");
    
    initDisplay();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutMotionFunc(motionFunc);
	glutMouseFunc(mouseFunc);
    glutIdleFunc(idle);
	
    init();
    
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
