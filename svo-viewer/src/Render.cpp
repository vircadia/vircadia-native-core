//
//  Render.cpp
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//



#include "svoviewer.h"

////////////////////////////////////////////////////
// Some GL helper functions

GLubyte SvoViewer::SetupGlVBO(GLuint * id, int sizeInBytes, GLenum target, GLenum usage, void * dataUp )
{
	glGenBuffers(1, id);
	glBindBuffer(target, *id);
	glBufferData(target, sizeInBytes, dataUp, usage);
	return PrintGLErrorCode();
}


////////////////////////////////////////////////////
// Include all the render methods here. 

////////////////////////////////////////////////////
// Points system
	
bool SvoViewer::FindNumLeaves(OctreeElement* node, void* extraData)
{
    FindNumLeavesData* args = (FindNumLeavesData*)extraData;
	if (node->isLeaf()) args->numLeaves++;
	return true;
}

struct PointRenderAssembleData
{
	glm::vec3 * buffer;
	unsigned char* colorBuffer;
	unsigned int count;
};

bool SvoViewer::PointRenderAssemblePerVoxel(OctreeElement* node, void* extraData)
{
    VoxelTreeElement* voxel = (VoxelTreeElement*)node;
    PointRenderAssembleData* args = (PointRenderAssembleData*)extraData;
	if (!node->isLeaf()) return true;
	AABox box = voxel->getAABox();
	glm::vec3 center = box.calcCenter();
	center += glm::vec3(0, -.05, 0);
	center *= 100.0;
	args->buffer[args->count] = center;
	int cCount = args->count * 3;
	args->colorBuffer[cCount] = voxel->getTrueColor()[0];
	args->colorBuffer[cCount+1] = voxel->getTrueColor()[1];
	args->colorBuffer[cCount+2] = voxel->getTrueColor()[2];
	args->count++;
    return true; // keep going!
}

void SvoViewer::InitializePointRenderSystem()
{
	qDebug("Initializing point render system!\n");
	quint64 fstart = usecTimestampNow();
	_renderFlags.voxelRenderDirty = true;
	_renderFlags.voxelOptRenderDirty = true;

	glGenBuffers(1, &_pointVtxBuffer);
	glBindBuffer( GL_ARRAY_BUFFER, _pointVtxBuffer);
	
	// Assemble the buffer data.
	PointRenderAssembleData args;
	args.buffer = _pointVertices = new glm::vec3[_nodeCount];
	args.colorBuffer = _pointColors = new unsigned char[_nodeCount*3];
	assert(args.buffer != NULL);
	args.count = 0;
    _systemTree.recurseTreeWithOperation(&PointRenderAssemblePerVoxel, &args);

	assert(args.count < _nodeCount);
	_pointVerticesCount = args.count;

	// create the data store.
	//int size = _nodeCount * sizeof(glm::vec3);
	glBindBuffer( GL_ARRAY_BUFFER, _pointVtxBuffer);
	glBufferData(GL_ARRAY_BUFFER, _nodeCount * 3, args.buffer, GL_STATIC_DRAW);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	//glBindBuffer(GL_ARRAY_BUFFER, _pointColorBuffer);
	//glBufferData(GL_ARRAY_BUFFER, size, args.colorBuffer, GL_STATIC_DRAW);
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	_renderFlags.ptRenderDirty = false;
	_ptRenderInitialized = true;
	float elapsed = (float)(usecTimestampNow() - fstart) / 1000.f;
	qDebug("Point render intialization took %f time for %ld nodes\n", elapsed, _nodeCount);
}

void SvoViewer::RenderTreeSystemAsPoints()
{
	
	glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_TEST);

	glColor3f(.5, 0, 0);
	glPointSize(1.0f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, _pointVtxBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glVertexPointer(3, GL_FLOAT, 0, _pointVertices);
	
	// Removed for testing.
	/*glEnableClientState(GL_COLOR_ARRAY);
	//glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, _pointColorBuffer);*/
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//glColorPointer(3, GL_UNSIGNED_BYTE, 0, _pointColors);

	glDrawArrays(GL_POINTS, 0, _pointVerticesCount);
}

