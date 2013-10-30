//
//  util.cpp
//  interface
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
#include <cstring>
#include <time.h>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/quaternion.hpp>

#include <AvatarData.h>
#include <SharedUtil.h>

#include "InterfaceConfig.h"
#include "ui/TextRenderer.h"
#include "VoxelConstants.h"
#include "world.h"

#include "Util.h"

using namespace std;

// no clue which versions are affected...
#define WORKAROUND_BROKEN_GLUT_STROKES
// see http://www.opengl.org/resources/libraries/glut/spec3/node78.html

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

void printVector(glm::vec3 vec) {
    printf("%4.2f, %4.2f, %4.2f\n", vec.x, vec.y, vec.z);
}

//  Return the azimuth angle in degrees between two points.
float azimuth_to(glm::vec3 head_pos, glm::vec3 source_pos) {
    return atan2(head_pos.x - source_pos.x, head_pos.z - source_pos.z) * 180.0f / PIf;
}

//  Return the angle in degrees between the head and an object in the scene.  The value is zero if you are looking right at it.  The angle is negative if the object is to your right.   
float angle_to(glm::vec3 head_pos, glm::vec3 source_pos, float render_yaw, float head_yaw) {
    return atan2(head_pos.x - source_pos.x, head_pos.z - source_pos.z) * 180.0f / PIf + render_yaw + head_yaw;
}

//  Helper function returns the positive angle in degrees between two 3D vectors 
float angleBetween(const glm::vec3& v1, const glm::vec3& v2) {
    return acos((glm::dot(v1, v2)) / (glm::length(v1) * glm::length(v2))) * 180.f / PIf;
}

//  Helper function return the rotation from the first vector onto the second
glm::quat rotationBetween(const glm::vec3& v1, const glm::vec3& v2) {
    float angle = angleBetween(v1, v2);
    if (isnan(angle) || angle < EPSILON) {
        return glm::quat();
    }
    glm::vec3 axis;
    if (angle > 179.99f) { // 180 degree rotation; must use another axis
        axis = glm::cross(v1, glm::vec3(1.0f, 0.0f, 0.0f));
        float axisLength = glm::length(axis);
        if (axisLength < EPSILON) { // parallel to x; y will work
            axis = glm::normalize(glm::cross(v1, glm::vec3(0.0f, 1.0f, 0.0f)));
        } else {
            axis /= axisLength;
        }        
    } else {
        axis = glm::normalize(glm::cross(v1, v2));
    }
    return glm::angleAxis(angle, axis);
}

//  Safe version of glm::eulerAngles; uses the factorization method described in David Eberly's
//  http://www.geometrictools.com/Documentation/EulerAngles.pdf (via Clyde,
// https://github.com/threerings/clyde/blob/master/src/main/java/com/threerings/math/Quaternion.java)
glm::vec3 safeEulerAngles(const glm::quat& q) {
    float sy = 2.0f * (q.y * q.w - q.x * q.z);
    if (sy < 1.0f - EPSILON) {
        if (sy > -1.0f + EPSILON) {
            return glm::degrees(glm::vec3(
                atan2f(q.y * q.z + q.x * q.w, 0.5f - (q.x * q.x + q.y * q.y)),
                asinf(sy),
                atan2f(q.x * q.y + q.z * q.w, 0.5f - (q.y * q.y + q.z * q.z))));
                
        } else {
            // not a unique solution; x + z = atan2(-m21, m11)
            return glm::degrees(glm::vec3(
                0.0f,
                PIf * -0.5f,
                atan2f(q.x * q.w - q.y * q.z, 0.5f - (q.x * q.x + q.z * q.z))));
        }
    } else {
        // not a unique solution; x - z = atan2(-m21, m11)
        return glm::degrees(glm::vec3(
            0.0f,
            PIf * 0.5f,
            -atan2f(q.x * q.w - q.y * q.z, 0.5f - (q.x * q.x + q.z * q.z))));
    }
}

