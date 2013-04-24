//
//  util.cpp
//  interface
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "InterfaceConfig.h"
#include <iostream>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SharedUtil.h>

#include "Log.h"
#include "world.h"
#include "Util.h"

using namespace std;

// no clue which versions are affected...
#define WORKAROUND_BROKEN_GLUT_STROKES
// see http://www.opengl.org/resources/libraries/glut/spec3/node78.html
static float MONO_STROKE_WIDTH_GLUT = 104.76;


void eulerToOrthonormals(glm::vec3 * angles, glm::vec3 * front, glm::vec3 * right, glm::vec3 * up) {
    //
    //  Converts from three euler angles to the associated orthonormal vectors
    //
    //  Angles contains (pitch, yaw, roll) in radians
    //
    
    //  First, create the quaternion associated with these euler angles
    glm::quat q(glm::vec3(angles->x, -(angles->y), angles->z));

    //  Next, create a rotation matrix from that quaternion
    glm::mat4 rotation;
    rotation = glm::mat4_cast(q);
    
    //  Transform the original vectors by the rotation matrix to get the new vectors
    glm::vec4 qup(0,1,0,0);
    glm::vec4 qright(-1,0,0,0);
    glm::vec4 qfront(0,0,1,0);
    glm::vec4 upNew    = qup*rotation;
    glm::vec4 rightNew = qright*rotation;
    glm::vec4 frontNew = qfront*rotation;
    
    //  Copy the answers to output vectors
    up->x = upNew.x;  up->y = upNew.y;  up->z = upNew.z;
    right->x = rightNew.x;  right->y = rightNew.y;  right->z = rightNew.z;
    front->x = frontNew.x;  front->y = frontNew.y;  front->z = frontNew.z;
}



//  Return the azimuth angle in degrees between two points.
float azimuth_to(glm::vec3 head_pos, glm::vec3 source_pos) {
    return atan2(head_pos.x - source_pos.x, head_pos.z - source_pos.z) * 180.0f / PIf;
}

//  Return the angle in degrees between the head and an object in the scene.  The value is zero if you are looking right at it.  The angle is negative if the object is to your right.   
float angle_to(glm::vec3 head_pos, glm::vec3 source_pos, float render_yaw, float head_yaw) {
    return atan2(head_pos.x - source_pos.x, head_pos.z - source_pos.z) * 180.0f / PIf + render_yaw + head_yaw;
}

void render_vector(glm::vec3 * vec)
{
    //  Show edge of world 
    glDisable(GL_LIGHTING);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glLineWidth(1.0);
    glBegin(GL_LINES);
    //  Draw axes
    glColor3f(1,0,0);
    glVertex3f(-1,0,0);
    glVertex3f(1,0,0);
    glColor3f(0,1,0);
    glVertex3f(0,-1,0);
    glVertex3f(0, 1, 0);
    glColor3f(0,0,1);
    glVertex3f(0,0,-1);
    glVertex3f(0, 0, 1);
    // Draw vector
    glColor3f(1,1,1);
    glVertex3f(0,0,0);
    glVertex3f(vec->x, vec->y, vec->z);
    // Draw marker dots for magnitude    
    glEnd();
    float particleAttenuationQuadratic[] =  { 0.0f, 0.0f, 2.0f }; // larger Z = smaller particles
    float particleAttenuationConstant[] = { 1.0f, 0.0f, 0.0f };

    glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, particleAttenuationQuadratic );
    
    glEnable(GL_POINT_SMOOTH);
    glPointSize(10.0);
    glBegin(GL_POINTS);
    glColor3f(1,0,0);
    glVertex3f(vec->x,0,0);
    glColor3f(0,1,0);
    glVertex3f(0,vec->y,0);
    glColor3f(0,0,1);
    glVertex3f(0,0,vec->z);
    glEnd();

    glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, particleAttenuationConstant );
}

