//
//  Util.h
//  interface
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Util__
#define __interface__Util__

#ifdef _WIN32
#include "Systime.h"
#else
#include <sys/time.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QSettings>

void eulerToOrthonormals(glm::vec3 * angles, glm::vec3 * fwd, glm::vec3 * left, glm::vec3 * up);

float azimuth_to(glm::vec3 head_pos, glm::vec3 source_pos);
float angle_to(glm::vec3 head_pos, glm::vec3 source_pos, float render_yaw, float head_yaw);

float randFloat();
const glm::vec3 randVector();

void renderWorldBox();
int widthText(float scale, int mono, char const* string);
float widthChar(float scale, int mono, char ch);
void drawtext(int x, int y, float scale, float rotate, float thick, int mono, 
              char const* string, float r=1.0, float g=1.0, float b=1.0);
void drawvec3(int x, int y, float scale, float rotate, float thick, int mono, glm::vec3 vec, 
              float r=1.0, float g=1.0, float b=1.0);

void drawVector(glm::vec3* vector);

void printVector(glm::vec3 vec);

float angleBetween(const glm::vec3& v1, const glm::vec3& v2); 

glm::quat rotationBetween(const glm::vec3& v1, const glm::vec3& v2);

glm::vec3 extractTranslation(const glm::mat4& matrix);

void setTranslation(glm::mat4& matrix, const glm::vec3& translation);

glm::quat extractRotation(const glm::mat4& matrix, bool assumeOrthogonal = false);

glm::vec3 extractScale(const glm::mat4& matrix);

float extractUniformScale(const glm::mat4& matrix);

float extractUniformScale(const glm::vec3& scale);

double diffclock(timeval *clock1,timeval *clock2);

void renderCollisionOverlay(int width, int height, float magnitude);

void renderOrientationDirections( glm::vec3 position, const glm::quat& orientation, float size );

void renderSphereOutline(glm::vec3 position, float radius, int numSides, glm::vec3 cameraPosition);
void renderCircle(glm::vec3 position, float radius, glm::vec3 surfaceNormal, int numSides );
void renderRoundedCornersRect(int x, int y, int width, int height, int radius, int numPointsCorner);
void renderBevelCornersRect(int x, int y, int width, int height, int bevelDistance);

void runTimingTests();

float loadSetting(QSettings* settings, const char* name, float defaultValue);

bool rayIntersectsSphere(const glm::vec3& rayStarting, const glm::vec3& rayNormalizedDirection,
    const glm::vec3& sphereCenter, float sphereRadius, float& distance);

bool pointInSphere(glm::vec3& point, glm::vec3& sphereCenter, double sphereRadius);

#endif
