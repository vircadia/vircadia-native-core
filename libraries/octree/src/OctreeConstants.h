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

#include <QtCore/QString> // for quint64/QString
#include <glm/glm.hpp>

const quint64 CHANGE_FUDGE = 1000 * 200; // useconds of fudge in determining if we want to resend changed voxels

const int TREE_SCALE = 32768; // ~20 miles.. This is the number of meters of the 0.0 to 1.0 voxel universe
const int HALF_TREE_SCALE = TREE_SCALE / 2;

// This controls the LOD. Larger number will make smaller voxels visible at greater distance.
const float DEFAULT_OCTREE_SIZE_SCALE = TREE_SCALE * 400.0f; 

// Since entities like models live inside of octree cells, and they themselves can have very small mesh parts,
// we want to have some constant that controls have big a mesh part must be to render even if the octree cell itself
// would be visible. This constanct controls that. It basically means you must be this many times closer to a mesh 
// than an octree cell to see the mesh.
const float OCTREE_TO_MESH_RATIO = 4.0f;

// This is used in the LOD Tools to translate between the size scale slider and the values used to set the OctreeSizeScale
const float MAX_LOD_SIZE_MULTIPLIER = 4000.0f;

const int NUMBER_OF_CHILDREN = 8;

const int MAX_TREE_SLICE_BYTES = 26;

const float VIEW_FRUSTUM_FOV_OVERSEND = 60.0f;

// These are guards to prevent our voxel tree recursive routines from spinning out of control
const int UNREASONABLY_DEEP_RECURSION = 29; // use this for something that you want to be shallow, but not spin out
const int DANGEROUSLY_DEEP_RECURSION = 200; // use this for something that needs to go deeper
const float SCALE_AT_UNREASONABLY_DEEP_RECURSION = (TREE_SCALE / powf(2.0f, UNREASONABLY_DEEP_RECURSION));
const float SCALE_AT_DANGEROUSLY_DEEP_RECURSION = (TREE_SCALE / powf(2.0f, DANGEROUSLY_DEEP_RECURSION));
const float SMALLEST_REASONABLE_OCTREE_ELEMENT_SCALE = SCALE_AT_UNREASONABLY_DEEP_RECURSION * 2.0f; // 0.00001525878 meter ~1/10,0000th

const int DEFAULT_MAX_OCTREE_PPS = 600; // the default maximum PPS we think any octree based server should send to a client

#endif // hifi_OctreeConstants_h