//  Safe version of glm::mix; based on the code in Nick Bobick's article,
//  http://www.gamasutra.com/features/19980703/quaternions_01.htm (via Clyde,
//  https://github.com/threerings/clyde/blob/master/src/main/java/com/threerings/math/Quaternion.java)
glm::quat safeMix(const glm::quat& q1, const glm::quat& q2, float proportion) {
    float cosa = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
    float ox = q2.x, oy = q2.y, oz = q2.z, ow = q2.w, s0, s1;

    // adjust signs if necessary
    if (cosa < 0.0f) {
        cosa = -cosa;
        ox = -ox;
        oy = -oy;
        oz = -oz;
        ow = -ow;
    }

    // calculate coefficients; if the angle is too close to zero, we must fall back
    // to linear interpolation
    if ((1.0f - cosa) > EPSILON) {
        float angle = acosf(cosa), sina = sinf(angle);
        s0 = sinf((1.0f - proportion) * angle) / sina;
        s1 = sinf(proportion * angle) / sina;
        
    } else {
        s0 = 1.0f - proportion;
        s1 = proportion;
    }

    return glm::normalize(glm::quat(s0 * q1.w + s1 * ow, s0 * q1.x + s1 * ox, s0 * q1.y + s1 * oy, s0 * q1.z + s1 * oz));
}

glm::vec3 extractTranslation(const glm::mat4& matrix) {
    return glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);
}

glm::quat extractRotation(const glm::mat4& matrix, bool assumeOrthogonal) {
    // uses the iterative polar decomposition algorithm described by Ken Shoemake at
    // http://www.cs.wisc.edu/graphics/Courses/838-s2002/Papers/polar-decomp.pdf
    // code adapted from Clyde, https://github.com/threerings/clyde/blob/master/src/main/java/com/threerings/math/Matrix4f.java

    // start with the contents of the upper 3x3 portion of the matrix
    glm::mat3 upper = glm::mat3(matrix);
    if (!assumeOrthogonal) {
        for (int i = 0; i < 10; i++) {
            // store the results of the previous iteration
            glm::mat3 previous = upper;
            
            // compute average of the matrix with its inverse transpose
            float sd00 = previous[1][1] * previous[2][2] - previous[2][1] * previous[1][2];
            float sd10 = previous[0][1] * previous[2][2] - previous[2][1] * previous[0][2];
            float sd20 = previous[0][1] * previous[1][2] - previous[1][1] * previous[0][2];
            float det = previous[0][0] * sd00 + previous[2][0] * sd20 - previous[1][0] * sd10;
            if (fabs(det) == 0.0f) {
                // determinant is zero; matrix is not invertible
                break;
            }
            float hrdet = 0.5f / det;
            upper[0][0] = +sd00 * hrdet + previous[0][0] * 0.5f;
            upper[1][0] = -sd10 * hrdet + previous[1][0] * 0.5f;
            upper[2][0] = +sd20 * hrdet + previous[2][0] * 0.5f;

            upper[0][1] = -(previous[1][0] * previous[2][2] - previous[2][0] * previous[1][2]) * hrdet + previous[0][1] * 0.5f;
            upper[1][1] = +(previous[0][0] * previous[2][2] - previous[2][0] * previous[0][2]) * hrdet + previous[1][1] * 0.5f;
            upper[2][1] = -(previous[0][0] * previous[1][2] - previous[1][0] * previous[0][2]) * hrdet + previous[2][1] * 0.5f;

            upper[0][2] = +(previous[1][0] * previous[2][1] - previous[2][0] * previous[1][1]) * hrdet + previous[0][2] * 0.5f;
            upper[1][2] = -(previous[0][0] * previous[2][1] - previous[2][0] * previous[0][1]) * hrdet + previous[1][2] * 0.5f;
            upper[2][2] = +(previous[0][0] * previous[1][1] - previous[1][0] * previous[0][1]) * hrdet + previous[2][2] * 0.5f;

            // compute the difference; if it's small enough, we're done
            glm::mat3 diff = upper - previous;
            if (diff[0][0] * diff[0][0] + diff[1][0] * diff[1][0] + diff[2][0] * diff[2][0] + diff[0][1] * diff[0][1] +
                    diff[1][1] * diff[1][1] + diff[2][1] * diff[2][1] + diff[0][2] * diff[0][2] + diff[1][2] * diff[1][2] +
                    diff[2][2] * diff[2][2] < EPSILON) {
                break;
            }
        }
    }
    
    // now that we have a nice orthogonal matrix, we can extract the rotation quaternion
    // using the method described in http://en.wikipedia.org/wiki/Rotation_matrix#Conversions
    float x2 = fabs(1.0f + upper[0][0] - upper[1][1] - upper[2][2]);
    float y2 = fabs(1.0f - upper[0][0] + upper[1][1] - upper[2][2]);
    float z2 = fabs(1.0f - upper[0][0] - upper[1][1] + upper[2][2]);
    float w2 = fabs(1.0f + upper[0][0] + upper[1][1] + upper[2][2]);  
    return glm::normalize(glm::quat(0.5f * sqrtf(w2),
        0.5f * sqrtf(x2) * (upper[1][2] >= upper[2][1] ? 1.0f : -1.0f),
        0.5f * sqrtf(y2) * (upper[2][0] >= upper[0][2] ? 1.0f : -1.0f),
        0.5f * sqrtf(z2) * (upper[0][1] >= upper[1][0] ? 1.0f : -1.0f)));
}