void render_world_box()
{
    //  Show edge of world 
    glDisable(GL_LIGHTING);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glLineWidth(1.0);
    glBegin(GL_LINES);
    glColor3f(1,0,0);
    glVertex3f(0,0,0);
    glVertex3f(WORLD_SIZE,0,0);
    glColor3f(0,1,0);
    glVertex3f(0,0,0);
    glVertex3f(0, WORLD_SIZE, 0);
    glColor3f(0,0,1);
    glVertex3f(0,0,0);
    glVertex3f(0, 0, WORLD_SIZE);
    glEnd();
    //  Draw little marker dots along the axis
    glEnable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(WORLD_SIZE,0,0);
    glColor3f(1,0,0);
    glutSolidSphere(0.125,10,10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0,WORLD_SIZE,0);
    glColor3f(0,1,0);
    glutSolidSphere(0.125,10,10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0,0,WORLD_SIZE);
    glColor3f(0,0,1);
    glutSolidSphere(0.125,10,10);
    glPopMatrix();
}

double diffclock(timeval *clock1,timeval *clock2)
{
	double diffms = (clock2->tv_sec - clock1->tv_sec) * 1000.0;
    diffms += (clock2->tv_usec - clock1->tv_usec) / 1000.0;   // us to ms
    
	return diffms;
}

int widthText(float scale, int mono, char const* string) {
    int width = 0;
    if (!mono) {
        width = scale * glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char *) string);
    } else {
#ifndef WORKAROUND_BROKEN_GLUT_STROKES
        width = scale * glutStrokeLength(GLUT_STROKE_MONO_ROMAN, (const unsigned char *) string);
#else
        // return value is unreliable, so just calculate it
        width = scale * float(strlen(string)) * MONO_STROKE_WIDTH_GLUT;
#endif
    }
    return width;
}

void drawtext(int x, int y, float scale, float rotate, float thick, int mono,
              char const* string, float r, float g, float b)
{
    //
    //  Draws text on screen as stroked so it can be resized
    //
    int len, i;
    glPushMatrix();
    glTranslatef( static_cast<float>(x), static_cast<float>(y), 0.0f);
    glColor3f(r,g,b);
    glRotated(180+rotate,0,0,1);
    glRotated(180,0,1,0);
    glLineWidth(thick);
    glScalef(scale, scale, 1.0);
    len = (int) strlen(string);
	for (i = 0; i < len; i++)
	{
        if (!mono) {
            glutStrokeCharacter(GLUT_STROKE_ROMAN, int(string[i]));
        } else {
#ifdef WORKAROUND_BROKEN_GLUT_STROKES
            if (string[i] != 'm') {
#endif
                glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, int(string[i]));
#ifdef WORKAROUND_BROKEN_GLUT_STROKES
            } else {
                // some glut implementations have a broken 'm'...
                unsigned char tmpStr[2]; 
                tmpStr[0] = string[i];
                tmpStr[1] = '\0';
                float scale = MONO_STROKE_WIDTH_GLUT / glutStrokeLength(GLUT_STROKE_ROMAN, tmpStr);
                glScalef(scale, 1.0f, 1.0f);
                glutStrokeCharacter(GLUT_STROKE_ROMAN, int(string[i]));
                // staying humble on the stack - might be in projection mode
                glScalef(1.0f / scale, 1.0f, 1.0f);
            }
#endif
        }
	}
    glPopMatrix();

}


void drawvec3(int x, int y, float scale, float rotate, float thick, int mono, glm::vec3 vec, 
              float r, float g, float b)
{
    //
    //  Draws text on screen as stroked so it can be resized
    //
    char vectext[20];
    sprintf(vectext,"%3.1f,%3.1f,%3.1f", vec.x, vec.y, vec.z);
    int len, i;
    glPushMatrix();
    glTranslatef(static_cast<float>(x), static_cast<float>(y), 0);
    glColor3f(r,g,b);
    glRotated(180+rotate,0,0,1);
    glRotated(180,0,1,0);
    glLineWidth(thick);
    glScalef(scale, scale, 1.0);
    len = (int) strlen(vectext);
	for (i = 0; i < len; i++)
	{
        if (!mono) glutStrokeCharacter(GLUT_STROKE_ROMAN, int(vectext[i]));
        else glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, int(vectext[i]));
	}
    glPopMatrix();
    
} 