void SvoViewer::StopUsingPointRenderSystem()
{
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

////////////////////////////////////////////////////

const GLchar * simpleVtxShaderSrc = 
	"uniform mat4 viewMatrix;\n\
	uniform mat4 projectionMatrix;\n\
	uniform mat4 modelMatrix;\n\
	void main()\n\
	{\n\
		vec4 world_pos = modelMatrix * gl_Vertex;\n\
		vec4 view_pos = viewMatrix * world_pos;\n\
		gl_Position = projectionMatrix * view_pos;\n\
		gl_FrontColor = gl_Color;\n\
	}\n";
int simpleVtxShaderLen = strlen(simpleVtxShaderSrc);

const GLchar * simpleGeomShaderSrc = 
	"#version 330 compatibility\n\
	layout(triangles) in;\n\
	layout(triangle_strip, max_vertices=3) out;\n\
	void main()\n\
	{\n\
		for(int i=0; i<3; i++)\n\
		{\n\
			gl_Position = gl_in[i].gl_Position;\n\
			EmitVertex();\n\
		}\n\
		EndPrimitive();\n\
	}";
int simpleGeomShaderLen = strlen(simpleGeomShaderSrc);

const GLchar * simpleFragShaderSrc = 
	"void main()\n\
	{\n\
		gl_FragColor = gl_Color;\n\
	}\n";
int simpleFragShaderLen = strlen(simpleFragShaderSrc);

struct VoxelRenderAssembleData
{
	glm::vec3 *		buffer;
	unsigned char*	colorBuffer;
	GLuint**		idxBuffs;
	unsigned int	leafCount;
	unsigned int	lastBufferSegmentStart;
	GLuint			vtxID;
	GLuint			colorID;
	GLuint*			idxIds;

};

bool SvoViewer::VoxelRenderAssemblePerVoxel(OctreeElement* node, void* extraData)
{    
	VoxelTreeElement* voxel = (VoxelTreeElement*)node;
    VoxelRenderAssembleData* args = (VoxelRenderAssembleData*)extraData;
	if (!node->isLeaf()) return true;

	AABox box = voxel->getAABox();
	int totalNodesProcessedSinceLastFlush = args->leafCount - args->lastBufferSegmentStart;
	// ack, one of these components is flags, not alpha
	int cCount = totalNodesProcessedSinceLastFlush * 4; // Place it relative to the current segment.
	unsigned char col[4] = {voxel->getTrueColor()[0], voxel->getTrueColor()[1], voxel->getTrueColor()[2], 1};
	for(int i = 0; i < GLOBAL_NORMALS_VERTICES_PER_VOXEL; i++)
		memcpy(&args->colorBuffer[cCount+i*4], col, sizeof(col));

	int vCount = totalNodesProcessedSinceLastFlush * GLOBAL_NORMALS_VERTICES_PER_VOXEL; // num vertices we've added so far.
	{
		int idxCount = totalNodesProcessedSinceLastFlush * INDICES_PER_FACE; // same for every cube face.
		// Indices follow a static pattern.
		for (int i = 0; i < NUM_CUBE_FACES; i++)
		{
			GLuint* idxBuff = args->idxBuffs[i];
			for (int j = 0; j < INDICES_PER_FACE; j++)
			{
				idxBuff[idxCount+j] = SvoViewerNames::cubeFaceIndices[i][j] + vCount;
			}
		}
	}

	// Grab vtx positions according to setup from indices layout above.	
		//center += glm::vec3(0, -.05, 0);
	float scale = 100.0;
	for ( int i = 0; i < GLOBAL_NORMALS_VERTICES_PER_VOXEL; i++)
	{
		args->buffer[vCount] = box.getVertex((BoxVertex)i); // i corresponds to BOTTOM_LEFT_NEAR,BOTTOM_RIGHT_NEAR,...
		args->buffer[vCount++] *= scale;
	}

	args->leafCount++;
	totalNodesProcessedSinceLastFlush++;

	if (totalNodesProcessedSinceLastFlush >= REASONABLY_LARGE_BUFFER) // Flush data to GL once we have assembled enough of it.
	{
		//qDebug("committing!\n");
		PrintGLErrorCode();
		glBindBuffer(GL_ARRAY_BUFFER, args->vtxID);
        glBufferSubData(GL_ARRAY_BUFFER, args->lastBufferSegmentStart * sizeof(glm::vec3) * GLOBAL_NORMALS_VERTICES_PER_VOXEL, 
			totalNodesProcessedSinceLastFlush * sizeof(glm::vec3) * GLOBAL_NORMALS_VERTICES_PER_VOXEL, args->buffer);
		PrintGLErrorCode();

		glBindBuffer(GL_ARRAY_BUFFER, args->colorID);
        glBufferSubData(GL_ARRAY_BUFFER, args->lastBufferSegmentStart * sizeof(GLubyte) * 4 * GLOBAL_NORMALS_VERTICES_PER_VOXEL, 
			totalNodesProcessedSinceLastFlush * sizeof(GLubyte) * 4 * GLOBAL_NORMALS_VERTICES_PER_VOXEL, args->colorBuffer);
		PrintGLErrorCode();

		for (int i = 0; i < NUM_CUBE_FACES; i++)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, args->idxIds[i]);
			GLuint* idxBuff = args->idxBuffs[i];
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, args->lastBufferSegmentStart * sizeof(GLuint) * INDICES_PER_FACE,
				totalNodesProcessedSinceLastFlush * sizeof(GLuint) * INDICES_PER_FACE, idxBuff); // Rework.
			PrintGLErrorCode(); 
		}
		glFlush();
		PrintGLErrorCode();
		args->lastBufferSegmentStart = args->leafCount;
	}

	return true;
}