//  Draw a 3D vector floating in space
void drawVector(glm::vec3 * vector) {
    glDisable(GL_LIGHTING);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(3.0);
    glLineWidth(2.0);

    //  Draw axes
    glBegin(GL_LINES);
    glColor3f(1,0,0);
    glVertex3f(0,0,0);
    glVertex3f(1,0,0);
    glColor3f(0,1,0);
    glVertex3f(0,0,0);
    glVertex3f(0, 1, 0);
    glColor3f(0,0,1);
    glVertex3f(0,0,0);
    glVertex3f(0, 0, 1);
    glEnd();
        
    // Draw the vector itself
    glBegin(GL_LINES);
    glColor3f(1,1,1);
    glVertex3f(0,0,0);
    glVertex3f(vector->x, vector->y, vector->z);
    glEnd();
    
    // Draw spheres for magnitude
    glPushMatrix();
    glColor3f(1,0,0);
    glTranslatef(vector->x, 0, 0);
    glutSolidSphere(0.02, 10, 10);
    glColor3f(0,1,0);
    glTranslatef(-vector->x, vector->y, 0);
    glutSolidSphere(0.02, 10, 10);
    glColor3f(0,0,1);
    glTranslatef(0, -vector->y, vector->z);
    glutSolidSphere(0.02, 10, 10);
    glPopMatrix();

}

//  Render a 2D set of squares using perlin/fractal noise
void noiseTest(int w, int h) {
    const float CELLS = 500;
    const float NOISE_SCALE = 10.0;
    float xStep = (float) w / CELLS;
    float yStep = (float) h / CELLS;
    glBegin(GL_QUADS);    
    for (float x = 0; x < (float)w; x += xStep) {
        for (float y = 0; y < (float)h; y += yStep) {
            //  Generate a vector varying between 0-1 corresponding to the screen location 
            glm::vec2 position(NOISE_SCALE * x / (float) w, NOISE_SCALE * y / (float) h);
            //  Set the cell color using the noise value at that location
            float color = glm::perlin(position);
            glColor4f(color, color, color, 1.0);
            glVertex2f(x, y);
            glVertex2f(x + xStep, y);
            glVertex2f(x + xStep, y + yStep);
            glVertex2f(x, y + yStep);
        }
    }
    glEnd();
}
    
void renderWorldBox() {
    //  Show edge of world
    float red[] = {1, 0, 0};
    float green[] = {0, 1, 0};
    float blue[] = {0, 0, 1};
    float gray[] = {0.5, 0.5, 0.5};
    
    glDisable(GL_LIGHTING);
    glLineWidth(1.0);
    glBegin(GL_LINES);
    glColor3fv(red);
    glVertex3f(0, 0, 0);
    glVertex3f(TREE_SCALE, 0, 0);
    glColor3fv(green);
    glVertex3f(0, 0, 0);
    glVertex3f(0, TREE_SCALE, 0);
    glColor3fv(blue);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, TREE_SCALE);
    glColor3fv(gray);
    glVertex3f(0, 0, TREE_SCALE);
    glVertex3f(TREE_SCALE, 0, TREE_SCALE);
    glVertex3f(TREE_SCALE, 0, TREE_SCALE);
    glVertex3f(TREE_SCALE, 0, 0);
    glEnd();
    //  Draw marker dots at very end
    glEnable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(TREE_SCALE, 0, 0);
    glColor3fv(red);
    glutSolidSphere(0.125, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, TREE_SCALE, 0);
    glColor3fv(green);
    glutSolidSphere(0.125, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 0, TREE_SCALE);
    glColor3fv(blue);
    glutSolidSphere(0.125, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glColor3fv(gray);
    glTranslatef(TREE_SCALE, 0, TREE_SCALE);
    glutSolidSphere(0.125, 10, 10);
    glPopMatrix();

}

double diffclock(timeval *clock1,timeval *clock2)
{
	double diffms = (clock2->tv_sec - clock1->tv_sec) * 1000.0;
    diffms += (clock2->tv_usec - clock1->tv_usec) / 1000.0;   // us to ms
    
	return diffms;
}

