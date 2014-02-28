//
//  globals.cpp
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//


#include "globals.h"

const GLfloat WHITE_SPECULAR_COLOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat NO_SPECULAR_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };

namespace SvoViewerNames // avoid conflicts with voxelSystem namespace objects
{
float identityVerticesGlobalNormals[] = { 0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1 };

float identityVertices[] = { 0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1, //0-7
                             0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1, //8-15
                             0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1 }; // 16-23

GLfloat identityNormals[] = { 0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
                              0,0,+1, 0,0,+1, 0,0,+1, 0,0,+1,
                              0,-1,0, 0,-1,0, 0,+1,0, 0,+1,0,
                              0,-1,0, 0,-1,0, 0,+1,0, 0,+1,0,
                              -1,0,0, +1,0,0, +1,0,0, -1,0,0,
                              -1,0,0, +1,0,0, +1,0,0, -1,0,0 };

GLubyte identityIndices[] = { 0,2,1,    0,3,2,    // Z-
                              8,9,13,   8,13,12,  // Y-
                              16,23,19, 16,20,23, // X-
                              17,18,22, 17,22,21, // X+
                              10,11,15, 10,15,14, // Y+
                              4,5,6,    4,6,7 };  // Z+

GLubyte identityIndicesTop[]    = {  2, 3, 7,  2, 7, 6 };
GLubyte identityIndicesBottom[] = {  0, 1, 5,  0, 5, 4 };
GLubyte identityIndicesLeft[]   = {  0, 7, 3,  0, 4, 7 };
GLubyte identityIndicesRight[]  = {  1, 2, 6,  1, 6, 5 };
GLubyte identityIndicesFront[]  = {  0, 2, 1,  0, 3, 2 };
GLubyte identityIndicesBack[]   = {  4, 5, 6,  4, 6, 7 };

GLubyte cubeFaceIndices[][2*3] = 
{
	{  2, 3, 7,  2, 7, 6 },
	{  0, 1, 5,  0, 5, 4 },
	{  0, 7, 3,  0, 4, 7 },
	{  1, 2, 6,  1, 6, 5 },
	{  0, 2, 1,  0, 3, 2 },
	{  4, 5, 6,  4, 6, 7 }
};

GLubyte cubeFaceVtxs[6][4] =
{
	{ 2, 3, 6, 7 },
	{ 0, 1, 4, 5 },
	{ 0, 3, 4, 7 },
	{ 1, 2, 5, 6 },
	{ 0, 1, 2, 3 },
	{ 4, 5, 6, 7 }
};

glm::vec3 faceNormals[6] = 
{
	glm::vec3(0.0, 1.0, 0.0),
	glm::vec3(0.0, -1.0, 0.0),
	glm::vec3(-1.0, 0.0, 0.0),
	glm::vec3(1.0, 0.0, 0.0),
	glm::vec3(0.0, 0.0, 1.0),
	glm::vec3(0.0, 0.0, -1.0)
};

} // SvoViewerNames