void drawGroundPlaneGrid( float size, int resolution )
{

	glColor3f( 0.4f, 0.5f, 0.3f );
	glLineWidth(2.0);
		
	float gridSize = 10.0;
	int gridResolution = 20;

	for (int g=0; g<gridResolution; g++)
	{
		float fraction = (float)g / (float)( gridResolution - 1 );
		float inc = -gridSize * ONE_HALF + fraction * gridSize;
		glBegin( GL_LINE_STRIP );			
		glVertex3f( inc, 0.0f, -gridSize * ONE_HALF );
		glVertex3f( inc, 0.0f,  gridSize * ONE_HALF );
		glEnd();
	}
		
	for (int g=0; g<gridResolution; g++)
	{
		float fraction = (float)g / (float)( gridResolution - 1 );
		float inc = -gridSize * ONE_HALF + fraction * gridSize;
		glBegin( GL_LINE_STRIP );			
		glVertex3f( -gridSize * ONE_HALF, 0.0f, inc );
		glVertex3f(  gridSize * ONE_HALF, 0.0f, inc );
		glEnd();
	}
    
    // Draw a translucent quad just underneath the grid.
    glColor4f(0.5, 0.5, 0.5, 0.4);
    glBegin(GL_QUADS);
    glVertex3f(-gridSize * ONE_HALF, 0, -gridSize * ONE_HALF);
    glVertex3f(gridSize * ONE_HALF, 0, -gridSize * ONE_HALF);
    glVertex3f(gridSize * ONE_HALF, 0, gridSize * ONE_HALF);
    glVertex3f(-gridSize * ONE_HALF, 0, gridSize * ONE_HALF);
    glEnd();
}