//  Return a random vector of average length 1
const glm::vec3 randVector() {
    return glm::vec3(randFloat() - 0.5f, randFloat() - 0.5f, randFloat() - 0.5f) * 2.f;
}

static TextRenderer* textRenderer(int mono) {
    static TextRenderer* monoRenderer = new TextRenderer(MONO_FONT_FAMILY);
    static TextRenderer* proportionalRenderer = new TextRenderer(SANS_FONT_FAMILY, -1, -1, false, TextRenderer::SHADOW_EFFECT);
    return mono ? monoRenderer : proportionalRenderer;
}

int widthText(float scale, int mono, char const* string) {
    return textRenderer(mono)->computeWidth(string) * (scale / 0.10);
}

float widthChar(float scale, int mono, char ch) {
    return textRenderer(mono)->computeWidth(ch) * (scale / 0.10);
}

void drawtext(int x, int y, float scale, float rotate, float thick, int mono,
              char const* string, float r, float g, float b) {
    //
    //  Draws text on screen as stroked so it can be resized
    //
    glPushMatrix();
    glTranslatef(static_cast<float>(x), static_cast<float>(y), 0.0f);
    glColor3f(r,g,b);
    glRotated(rotate,0,0,1);
    // glLineWidth(thick);
    glScalef(scale / 0.10, scale / 0.10, 1.0);
    
    textRenderer(mono)->draw(0, 0, string);
    
    glPopMatrix();

}

void drawvec3(int x, int y, float scale, float rotate, float thick, int mono, glm::vec3 vec, float r, float g, float b) {
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
	for (i = 0; i < len; i++) {
        if (!mono) glutStrokeCharacter(GLUT_STROKE_ROMAN, int(vectext[i]));
        else glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, int(vectext[i]));
	}
    glPopMatrix();
} 

void renderCollisionOverlay(int width, int height, float magnitude) {
    const float MIN_VISIBLE_COLLISION = 0.01f;
    if (magnitude > MIN_VISIBLE_COLLISION) {
        glColor4f(0, 0, 0, magnitude);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2d(width, 0);
        glVertex2d(width, height);
        glVertex2d(0, height);
        glEnd();
    }
}

void renderMouseVoxelGrid(const float& mouseVoxelX, const float& mouseVoxelY, const float& mouseVoxelZ, const float& mouseVoxelS) {
    glm::vec3 origin = glm::vec3(mouseVoxelX, mouseVoxelY, mouseVoxelZ);

    glLineWidth(3.0);
    
    const int HALF_GRID_DIMENSIONS = 4;
    glBegin(GL_LINES);

    glm::vec3 xColor(0.0, 0.6, 0.0);
    glColor3fv(&xColor.x);

    glVertex3f(origin.x + HALF_GRID_DIMENSIONS * mouseVoxelS, 0, origin.z);
    glVertex3f(origin.x - HALF_GRID_DIMENSIONS * mouseVoxelS, 0, origin.z);

    glm::vec3 zColor(0.0, 0.0, 0.6);
    glColor3fv(&zColor.x);

    glVertex3f(origin.x, 0, origin.z + HALF_GRID_DIMENSIONS * mouseVoxelS);
    glVertex3f(origin.x, 0, origin.z - HALF_GRID_DIMENSIONS * mouseVoxelS);

    glm::vec3 yColor(0.6, 0.0, 0.0);
    glColor3fv(&yColor.x);

    glVertex3f(origin.x, 0, origin.z);
    glVertex3f(origin.x, origin.y, origin.z);
    glEnd();
}