void SvoViewer::InitializeVoxelRenderSystem()
{
	qDebug("Initializing voxel render system.\n");

	FindNumLeavesData data;
	data.numLeaves = 0;	
    _systemTree.recurseTreeWithOperation(&FindNumLeaves, &data);
	_leafCount = data.numLeaves;

	glGenBuffers(NUM_CUBE_FACES, _vboIndicesIds);
	for (int i = 0; i < NUM_CUBE_FACES; i++)
	{
		_vboIndices[i] = new GLuint[REASONABLY_LARGE_BUFFER * INDICES_PER_FACE];
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesIds[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDICES_PER_FACE * sizeof(GLuint) * _nodeCount,
                 NULL, GL_STATIC_DRAW);
	}

	glGenBuffers(1, &_vboVerticesID);
	glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesID);
	glBufferData(GL_ARRAY_BUFFER, GLOBAL_NORMALS_VERTICES_PER_VOXEL * sizeof(glm::vec3) * _nodeCount, NULL, GL_STATIC_DRAW);
	PrintGLErrorCode();

	glGenBuffers(1, &_vboColorsID);
	glBindBuffer(GL_ARRAY_BUFFER, _vboColorsID);
	glBufferData(GL_ARRAY_BUFFER, GLOBAL_NORMALS_VERTICES_PER_VOXEL * sizeof(GLubyte) * 4 * _nodeCount, NULL, GL_STATIC_DRAW);
	PrintGLErrorCode();

	//int numVertices = GLOBAL_NORMALS_VERTICES_PER_VOXEL * _nodeCount;
	// Working set test = dead after about 1.5gb! I.e... don't try to allocate this here.
	// This will tell you about what you have to play with. On my system, it died consistently after 1.3-1.5gb.
	//glm::vec3* memPtrs[20];
	//int i;
	//for (i = 0; i < 20; i++)
	//{
	//	memPtrs[i] = new glm::vec3[numVertices / 20];
	//}
	//
	//for (i = 0; i < 20; i++)
	//	delete [] memPtrs[i]; 
