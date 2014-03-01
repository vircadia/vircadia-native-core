//
//  Render2.cpp
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//


#include "svoviewer.h"

/////////////////////////////////////////////////////////////////////////////
// Helper functions

// Precision dependent hack. After debugging - change this to a magnitude function.
// simple version for clarity/debugging.
int SvoViewer::ptCompFunc(const void * a, const void * b)
{
	if ((*(glm::vec3*)a).x < (*(glm::vec3*)b).x) return -1;
	else if ((*(glm::vec3*)a).x > (*(glm::vec3*)b).x) return 1;

	if ((*(glm::vec3*)a).y < (*(glm::vec3*)b).y) return -1; 
	else if ((*(glm::vec3*)a).y > (*(glm::vec3*)b).y) return 1;

	if ((*(glm::vec3*)a).z < (*(glm::vec3*)b).z) return -1; 
	else if ((*(glm::vec3*)a).z > (*(glm::vec3*)b).z) return 1;
	return 0;
}

//#define PRECISION_ERR .00000001
#define PRECISION_ERR .00001
// aggressive mode
//(0.00097656250 /2) // Space of smallest voxel should define our error bounds here. Test this if time allows.
int SvoViewer::ptCloseEnough(const void * a, const void * b)
{
	glm::vec3 diffVec = (*(glm::vec3*)a) - (*(glm::vec3*)b);
	if (fabs(diffVec.x) < PRECISION_ERR && fabs(diffVec.y) < PRECISION_ERR && fabs(diffVec.z) < PRECISION_ERR) return 0;
	//float len = diffVec.length();
	//if (len < PRECISION_ERR) return 0;
	if ((*(glm::vec3*)a).x < (*(glm::vec3*)b).x) return -1;
	else if ((*(glm::vec3*)a).x > (*(glm::vec3*)b).x) return 1;
	if ((*(glm::vec3*)a).y < (*(glm::vec3*)b).y) return -1; 
	else if ((*(glm::vec3*)a).y > (*(glm::vec3*)b).y) return 1;
	if ((*(glm::vec3*)a).z < (*(glm::vec3*)b).z) return -1; 
	else if ((*(glm::vec3*)a).z > (*(glm::vec3*)b).z) return 1;
	return 0;
}