void renderOrientationDirections( glm::vec3 position, Orientation orientation, float size ) {
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

bool closeEnoughForGovernmentWork(float a, float b) {
    float distance = std::abs(a-b);
    //printLog("closeEnoughForGovernmentWork() a=%1.10f b=%1.10f distance=%1.10f\n",a,b,distance);
    return (distance < 0.00001f);
}


void testOrientationClass() {
    printLog("\n----------\ntestOrientationClass()\n----------\n\n");
    
    oTestCase tests[] = { 
        //       - inputs ------------, outputs --------------------  -------------------  ----------------------------
        //                              -- front -------------------, -- up -------------, -- right -------------------
        //       ( yaw  , pitch, roll , front.x , front.y , front.z , up.x , up.y , up.z , right.x , right.y , right.z  )

        // simple yaw tests
        oTestCase( 0.f  , 0.f  , 0.f  ,  0.f       , 0.f , 1.0f      ,  0.f , 1.0f , 0.f  , -1.0f     , 0.f     , 0.f      ),
        oTestCase(45.0f , 0.f  , 0.f  ,  0.707107f , 0.f , 0.707107f ,  0.f , 1.0f , 0.f  , -0.707107f, 0.f     , 0.707107f),
        oTestCase( 90.0f, 0.f  , 0.f  , 1.0f    , 0.f     , 0.f     ,   0.f , 1.0f , 0.f  , 0.0f    , 0.f     , 1.0f     ),
        oTestCase(135.0f, 0.f  , 0.f  ,  0.707107f , 0.f ,-0.707107f ,  0.f , 1.0f , 0.f  , 0.707107f, 0.f     , 0.707107f),
        oTestCase(180.0f, 0.f  , 0.f  ,  0.f    , 0.f     , -1.0f   ,   0.f , 1.0f , 0.f  , 1.0f    , 0.f     , 0.f      ),
        oTestCase(225.0f, 0.f  , 0.f  , -0.707107f , 0.f ,-0.707107f ,  0.f , 1.0f , 0.f  , 0.707107f, 0.f     , -0.707107f),
        oTestCase(270.0f, 0.f  , 0.f  , -1.0f   , 0.f    , 0.f       ,   0.f , 1.0f , 0.f  , 0.0f    , 0.f     , -1.0f    ),
        oTestCase(315.0f, 0.f  , 0.f  , -0.707107f , 0.f , 0.707107f ,  0.f , 1.0f , 0.f  , -0.707107f, 0.f     , -0.707107f),
        oTestCase(-45.0f, 0.f  , 0.f  , -0.707107f , 0.f , 0.707107f ,  0.f , 1.0f , 0.f  , -0.707107f, 0.f     , -0.707107f),
        oTestCase(-90.0f, 0.f  , 0.f  , -1.0f   , 0.f    , 0.f       ,   0.f , 1.0f , 0.f  , 0.0f    , 0.f     , -1.0f    ),
        oTestCase(-135.0f,0.f  , 0.f  , -0.707107f , 0.f ,-0.707107f ,  0.f , 1.0f , 0.f  , 0.707107f, 0.f     , -0.707107f),
        oTestCase(-180.0f,0.f  , 0.f  ,  0.f    , 0.f     , -1.0f   ,   0.f , 1.0f , 0.f  , 1.0f    , 0.f     , 0.f      ),
        oTestCase(-225.0f,0.f  , 0.f  ,  0.707107f , 0.f ,-0.707107f ,  0.f , 1.0f , 0.f  , 0.707107f, 0.f     , 0.707107f),
        oTestCase(-270.0f,0.f  , 0.f  , 1.0f    , 0.f     , 0.f     ,   0.f , 1.0f , 0.f  , 0.0f    , 0.f     , 1.0f     ),
        oTestCase(-315.0f,0.f  , 0.f  ,  0.707107f , 0.f , 0.707107f ,  0.f , 1.0f , 0.f  , -0.707107f, 0.f     , 0.707107f),

        // simple pitch tests
        oTestCase( 0.f  , 0.f  , 0.f  ,  0.f, 0.f       , 1.0f      ,  0.f , 1.0f    , 0.f       ,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,45.0f , 0.f  ,  0.f, 0.707107f , 0.707107f,   0.f ,0.707107f, -0.707107f,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,90.f  , 0.f  ,  0.f, 1.0f      , 0.0f     ,   0.f ,0.0f     , -1.0f     ,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,135.0f, 0.f  ,  0.f, 0.707107f , -0.707107f,  0.f ,-0.707107f, -0.707107f,   -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,180.f , 0.f  ,  0.f, 0.0f      ,-1.0f     ,   0.f ,-1.0f    , 0.f       ,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,225.0f, 0.f  ,  0.f,-0.707107f , -0.707107f,  0.f ,-0.707107f, 0.707107f,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,270.f , 0.f  ,  0.f,-1.0f      , 0.0f     ,   0.f ,0.0f     , 1.0f      ,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,315.0f, 0.f  ,  0.f,-0.707107f , 0.707107f,  0.f , 0.707107f,  0.707107f,    -1.0f  , 0.f  , 0.f  ),

        // simple roll tests
        oTestCase( 0.f  , 0.f  , 0.f    , 0.f  , 0.f , 1.0f  ,  0.f       , 1.0f      ,0.0f   , -1.0f     , 0.f      , 0.0f ),
        oTestCase( 0.f  , 0.f  ,45.0f   , 0.f  , 0.f , 1.0f  ,  0.707107f , 0.707107f ,0.0f   , -0.707107f, 0.707107f, 0.0f ),
        oTestCase( 0.f  , 0.f  ,90.f    , 0.f  , 0.f , 1.0f  ,  1.0f      , 0.0f      ,0.0f   , 0.0f      , 1.0f     , 0.0f ),
        oTestCase( 0.f  , 0.f  ,135.0f  , 0.f  , 0.f , 1.0f  ,  0.707107f , -0.707107f,0.0f   , 0.707107f , 0.707107f, 0.0f ),
        oTestCase( 0.f  , 0.f  ,180.f   , 0.f  , 0.f , 1.0f  ,  0.0f      , -1.0f     ,0.0f   , 1.0f      , 0.0f     , 0.0f ),
        oTestCase( 0.f  , 0.f  ,225.0f  , 0.f  , 0.f , 1.0f  ,  -0.707107f, -0.707107f,0.0f   , 0.707107f ,-0.707107f, 0.0f ),
        oTestCase( 0.f  , 0.f  ,270.f   , 0.f  , 0.f , 1.0f  , -1.0f      , 0.0f      ,0.0f   , 0.0f      , -1.0f    , 0.0f ),
        oTestCase( 0.f  , 0.f  ,315.0f  , 0.f  , 0.f , 1.0f  ,  -0.707107f, 0.707107f ,0.0f   , -0.707107f,-0.707107f, 0.0f ),

        // yaw combo tests
        oTestCase( 90.f , 90.f , 0.f    ,  0.f  , 1.0f , 0.0f    ,  -1.0f , 0.0f , 0.f     , 0.0f , 0.f   , 1.0f       ),
        oTestCase( 90.f , 0.f , 90.f    ,  1.0f , 0.0f,  0.f     ,  0.0f , 0.0f , -1.f     , 0.0f , 1.0f  , 0.0f       ),
    };
    
    int failedCount = 0;
    int totalTests = sizeof(tests)/sizeof(oTestCase);
    
    for (int i=0; i < totalTests; i++) {
    
        bool passed = true; // I'm an optimist!

        float yaw   = tests[i].yaw;
        float pitch = tests[i].pitch;
        float roll  = tests[i].roll;

        Orientation o1;
        o1.setToIdentity();
        o1.yaw(yaw);
        o1.pitch(pitch);
        o1.roll(roll);
    
        glm::vec3 front = o1.getFront();
        glm::vec3 up    = o1.getUp();
        glm::vec3 right = o1.getRight();

        printLog("\n-----\nTest: %d - yaw=%f , pitch=%f , roll=%f \n",i+1,yaw,pitch,roll);

        printLog("\nFRONT\n");
        printLog(" + received: front.x=%f, front.y=%f, front.z=%f\n",front.x,front.y,front.z);
        
        if (closeEnoughForGovernmentWork(front.x, tests[i].frontX) 
            && closeEnoughForGovernmentWork(front.y, tests[i].frontY)
            && closeEnoughForGovernmentWork(front.z, tests[i].frontZ)) {
            printLog("  front vector PASSES!\n");
        } else {
            printLog("   expected: front.x=%f, front.y=%f, front.z=%f\n",tests[i].frontX,tests[i].frontY,tests[i].frontZ);
            printLog("  front vector FAILED! \n");
            passed = false;
        }
            
        printLog("\nUP\n");
        printLog(" + received: up.x=%f,    up.y=%f,    up.z=%f\n",up.x,up.y,up.z);
        if (closeEnoughForGovernmentWork(up.x, tests[i].upX) 
            && closeEnoughForGovernmentWork(up.y, tests[i].upY)
            && closeEnoughForGovernmentWork(up.z, tests[i].upZ)) {
            printLog("  up vector PASSES!\n");
        } else {
            printLog("  expected: up.x=%f, up.y=%f, up.z=%f\n",tests[i].upX,tests[i].upY,tests[i].upZ);
            printLog("  up vector FAILED!\n");
            passed = false;
        }


        printLog("\nRIGHT\n");
        printLog(" + received: right.x=%f, right.y=%f, right.z=%f\n",right.x,right.y,right.z);
        if (closeEnoughForGovernmentWork(right.x, tests[i].rightX) 
            && closeEnoughForGovernmentWork(right.y, tests[i].rightY)
            && closeEnoughForGovernmentWork(right.z, tests[i].rightZ)) {
            printLog("  right vector PASSES!\n");
        } else {
            printLog("   expected: right.x=%f, right.y=%f, right.z=%f\n",tests[i].rightX,tests[i].rightY,tests[i].rightZ);
            printLog("  right vector FAILED!\n");
            passed = false;
        }
        
        if (!passed) {
            printLog("\n-----\nTest: %d - FAILED! \n----------\n\n",i+1);
            failedCount++;
        }
    }
    printLog("\n-----\nTotal Failed: %d out of %d \n----------\n\n",failedCount,totalTests);
    printLog("\n----------DONE----------\n\n");
}