#define PAD 32
	_readVerticesArray = new glm::vec3[GLOBAL_NORMALS_VERTICES_PER_VOXEL * REASONABLY_LARGE_BUFFER + PAD];
	_readColorsArray = new GLubyte[(GLOBAL_NORMALS_VERTICES_PER_VOXEL * REASONABLY_LARGE_BUFFER * 4) + PAD];

	// Assemble the buffer data.
	VoxelRenderAssembleData args;
	args.buffer = _readVerticesArray;
	args.colorBuffer = _readColorsArray;
	assert(args.buffer != NULL && args.colorBuffer != NULL);
	args.leafCount	= 0;
	args.lastBufferSegmentStart = 0;
	args.idxIds		= _vboIndicesIds;
	args.idxBuffs	= _vboIndices;
	args.vtxID		= _vboVerticesID;
	args.colorID	= _vboColorsID;
    _systemTree.recurseTreeWithOperation(&VoxelRenderAssemblePerVoxel, &args);

	glBindBuffer(GL_ARRAY_BUFFER, _vboVerticesID);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
	glEnableVertexAttribArray(0);
	//glVertexPointer(3, GL_FLOAT, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, _vboColorsID);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(unsigned char) * 4, 0);
	glEnableVertexAttribArray(1);

	// Set up the shaders.
	_vertexShader = glCreateShader(GL_VERTEX_SHADER);
	_geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
	_pixelShader = glCreateShader(GL_FRAGMENT_SHADER);
	
	glShaderSource(_vertexShader, 1, &simpleVtxShaderSrc, &simpleVtxShaderLen);
	glShaderSource(_geometryShader, 1, &simpleGeomShaderSrc, &simpleGeomShaderLen);
	glShaderSource(_pixelShader, 1, &simpleFragShaderSrc, &simpleFragShaderLen);

	GLchar shaderLog[1000];
	GLsizei shaderLogLength;
	//GLint compiled;
	glCompileShaderARB((void*)_vertexShader);
	glGetShaderInfoLog(_vertexShader, 1000, &shaderLogLength, shaderLog);
	if (shaderLog[0] != 0) qDebug("Shaderlog v :\n %s\n", shaderLog);
	glCompileShaderARB((void*)_geometryShader);
	glGetShaderInfoLog(_geometryShader, 1000, &shaderLogLength, shaderLog);
	if (shaderLog[0] != 0) qDebug("Shaderlog g :\n %s\n", shaderLog);
	glCompileShaderARB((void*)_pixelShader);
	glGetShaderInfoLog(_pixelShader, 51000, &shaderLogLength, shaderLog);
	if (shaderLog[0] != 0) qDebug("Shaderlog p :\n %s\n", shaderLog);

	_linkProgram = glCreateProgram();
	glAttachShader(_linkProgram, _vertexShader);
	glAttachShader(_linkProgram, _geometryShader);
	glAttachShader(_linkProgram, _pixelShader);
	glLinkProgram(_linkProgram);     
	GLint linked;
	glGetProgramiv(_linkProgram, GL_LINK_STATUS, &linked);
	if (!linked) qDebug("Linking failed! %d\n", linked);
  

	_voxelOptRenderInitialized = true;
}

void SvoViewer::RenderTreeSystemAsVoxels()
{
	if (!_voxelOptRenderInitialized) return; // What the??
	glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	// for performance, enable backface culling
	glEnable(GL_CULL_FACE);

	// disable specular lighting for ground and voxels
    glMaterialfv(GL_FRONT, GL_SPECULAR, NO_SPECULAR_COLOR);

	setupWorldLight();

	glNormal3f(0,1.0f,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesIds[CUBE_TOP]);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _leafCount - 1,
        INDICES_PER_FACE * _leafCount, GL_UNSIGNED_INT, 0);

    glNormal3f(0,-1.0f,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesIds[CUBE_BOTTOM]);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _leafCount - 1,
        INDICES_PER_FACE * _leafCount, GL_UNSIGNED_INT, 0);

    glNormal3f(-1.0f,0,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesIds[CUBE_LEFT]);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _leafCount - 1,
        INDICES_PER_FACE * _leafCount, GL_UNSIGNED_INT, 0);

    glNormal3f(1.0f,0,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesIds[CUBE_RIGHT]);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _leafCount - 1,
        INDICES_PER_FACE * _leafCount, GL_UNSIGNED_INT, 0);

    glNormal3f(0,0,-1.0f);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesIds[CUBE_FRONT]);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _leafCount - 1,
        INDICES_PER_FACE * _leafCount, GL_UNSIGNED_INT, 0);

    glNormal3f(0,0,1.0f);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIndicesIds[CUBE_BACK]);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, GLOBAL_NORMALS_VERTICES_PER_VOXEL * _leafCount - 1,
        INDICES_PER_FACE * _leafCount, GL_UNSIGNED_INT, 0);

	//glDisable(GL_CULL_FACE);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SvoViewer::StopUsingVoxelRenderSystem()
{
	if (!_voxelOptRenderInitialized) return;
	//for (int i = 0; i < NUM_CUBE_FACES; i++) delete [] _vboIndices[i];
	//delete [] _readVerticesArray;
	//delete [] _readColorsArray;
	
	glDeleteBuffers(1, &_vboVerticesID);
	glDeleteBuffers(1, &_vboColorsID);
	glDeleteBuffers(NUM_CUBE_FACES, _vboIndicesIds);
	_voxelOptRenderInitialized = false;
}

////////////////////////////////////////////////////