// return parameterized intersection in t.
bool SvoViewer::parameterizedRayPlaneIntersection(const glm::vec3 origin, const glm::vec3 direction, const glm::vec3 planePt, const glm::vec3 planeNormal, float *t)
{
	float denom = glm::dot(direction, planeNormal);
	if (denom < PRECISION_ERR) return false;

	glm::vec3 p010 = planePt - origin;
	*t = glm::dot(p010, planeNormal) / denom;
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// 2nd stab at optimizing this. Cull back faces more aggressively.


struct VoxelOpt2RenderAssembleData
{
	Vertex*			vtxBuffer;
	VoxelDimIdxSet*	idxSet;
	int				vtxCount;
	int				faceCenterCount;
	glm::vec3 *		faceCenterList;
	int				discardedCount;
};

bool SvoViewer::VoxelOpt2RenderAssemblePerVoxel(OctreeElement* node, void* extraData)
{
	VoxelTreeElement* voxel = (VoxelTreeElement*)node;
    VoxelOpt2RenderAssembleData* args = (VoxelOpt2RenderAssembleData*)extraData;
	if (!node->isLeaf()) return true;

	AABox box = voxel->getAABox();

	glm::vec3 p0, p1, p2, p3, hackCenterVal;
	glm::vec3 cubeVerts[GLOBAL_NORMALS_VERTICES_PER_VOXEL];
	for (int i = 0; i < GLOBAL_NORMALS_VERTICES_PER_VOXEL; i++) // Cache, as aabox reconstructs with every call.
		cubeVerts[i] = box.getVertex((BoxVertex)i);
	
	bool doAddFace[NUM_CUBE_FACES] = {true, false, true, true, true, true};  // Cull bottom faces by default.
	
	// Accumulate all the faces that need to be added.
	for (int i = 0; i < NUM_CUBE_FACES; i++)
	{
		p0 = cubeVerts[SvoViewerNames::cubeFaceVtxs[i][0]];
		p1 = cubeVerts[SvoViewerNames::cubeFaceVtxs[i][1]];
		p2 = cubeVerts[SvoViewerNames::cubeFaceVtxs[i][2]];
		p3 = cubeVerts[SvoViewerNames::cubeFaceVtxs[i][3]];
		hackCenterVal = computeQuickAndDirtyQuadCenter(p0, p1, p2, p3);
		// Search for this in the face center list
		//glm::vec3 * foundVal = (glm::vec3*)bsearch(&hackCenterVal, args->faceCenterList, args->faceCenterCount, sizeof(glm::vec3), ptCompFunc);
		// BSEARCH FAILING! What the? wrote my own index approximate version.
		int foundBVS = 0;
		int idxIntoList = binVecSearch(hackCenterVal, args->faceCenterList, args->faceCenterCount, &foundBVS);
		if (foundBVS == 0) { assert(0); continue; } // What the? 
		assert(idxIntoList <= args->faceCenterCount-1);
		// Now check face center list values that are immmediately adjacent to this value. If they're equal, don't add this face as
		// another leaf voxel must contain this triangle too.
		bool foundMatch = false;
		if (idxIntoList != 0) 
		{	
			if (ptCloseEnough(&hackCenterVal, &args->faceCenterList[idxIntoList-1]) == 0) foundMatch = true;
		}
		if (idxIntoList != args->faceCenterCount-1 && foundMatch == false)
		{
			if (ptCloseEnough(&hackCenterVal, &args->faceCenterList[idxIntoList+1]) == 0) foundMatch = true;
		}
		if (foundMatch)
		{
			doAddFace[i] = false; // Remove.
			args->discardedCount++;
		}
	}

#define VTX_NOT_USED 255
	unsigned char vtxToAddMap[GLOBAL_NORMALS_VERTICES_PER_VOXEL]; // Map from vertex to actual position in the new vtx list.
	memset(vtxToAddMap, VTX_NOT_USED, sizeof(vtxToAddMap));

	// Figure out what vertices to add. NOTE - QUICK and dirty. easy opt - precalulate bit pattern for every face and just & it.
	bool useVtx[GLOBAL_NORMALS_VERTICES_PER_VOXEL];
	memset(useVtx, 0, sizeof(useVtx));
	for ( int face = 0; face < NUM_CUBE_FACES; face++) // Vertex add order.
	{
		if (doAddFace[face])
		{
			for (int vOrder = 0; vOrder < 4; vOrder++)
			{
				useVtx[SvoViewerNames::cubeFaceVtxs[face][vOrder]] = true;
			}
		}
	}
	unsigned char vtxAddedCount = 0;
	int baseVtxCount = args->vtxCount;
	for (int i = 0; i < GLOBAL_NORMALS_VERTICES_PER_VOXEL; i++)
	{
		if (useVtx[i])
		{
			vtxToAddMap[i] = vtxAddedCount;
			vtxAddedCount++;

			args->vtxBuffer[args->vtxCount].position = cubeVerts[i];
			args->vtxBuffer[args->vtxCount].position *= 100;
			args->vtxBuffer[args->vtxCount].position.x -= 25;
			args->vtxBuffer[args->vtxCount].position.y -= 4;
			args->vtxBuffer[args->vtxCount].color[0] = voxel->getTrueColor()[0];		
			args->vtxBuffer[args->vtxCount].color[1] = voxel->getTrueColor()[1];		
			args->vtxBuffer[args->vtxCount].color[2] = voxel->getTrueColor()[2];		
			args->vtxBuffer[args->vtxCount].color[3] = 1;		
			cubeVerts[i] = args->vtxBuffer[args->vtxCount].position;
			args->vtxCount++;
		}
	}

	// Assemble the index lists.
	for ( int face = 0; face < NUM_CUBE_FACES; face++)
	{
		if (doAddFace[face])
		{
			for (int i = 0; i < 6; i++) // 2 * 3 triangles.
			{
				args->idxSet->idxBuff[face][args->idxSet->idxCount[face]] = baseVtxCount + vtxToAddMap[ SvoViewerNames::cubeFaceIndices[face][i] ];
				args->idxSet->idxCount[face]++;
			}			
			for (int i = 0; i < 4; i++)
			{				
				args->idxSet->bounds[face].AddToSet( cubeVerts[SvoViewerNames::cubeFaceVtxs[face][i]] );
			}
			args->idxSet->elemCount[face] += 2;
		}
	}

	return true;
}

void SvoViewer::InitializeVoxelOpt2RenderSystem()
{
    quint64 startInit = usecTimestampNow();
	if (_voxelOptRenderInitialized) return;
	_numSegments = 0;
	_totalPossibleElems = 0;
	memset(_numChildNodeLeaves, 0, sizeof(_numChildNodeLeaves));
	memset(_segmentNodeReferences, 0, sizeof(_segmentNodeReferences));

	// Set up the segments. Find the number of leaves at each subtree. 
	OctreeElement * rootNode = _systemTree.getRoot();
	OctreeElement* node0fromRoot = rootNode->getChildAtIndex(0); // ALL the interesting data for our test SVO is in this node! HACK!!
	//int rootNumChildren = rootNode->getChildCount();
	for (int i = 0; i < NUMBER_OF_CHILDREN; i++)
	{		
		OctreeElement* childNode1stOrder = node0fromRoot->getChildAtIndex(i);
		if (childNode1stOrder == NULL) continue;
		// Grab 2nd order nodes for better separation. At some point, this would need to be done intelligently.
		for (int j = 0; j < NUMBER_OF_CHILDREN; j++)
		{			
			OctreeElement* childNode2ndOrder = childNode1stOrder->getChildAtIndex(j);
			if (childNode2ndOrder == NULL) continue;

			//int num2ndOrderChildren = childNode2ndOrder->getChildCount();
			// Figure out how populated this child is.
			FindNumLeavesData data;
			data.numLeaves = 0;
			_systemTree.recurseNodeWithOperation(childNode2ndOrder, &FindNumLeaves, &data, 0);

			// Some of these nodes have a single leaf. Ignore for the moment. We really only want the big segments in this test. Add this back in at some point.
			if (data.numLeaves > 1)
			{
				_numChildNodeLeaves[_numSegments] = data.numLeaves;
				_segmentNodeReferences[_numSegments] = childNode2ndOrder;
				_totalPossibleElems += data.numLeaves * NUM_CUBE_FACES * 2;
				_numSegments++;
				qDebug("child node %d %d has %d leaves and %d children itself\n", i, j, data.numLeaves, childNode2ndOrder->getChildCount());
				if (_numSegments >= MAX_NUM_OCTREE_PARTITIONS ) { qDebug("Out of segment space??? What the?\n"); break; }
			}
		}
		if (_numSegments >= MAX_NUM_OCTREE_PARTITIONS ) { qDebug("Out of segment space??? What the?\n"); break; }
	}


	// Set up the VBO's. Once for every partition we stored.
	for (int i = 0; i < _numSegments; i++)
	{
		// compute the visible set of this segment first.
		glm::vec3* faceCenters = new glm::vec3[NUM_CUBE_FACES *_numChildNodeLeaves[i]]; 
		VisibleFacesData visFaceData;
		visFaceData.ptList = faceCenters;
		visFaceData.count = 0;
		_systemTree.recurseNodeWithOperation(_segmentNodeReferences[i], &TrackVisibleFaces, &visFaceData, 0);
		// Now there's a list of all the face centers. Sort it.
		qsort(faceCenters, visFaceData.count, sizeof(glm::vec3), ptCompFunc);
		qDebug("Creating VBO's. Sorted neighbor list %d\n", i);

		_readVertexStructs = new Vertex[GLOBAL_NORMALS_VERTICES_PER_VOXEL * _numChildNodeLeaves[i]];
		memset(&_segmentIdxBuffers[i], 0, sizeof(VoxelDimIdxSet)); // Don't do it this way if we ever use a vtable for AABoundingVolumes!
		for (int k = 0; k < NUM_CUBE_FACES; k++)
		{
			_segmentIdxBuffers[i].idxBuff[k] = new GLuint[2 * 3 * _numChildNodeLeaves[i]];
			assert(_segmentIdxBuffers[i].idxBuff[k] != NULL);
		}

		VoxelOpt2RenderAssembleData args;
		args.vtxBuffer = _readVertexStructs;
		args.vtxCount  = 0;
		args.faceCenterCount = visFaceData.count;
		args.faceCenterList = visFaceData.ptList;
		args.discardedCount = 0;
		args.idxSet = &_segmentIdxBuffers[i];
		_systemTree.recurseNodeWithOperation(_segmentNodeReferences[i], &VoxelOpt2RenderAssemblePerVoxel, &args, 0);

		SetupGlVBO(&_vboOVerticesIds[i], args.vtxCount * sizeof(Vertex), GL_ARRAY_BUFFER, GL_STATIC_DRAW, _readVertexStructs);
		unsigned int idxCount = 0;
		for (int k = 0; k < NUM_CUBE_FACES; k++)
		{
			SetupGlVBO(&_segmentIdxBuffers[i].idxIds[k], _segmentIdxBuffers[i].idxCount[k] * sizeof(GLuint), 
				GL_ARRAY_BUFFER, GL_STATIC_DRAW, _segmentIdxBuffers[i].idxBuff[k]);
			idxCount += _segmentIdxBuffers[i].idxCount[k];
			_segmentIdxBuffers[i].bounds[k].setIsSingleDirection(true, SvoViewerNames::faceNormals[k]);
		}
 
		qDebug("Partition %d, vertices %d, indices %d, discarded %d\n", i, args.vtxCount, idxCount, args.discardedCount);

		delete [] _readVertexStructs;
		//delete [] _readIndicesArray;
		delete [] faceCenters;
		for (int k = 0; k < NUM_CUBE_FACES; k++) if (_segmentIdxBuffers[i].idxBuff[k] != NULL) delete [] _segmentIdxBuffers[i].idxBuff[k];
	}

	_voxelOptRenderInitialized = true;

	UpdateOpt2BVFaceVisibility();

    quint64 endInit = usecTimestampNow();
    quint64 elapsed = endInit - startInit;
    qDebug() << "init elapsed:" << ((float)elapsed / (float)1000000.0f) << "seconds";
    
}

void SvoViewer::RenderTreeSystemAsOpt2Voxels()
{
	glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	// disable specular lighting for ground and voxels
    glMaterialfv(GL_FRONT, GL_SPECULAR, NO_SPECULAR_COLOR);

	setupWorldLight();
	_numElemsDrawn = 0;
	for (int i = 0; i < _numSegments; i++)
	{
		if (_displayOnlyPartition == i || _displayOnlyPartition == NO_PARTITION )
		{
			glBindBuffer(GL_ARRAY_BUFFER, _vboOVerticesIds[i]);
			glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,position));
			glEnableVertexAttribArray(ATTRIB_POSITION);

			glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,color));
			glEnableVertexAttribArray(ATTRIB_COLOR);

			//glVertexPointer(3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex,position));
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex,color));

			for (int j = 0; j < NUM_CUBE_FACES; j++)
			{
				 // Add aggressive LOD check here.
				if (_segmentIdxBuffers[i].visibleFace[j] || _useBoundingVolumes != true)
				{
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _segmentIdxBuffers[i].idxIds[j]);//_vboOIndicesIds[i]);
					glDrawElements(GL_TRIANGLES, _segmentIdxBuffers[i].elemCount[j]*3, GL_UNSIGNED_INT, NULL);
					_numElemsDrawn += _segmentIdxBuffers[i].elemCount[j];
				}
			}
		}
	}
	glDisableVertexAttribArray(ATTRIB_POSITION);
	glDisableVertexAttribArray(ATTRIB_COLOR);	
}

