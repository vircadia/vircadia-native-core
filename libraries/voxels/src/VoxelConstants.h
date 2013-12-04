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
#include <stdint.h>
#include <OctalCode.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <glm/glm.hpp>
#include <OctreeConstants.h>

// this is where the coordinate system is represented

const int DEFAULT_MAX_VOXELS_PER_SYSTEM = 500000;
const int VERTICES_PER_VOXEL = 24; // 6 sides * 4 corners per side
const int VERTEX_POINTS_PER_VOXEL = 3 * VERTICES_PER_VOXEL; // xyz for each VERTICE_PER_VOXEL
const int INDICES_PER_VOXEL = 3 * 12; // 6 sides * 2 triangles per size * 3 vertices per triangle
const int COLOR_VALUES_PER_VOXEL = NUMBER_OF_COLORS * VERTICES_PER_VOXEL;

const int VERTICES_PER_FACE = 4; // 6 sides * 4 corners per side
const int INDICES_PER_FACE = 3 * 2; // 1 side * 2 triangles per size * 3 vertices per triangle
const int GLOBAL_NORMALS_VERTICES_PER_VOXEL = 8; // no need for 3 copies because they don't include normal
const int GLOBAL_NORMALS_VERTEX_POINTS_PER_VOXEL = 3 * GLOBAL_NORMALS_VERTICES_PER_VOXEL;
const int GLOBAL_NORMALS_COLOR_VALUES_PER_VOXEL = NUMBER_OF_COLORS * GLOBAL_NORMALS_VERTICES_PER_VOXEL;

typedef unsigned long int glBufferIndex;
const glBufferIndex GLBUFFER_INDEX_UNKNOWN = ULONG_MAX;

const float SIXTY_FPS_IN_MILLISECONDS = 1000.0f / 60.0f;
const float VIEW_CULLING_RATE_IN_MILLISECONDS = 1000.0f; // once a second is fine

const uint64_t CLIENT_TO_SERVER_VOXEL_SEND_INTERVAL_USECS = 1000 * 5; // 1 packet every 50 milliseconds


const int DEFAULT_MAX_VOXEL_PPS = 600; // the default maximum PPS we think a voxel server should send to a client

#endif