//
//  VoxelConstants.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 4/29/13.
//
//
//  Various important constants used throughout the system related to voxels
//
//  

#ifndef __hifi_VoxelConstants_h__
#define __hifi_VoxelConstants_h__

#include <limits.h>
#include <OctalCode.h>

// this is where the coordinate system is represented
const glm::vec3 IDENTITY_RIGHT = glm::vec3( 1.0f, 0.0f, 0.0f);
const glm::vec3 IDENTITY_UP    = glm::vec3( 0.0f, 1.0f, 0.0f);
const glm::vec3 IDENTITY_FRONT = glm::vec3( 0.0f, 0.0f,-1.0f);

const bool LOW_RES_MONO = false; // while in "low res mode" do voxels switch to monochrome
const uint64_t CHANGE_FUDGE = 1000*200; // useconds of fudge in determining if we want to resend changed voxels

const int TREE_SCALE = 128;

const int NUMBER_OF_CHILDREN = 8;
const int MAX_VOXEL_PACKET_SIZE = 1492;
const int MAX_TREE_SLICE_BYTES = 26;
const int MAX_VOXELS_PER_SYSTEM = 200000;
const int VERTICES_PER_VOXEL = 24;
const int VERTEX_POINTS_PER_VOXEL = 3 * VERTICES_PER_VOXEL;
const int INDICES_PER_VOXEL = 3 * 12;
const int COLOR_VALUES_PER_VOXEL = NUMBER_OF_COLORS * VERTICES_PER_VOXEL;

typedef unsigned long int glBufferIndex;
const glBufferIndex GLBUFFER_INDEX_UNKNOWN = ULONG_MAX;

const float SIXTY_FPS_IN_MILLISECONDS = 1000.0f / 60.0f;
const float VIEW_CULLING_RATE_IN_MILLISECONDS = 1000.0f; // once a second is fine

#endif