// special rules for single direction bv sets. Basically, we intersect a lookat ray from the camera with two opposite faces and discard
// the entire set of the face that is further away as it must be back facing.
void SvoViewer::UpdateOpt2BVFaceVisibility()
{
	if (_currentShaderModel != RENDER_OPT_CULLED_POLYS || _voxelOptRenderInitialized != true ) return; 

	//float faceParamVals[NUM_CUBE_FACES];
	glm::vec3 pos = _myCamera.getPosition();

	for (int i = 0; i < _numSegments; i++)
	{
		VoxelDimIdxSet* setPtr = &_segmentIdxBuffers[i];
		// Fast cull check.
		setPtr->visibleFace[0] = (_segmentIdxBuffers[i].bounds[0].within(pos.y, 1) >= 0) ? true : false;
		setPtr->visibleFace[1] = (_segmentIdxBuffers[i].bounds[1].within(pos.y, 1) <= 0) ? true : false;
		setPtr->visibleFace[2] = (_segmentIdxBuffers[i].bounds[2].within(pos.x, 0) >= 0) ? true : false;
		setPtr->visibleFace[3] = (_segmentIdxBuffers[i].bounds[3].within(pos.x, 0) <= 0) ? true : false;
		setPtr->visibleFace[4] = (_segmentIdxBuffers[i].bounds[4].within(pos.z, 2) <= 0) ? true : false;
		setPtr->visibleFace[5] = (_segmentIdxBuffers[i].bounds[5].within(pos.z, 2) >= 0) ? true : false;

		// Make sure its actually on the screen.
		/*for (int j = 0; j < NUM_CUBE_FACES; j++)
		{
			if (setPtr->visibleFace[j])
			{
				if (visibleAngleSubtended(&_segmentIdxBuffers[i].bounds[j], &_myCamera, &_viewFrustum) <= 0)
					setPtr->visibleFace[j] = false;
			}
		}*/
	}
		/*
		for (int j = 0; j < NUM_CUBE_FACES; j++)
		{
			setPtr->visibleFace[i] = true;
			AABoundingVolume* volume = &_segmentIdxBuffers[i].bounds[j];
			glm::vec3 randomPlaneVtx = volume->getCorner((BoxVertex)SvoViewerNames::cubeFaceIndices[j][0]);
			glm::vec3 raydir = randomPlaneVtx - pos;
			rayder /= glm::length(raydir);
			if (glm::dot(target, raydir) < 1) raydir *= -1;
			if (!parameterizedRayPlaneIntersection(pos, raydir, randomPlaneVtx, SvoViewerNames::faceNormals[j], &faceParamVals[j]))
				faceParamVals[j] = -1;		
		}
		*/
}

void SvoViewer::StopUsingVoxelOpt2RenderSystem()
{
	glDisableVertexAttribArray(ATTRIB_POSITION);
	glDisableVertexAttribArray(ATTRIB_COLOR);	
	glDisable(GL_LIGHTING);
}