// NOTE - the voxel tree data structure doesn't currently have neighbor information encoded in any convenient searchable
// way. So here I'm using a HUGE hack to compute repeated faces in the data structure. I 1) compute the barycentric coordinates
// of every triangle for every leaf face. 2) Compute the center point for the triangle. (edit: I removed the barycentric part after some testing),
// 3) That center point becomes a ID for that tri.
// 4) If that ID is added more than once, discard this triangle, as it must be an internal. External triangles will only be added once. 
// SUBNOTE - I have NO idea how the tree traversal is actually happening as I haven't looked into its usage pattern. If traversal order was intelligent,
// static and the pattern was predetermined, traversal could actually generate a mostly sorted list automatically. Because that isn't guaranteed 
// here, its uglier.


// The 
bool SvoViewer::TrackVisibleFaces(OctreeElement* node, void* extraData)
{
	VoxelTreeElement* voxel = (VoxelTreeElement*)node;
    VisibleFacesData* args = (VisibleFacesData*)extraData;
	if (!node->isLeaf()) return true;

	AABox box = voxel->getAABox();
	glm::vec3 p0, p1, p2, p3, hackCenterVal;
	glm::vec3 cubeVerts[GLOBAL_NORMALS_VERTICES_PER_VOXEL];

	for (int i = 0; i < GLOBAL_NORMALS_VERTICES_PER_VOXEL; i++) // Cache as aabox reconstructs with every call.
		cubeVerts[i] = box.getVertex((BoxVertex)i);

	for (int i = 0; i < NUM_CUBE_FACES; i++)
	{
		p0 = cubeVerts[SvoViewerNames::cubeFaceVtxs[i][0]];
		p1 = cubeVerts[SvoViewerNames::cubeFaceVtxs[i][1]];
		p2 = cubeVerts[SvoViewerNames::cubeFaceVtxs[i][2]];			
		p3 = cubeVerts[SvoViewerNames::cubeFaceVtxs[i][3]];
		hackCenterVal = computeQuickAndDirtyQuadCenter(p0, p1, p2, p3);
		args->ptList[args->count] += hackCenterVal;
		args->count++;
	}
	return true;
}

// Do a binary search on the vector list.
int SvoViewer::binVecSearch(glm::vec3 searchVal, glm::vec3* list, int count, int * found)
{
	int key = count / 2;
	int lowerbound = 0;
	int upperbound = count-1;
	int lastkey = -1;
	while (lastkey != key)
	{
		lastkey = key;
		int ret = ptCloseEnough(&searchVal, &list[key]);
		if (ret == 0) { *found = 1; return key; }
		if (lowerbound == key || upperbound == key) { *found = 0; return key;} // closest we can get!
		if (ret < 0)
		{
			upperbound = key;
			if (key == lowerbound+1) key = lowerbound;
			else key = ((upperbound - lowerbound) / 2) + lowerbound;			
		}
		else if (ret > 0)
		{
 			lowerbound = key;			
			if (key == upperbound-1) key = upperbound;
			else key = ((upperbound - lowerbound) /2) + lowerbound;
		}
	}
	*found = 0;
	return key;
}

struct VoxelOptRenderAssembleData
{
	Vertex*			vtxBuffer;
	GLuint*			idxBuffer;
	int				vtxCount;
	int				idxCount;
	AABoundingVolume bounds;
	int				faceCenterCount;
	glm::vec3 *		faceCenterList;
	int				discardedCount;
	int				elemCount;
};

bool SvoViewer::VoxelOptRenderAssemblePerVoxel(OctreeElement* node, void* extraData)
{
	VoxelTreeElement* voxel = (VoxelTreeElement*)node;
    VoxelOptRenderAssembleData* args = (VoxelOptRenderAssembleData*)extraData;
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
		int foundBVS = 0;
		int idxIntoList = binVecSearch(hackCenterVal, args->faceCenterList, args->faceCenterCount, &foundBVS);
		if (foundBVS == 0) { assert(0); continue; } // What the? 
		//if (foundVal == NULL) { assert(0); continue; } // What the? 
		//unsigned int idxIntoList = (foundVal - args->faceCenterList) / sizeof(glm::vec3);
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
			args->bounds.AddToSet(args->vtxBuffer[args->vtxCount].position);			
			args->vtxCount++;
		}
	}

	// Assemble the index lists.
	for ( int face = 0; face < NUM_CUBE_FACES; face++)
	{
		if (doAddFace[face])
		{
			for (int i = 0; i < 6; i++)
			{
				// index is : current vtx + offset in map table
				args->idxBuffer[args->idxCount] = baseVtxCount + vtxToAddMap[ SvoViewerNames::cubeFaceIndices[face][i] ];
				args->idxCount++;
				args->elemCount += 2; // Add 2 elements per face
			}
		}
	}

	return true;
}