void renderNudgeGrid(float voxelX, float voxelY, float voxelZ, float voxelS, float voxelPrecision) {
    glm::vec3 origin = glm::vec3(voxelX, voxelY, voxelZ);

    glLineWidth(1.0);
    
    const int GRID_DIMENSIONS = 4;
    const int GRID_SCALER = voxelS / voxelPrecision;
    const int GRID_SEGMENTS = GRID_DIMENSIONS * GRID_SCALER;
    glBegin(GL_LINES);

    for (int xz = - (GRID_SEGMENTS / 2); xz <= GRID_SEGMENTS / 2 + GRID_SCALER; xz++) {
        glm::vec3 xColor(0.0, 0.6, 0.0);
        glColor3fv(&xColor.x);

        glVertex3f(origin.x + GRID_DIMENSIONS * voxelS, 0, origin.z + xz * voxelPrecision);
        glVertex3f(origin.x - (GRID_DIMENSIONS - 1) * voxelS, 0, origin.z + xz * voxelPrecision);

        glm::vec3 zColor(0.0, 0.0, 0.6);
        glColor3fv(&zColor.x);

        glVertex3f(origin.x + xz * voxelPrecision, 0, origin.z + GRID_DIMENSIONS * voxelS);
        glVertex3f(origin.x + xz * voxelPrecision, 0, origin.z - (GRID_DIMENSIONS - 1) * voxelS);
    }
    glEnd();

    glColor3f(1.0f,1.0f,1.0f);
    
    glBegin(GL_POLYGON);//begin drawing of square
      glVertex3f(voxelX, 0.0f, voxelZ);//first vertex
      glVertex3f(voxelX + voxelS, 0.0f, voxelZ);//second vertex
      glVertex3f(voxelX + voxelS, 0.0f, voxelZ + voxelS);//third vertex
      glVertex3f(voxelX, 0.0f, voxelZ + voxelS);//fourth vertex
    glEnd();//end drawing of polygon
}

void renderNudgeGuide(float voxelX, float voxelY, float voxelZ, float voxelS) {
    glm::vec3 origin = glm::vec3(voxelX, voxelY, voxelZ);

    glLineWidth(3.0);
    
    glBegin(GL_LINES);

    glm::vec3 guideColor(1.0, 1.0, 1.0);
    glColor3fv(&guideColor.x);

    glVertex3f(origin.x + voxelS, 0, origin.z);
    glVertex3f(origin.x, 0, origin.z);

    glVertex3f(origin.x, 0, origin.z);
    glVertex3f(origin.x, 0, origin.z + voxelS);

    glVertex3f(origin.x + voxelS, 0, origin.z);
    glVertex3f(origin.x + voxelS, 0, origin.z + voxelS);

    glVertex3f(origin.x, 0, origin.z + voxelS);
    glVertex3f(origin.x + voxelS, 0, origin.z + voxelS);

    glEnd();
}

void renderDiskShadow(glm::vec3 position, glm::vec3 upDirection, float radius, float darkness) {

    glColor4f(0.0f, 0.0f, 0.0f, darkness);
    
    int   num = 20;
    float y  = 0.001f;
    float x2 = 0.0f;
    float z2 = radius;
    float x1;
    float z1;

    glBegin(GL_TRIANGLES);             

    for (int i=1; i<num+1; i++) {
        x1 = x2;
        z1 = z2;
        float r = ((float)i / (float)num) * PIf * 2.0;
        x2 = radius * sin(r);
        z2 = radius * cos(r);
    
            glVertex3f(position.x,      y, position.z     ); 
            glVertex3f(position.x + x1, y, position.z + z1); 
            glVertex3f(position.x + x2, y, position.z + z2); 
    }
    
    glEnd();
}



