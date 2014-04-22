//
//  Util.h
//  interface/src
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Util_h
#define hifi_Util_h

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

void drawText(int x, int y, float scale, float radians, int mono,
              char const* string, const float* color);

void drawvec3(int x, int y, float scale, float radians, float thick, int mono, glm::vec3 vec, 
              float r=1.0, float g=1.0, float b=1.0);

void drawVector(glm::vec3* vector);

void printVector(glm::vec3 vec);

void renderCollisionOverlay(int width, int height, float magnitude, float red = 0, float blue = 0, float green = 0);

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

#endif // hifi_Util_h