void SvoViewer::InitializeVoxelOptRenderSystem()
{
	if (_voxelOptRenderInitialized) return;
	_numSegments = 0;
	_totalPossibleElems = 0;
	memset(_numChildNodeLeaves, 0, sizeof(_numChildNodeLeaves));
	memset(_segmentNodeReferences, 0, sizeof(_segmentNodeReferences));
	memset(_segmentBoundingVolumes, 0, sizeof(_segmentBoundingVolumes));

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
		_readIndicesArray  = new GLuint[NUM_CUBE_FACES * 2 * 3 * _numChildNodeLeaves[i]];

		VoxelOptRenderAssembleData args;
		args.vtxBuffer = _readVertexStructs;
		args.idxBuffer = _readIndicesArray;
		args.vtxCount  = 0;
		args.idxCount  = 0;
		args.faceCenterCount = visFaceData.count;
		args.faceCenterList = visFaceData.ptList;
		args.discardedCount = 0;
		args.elemCount = 0;
		_systemTree.recurseNodeWithOperation(_segmentNodeReferences[i], &VoxelOptRenderAssemblePerVoxel, &args, 0);
		_segmentBoundingVolumes[i] = args.bounds;
		_segmentElemCount[i] = args.elemCount;

		SetupGlVBO(&_vboOVerticesIds[i], args.vtxCount * sizeof(Vertex), GL_ARRAY_BUFFER, GL_STATIC_DRAW, _readVertexStructs);				
		SetupGlVBO(&_vboOIndicesIds[i], args.idxCount * sizeof(GLuint), GL_ARRAY_BUFFER, GL_STATIC_DRAW, _readIndicesArray);

		qDebug("Partition %d, vertices %d, indices %d, discarded %d\n", i, args.vtxCount, args.idxCount, args.discardedCount);

		delete [] _readVertexStructs;
		delete [] _readIndicesArray;
		delete [] faceCenters;
	}

	_voxelOptRenderInitialized = true;
}

void SvoViewer::RenderTreeSystemAsOptVoxels()
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
			if (isVisibleBV(&_segmentBoundingVolumes[i], &_myCamera, &_viewFrustum)) // Add aggressive LOD check here.
			{
				glBindBuffer(GL_ARRAY_BUFFER, _vboOVerticesIds[i]);
                // NOTE: mac compiler doesn't support offsetof() for non-POD types, which apparently glm::vec3 is
				//glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,position));
				glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
				glEnableVertexAttribArray(ATTRIB_POSITION);

                // NOTE: mac compiler doesn't support offsetof() for non-POD types, which apparently glm::vec3 is
				//glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,color));
				glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*)sizeof(glm::vec3));
				glEnableVertexAttribArray(ATTRIB_COLOR);

				//glVertexPointer(3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex,position));
				glEnableClientState(GL_COLOR_ARRAY);

                // NOTE: mac compiler doesn't support offsetof() for non-POD types, which apparently glm::vec3 is
				//glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex,color));
				glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)sizeof(glm::vec3));

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboOIndicesIds[i]);

				glDrawElements(GL_TRIANGLES, _segmentElemCount[i], GL_UNSIGNED_INT, NULL);
				_numElemsDrawn += _segmentElemCount[i];

				/*glColor3f(.8f, 0.0f, 0.0f);
				glBegin(GL_POINTS);
				for (int j = 0; j < GLOBAL_NORMALS_VERTICES_PER_VOXEL; j++)
				{
					glm::vec3 pt = _segmentBoundingVolumes[i].getCorner((BoxVertex)j);
					glVertex3f(pt.x, pt.y, pt.z);
				}
				glEnd();*/
			}
		}
	}
	glDisableVertexAttribArray(ATTRIB_POSITION);
	glDisableVertexAttribArray(ATTRIB_COLOR);	
}


void SvoViewer::StopUsingVoxelOptRenderSystem()
{
	glDisableVertexAttribArray(ATTRIB_POSITION);
	glDisableVertexAttribArray(ATTRIB_COLOR);	
	glDisable(GL_LIGHTING);
}