void renderSphereOutline(glm::vec3 position, float radius, int numSides, glm::vec3 cameraPosition) {
    glm::vec3 vectorToPosition(glm::normalize(position - cameraPosition));
    glm::vec3 right = glm::cross(vectorToPosition, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 up    = glm::cross(right, vectorToPosition);
    
    glBegin(GL_LINE_STRIP);             
    for (int i=0; i<numSides+1; i++) {
        float r = ((float)i / (float)numSides) * PIf * 2.0;
        float s = radius * sin(r);
        float c = radius * cos(r);
    
        glVertex3f
        (
            position.x + right.x * s + up.x * c, 
            position.y + right.y * s + up.y * c, 
            position.z + right.z * s + up.z * c 
        ); 
    }
    
    glEnd();
}


void renderCircle(glm::vec3 position, float radius, glm::vec3 surfaceNormal, int numSides) {
    glm::vec3 perp1 = glm::vec3(surfaceNormal.y, surfaceNormal.z, surfaceNormal.x);
    glm::vec3 perp2 = glm::vec3(surfaceNormal.z, surfaceNormal.x, surfaceNormal.y);
    
    glBegin(GL_LINE_STRIP);             

    for (int i=0; i<numSides+1; i++) {
        float r = ((float)i / (float)numSides) * PIf * 2.0;
        float s = radius * sin(r);
        float c = radius * cos(r);
        glVertex3f
        (
            position.x + perp1.x * s + perp2.x * c, 
            position.y + perp1.y * s + perp2.y * c, 
            position.z + perp1.z * s + perp2.z * c 
        ); 
    }
    glEnd();
}


void renderOrientationDirections(glm::vec3 position, const glm::quat& orientation, float size) {
	glm::vec3 pRight	= position + orientation * IDENTITY_RIGHT * size;
	glm::vec3 pUp		= position + orientation * IDENTITY_UP    * size;
	glm::vec3 pFront	= position + orientation * IDENTITY_FRONT * size;
		
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(pRight.x, pRight.y, pRight.z);
	glEnd();

	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(pUp.x, pUp.y, pUp.z);
	glEnd();

	glColor3f(0.0f, 0.0f, 1.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(pFront.x, pFront.y, pFront.z);
	glEnd();
}

bool closeEnoughForGovernmentWork(float a, float b) {
    float distance = std::abs(a-b);
    //qDebug("closeEnoughForGovernmentWork() a=%1.10f b=%1.10f distance=%1.10f\n",a,b,distance);
    return (distance < 0.00001f);
}

//  Do some basic timing tests and report the results
void runTimingTests() {
    //  How long does it take to make a call to get the time?
    const int numTests = 1000000;
    int iResults[numTests];
    float fTest = 1.0;
    float fResults[numTests];
    timeval startTime, endTime;
    float elapsedMsecs;
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        gettimeofday(&endTime, NULL);
    }
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("gettimeofday() usecs: %f\n", 1000.0f * elapsedMsecs / (float) numTests);
    
    // Random number generation
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        iResults[i] = rand();
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("rand() stored in array usecs: %f\n", 1000.0f * elapsedMsecs / (float) numTests);

    // Random number generation using randFloat()
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        fResults[i] = randFloat();
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("randFloat() stored in array usecs: %f\n", 1000.0f * elapsedMsecs / (float) numTests);

    //  PowF function
    fTest = 1145323.2342f;
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        fTest = powf(fTest, 0.5f);
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("powf(f, 0.5) usecs: %f\n", 1000.0f * elapsedMsecs / (float) numTests);

    //  Vector Math
    float distance;
    glm::vec3 pointA(randVector()), pointB(randVector());
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        //glm::vec3 temp = pointA - pointB;
        //float distanceSquared = glm::dot(temp, temp);
        distance = glm::distance(pointA, pointB);
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("vector math usecs: %f [%f msecs total for %d tests]\n", 
             1000.0f * elapsedMsecs / (float) numTests, elapsedMsecs, numTests);
    
    //  Vec3 test
    glm::vec3 vecA(randVector()), vecB(randVector());
    float result;
    
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        glm::vec3 temp = vecA-vecB;
        result = glm::dot(temp,temp);
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("vec3 assign and dot() usecs: %f\n", 1000.0f * elapsedMsecs / (float) numTests);
}

float loadSetting(QSettings* settings, const char* name, float defaultValue) {
    float value = settings->value(name, defaultValue).toFloat();
    if (isnan(value)) {
        value = defaultValue;
    }
    return value;
}

bool rayIntersectsSphere(const glm::vec3& rayStarting, const glm::vec3& rayNormalizedDirection,
        const glm::vec3& sphereCenter, float sphereRadius, float& distance) {
    glm::vec3 relativeOrigin = rayStarting - sphereCenter;
    
    // compute the b, c terms of the quadratic equation (a is dot(direction, direction), which is one)
    float b = 2.0f * glm::dot(rayNormalizedDirection, relativeOrigin);
    float c = glm::dot(relativeOrigin, relativeOrigin) - sphereRadius * sphereRadius;
    
    // compute the radicand of the quadratic.  if less than zero, there's no intersection
    float radicand = b * b - 4.0f * c;
    if (radicand < 0.0f) {
        return false;
    }
    
    // compute the first solution of the quadratic
    float root = sqrtf(radicand);
    float firstSolution = -b - root;
    if (firstSolution > 0.0f) {
        distance = firstSolution / 2.0f; 
        return true; // origin is outside the sphere
    }
    
    // now try the second solution
    float secondSolution = -b + root;
    if (secondSolution > 0.0f) {
        distance = 0.0f;
        return true; // origin is inside the sphere
    }
    
    return false;
}

bool pointInSphere(glm::vec3& point, glm::vec3& sphereCenter, double sphereRadius) {
    glm::vec3 diff = point - sphereCenter;
    double mag = sqrt(glm::dot(diff, diff));
    if (mag <= sphereRadius) {
        return true;
    }
    return false;
}
