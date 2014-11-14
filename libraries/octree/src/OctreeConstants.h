//
//  OctreeConstants.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 4/29/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeConstants_h
#define hifi_OctreeConstants_h

#include <QtGlobal> // for quint64
#include <glm/glm.hpp>

// this is where the coordinate system is represented
const glm::vec3 IDENTITY_RIGHT = glm::vec3( 1.0f, 0.0f, 0.0f);
const glm::vec3 IDENTITY_UP    = glm::vec3( 0.0f, 1.0f, 0.0f);
const glm::vec3 IDENTITY_FRONT = glm::vec3( 0.0f, 0.0f,-1.0f);

const quint64 CHANGE_FUDGE = 1000 * 200; // useconds of fudge in determining if we want to resend changed voxels

const int   TREE_SCALE = 16384; // ~10 miles.. This is the number of meters of the 0.0 to 1.0 voxel universe

// This controls the LOD. Larger number will make smaller voxels visible at greater distance.
const float DEFAULT_OCTREE_SIZE_SCALE = TREE_SCALE * 400.0f; 
const float MAX_LOD_SIZE_MULTIPLIER = 2000.0f;

const int NUMBER_OF_CHILDREN = 8;

const int MAX_TREE_SLICE_BYTES = 26;

const float VIEW_FRUSTUM_FOV_OVERSEND = 60.0f;

// These are guards to prevent our voxel tree recursive routines from spinning out of control
const int UNREASONABLY_DEEP_RECURSION = 29; // use this for something that you want to be shallow, but not spin out
const int DANGEROUSLY_DEEP_RECURSION = 200; // use this for something that needs to go deeper
const float SCALE_AT_UNREASONABLY_DEEP_RECURSION = (1.0f / powf(2.0f, UNREASONABLY_DEEP_RECURSION));
const float SCALE_AT_DANGEROUSLY_DEEP_RECURSION = (1.0f / powf(2.0f, DANGEROUSLY_DEEP_RECURSION));
const float SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE = SCALE_AT_UNREASONABLY_DEEP_RECURSION * 2.0f; // 0.00006103515 meter ~1/10,0000th

const int DEFAULT_MAX_OCTREE_PPS = 600; // the default maximum PPS we think any octree based server should send to a client

#endif // hifi_OctreeConstants_h
