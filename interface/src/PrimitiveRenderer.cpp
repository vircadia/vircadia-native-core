/** 
 *	@file PrimitiveRenderer.cpp
 *	A geometric primitive renderer.
 *
 *	@author: Norman Crafts
 *	@copyright 2014, High Fidelity, Inc. All rights reserved.
 */
#include "InterfaceConfig.h"
#include "OctreeElement.h"
#include "PrimitiveRenderer.h"

Primitive::Primitive()
{}

Primitive::~Primitive()
{}

// Simple dispatch between API and SPI

VertexElementList const & Primitive::vertexElements() const
{
	return vVertexElements();
}

VertexElementIndexList & Primitive::vertexElementIndices()
{
	return vVertexElementIndices();
}

TriElementList & Primitive::triElements()
{
	return vTriElements();
}

void Primitive::releaseVertexElements()
{
	vReleaseVertexElements();
}


Cube::Cube(
	float x,
	float y,
	float z,
	float s,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	unsigned char faceExclusions
	)
{
	initialize(x, y, z, s, r, g, b, faceExclusions);
}

Cube::~Cube()
{
	terminate();
}

void Cube::initialize(
	float x,
	float y,
	float z,
	float s,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	unsigned char faceExclusions
	)
{
	initializeVertices(x, y, z, s, r, g, b, faceExclusions);
	initializeTris(faceExclusions);
}

void Cube::terminate()
{
	terminateTris();
	terminateVertices();
}

void Cube::initializeVertices(
	float x,
	float y,
	float z,
	float s,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	unsigned char faceExclusions
	)
{
	for (int i = 0; i < 24; i++) 
	{
		// Check whether the vertex is necessary for the faces indicated by faceExclusions bit mask.
		if (~faceExclusions & s_faceIndexToHalfSpaceMask[i >> 2]) 
		//if (~0x00 & s_faceIndexToHalfSpaceMask[i >> 2]) 
		//if (faceExclusions & s_faceIndexToHalfSpaceMask[i >> 2]) 
		{
			VertexElement *v = new VertexElement();
			if (v)
			{
				// Construct vertex position
				v->position.x = x + s * s_vertexIndexToConstructionVector[i][0];
				v->position.y = y + s * s_vertexIndexToConstructionVector[i][1];
				v->position.z = z + s * s_vertexIndexToConstructionVector[i][2];

				// Construct vertex normal
				v->normal.x = s_vertexIndexToNormalVector[i >> 2][0];
				v->normal.y = s_vertexIndexToNormalVector[i >> 2][1];
				v->normal.z = s_vertexIndexToNormalVector[i >> 2][2];

				// Construct vertex color
//#define FALSE_COLOR
#ifndef FALSE_COLOR
				v->color.r = r;
				v->color.g = g;
				v->color.b = b;
				v->color.a = 255;
#else
				static unsigned char falseColor[6][3] = {
					192,   0,   0, // Bot
					  0, 192,   0, // Top
					  0,   0, 192, // Right
					192,   0, 192, // Left
					192, 192,   0, // Near
					192, 192, 192  // Far
				}; 
				v->color.r = falseColor[i >> 2][0];
				v->color.g = falseColor[i >> 2][1];
				v->color.b = falseColor[i >> 2][2];
				v->color.a = 255;
#endif

				// Add vertex element to list
				_vertices.push_back(v);
			}
		}
	}
}

void Cube::terminateVertices()
{
	VertexElementList::iterator it  = _vertices.begin();
	VertexElementList::iterator end = _vertices.end();

	for ( ; it != end; ++it)
	{
		delete *it;
	}
	_vertices.clear();
}

void Cube::initializeTris(
	unsigned char faceExclusions
	)
{
	int index = 0;
	for (int i = 0; i < 6; i++) 
	{
		// Check whether the vertex is necessary for the faces indicated by faceExclusions bit mask.
		// uncomment this line to exclude shared faces: 
		if (~faceExclusions & s_faceIndexToHalfSpaceMask[i]) 
		// uncomment this line to load all faces: if (~0x00 & s_faceIndexToHalfSpaceMask[i]) 
		// uncomment this line to include shared faces: if (faceExclusions & s_faceIndexToHalfSpaceMask[i]) 
		{
			int start = index;
			// Create the triangulated face, two tris, six indices referencing four vertices, both
			// with cw winding order, such that:

			//       A-B
			//       |\|
			//       D-C

			// Store triangle ABC

			TriElement *tri = new TriElement();
			if (tri)
			{
				tri->indices[0] = index++;
				tri->indices[1] = index++;
				tri->indices[2] = index;

				// Add tri element to list
				_tris.push_back(tri);
			}

			// Now store triangle ACD
			tri = new TriElement();
			if (tri)
			{
				tri->indices[0] = start;
				tri->indices[1] = index++;
				tri->indices[2] = index++;

				// Add tri element to list
				_tris.push_back(tri);
			}
		}
	}
}

void Cube::terminateTris()
{
	TriElementList::iterator it  = _tris.begin();
	TriElementList::iterator end = _tris.end();

	for ( ; it != end; ++it)
	{
		delete *it;
	}
	_tris.clear();
}

VertexElementList const & Cube::vVertexElements() const
{
	return _vertices;
}

VertexElementIndexList & Cube::vVertexElementIndices()
{
	return _vertexIndices;
}

TriElementList & Cube::vTriElements()
{
	return _tris;
}

void Cube::vReleaseVertexElements()
{
	terminateVertices();
}

unsigned char Cube::s_faceIndexToHalfSpaceMask[6] = {
	OctreeElement::HalfSpace::Bottom,
	OctreeElement::HalfSpace::Top,
	OctreeElement::HalfSpace::Right,
	OctreeElement::HalfSpace::Left,
	OctreeElement::HalfSpace::Near,
	OctreeElement::HalfSpace::Far,
};

#define CW_CONSTRUCTION
#ifdef CW_CONSTRUCTION
// Construction vectors ordered such that the vertices of each face are
// CW in a right-handed coordinate system with B-L-N at 0,0,0. 
float Cube::s_vertexIndexToConstructionVector[24][3] = {
	// Bottom
	{ 0,0,0 },
	{ 1,0,0 },
	{ 1,0,1 },
	{ 0,0,1 },
	// Top
	{ 0,1,0 },
	{ 0,1,1 },
	{ 1,1,1 },
	{ 1,1,0 },
	// Right
	{ 1,0,0 },
	{ 1,1,0 },
	{ 1,1,1 },
	{ 1,0,1 },
	// Left
	{ 0,0,0 },
	{ 0,0,1 },
	{ 0,1,1 },
	{ 0,1,0 },
	// Near
	{ 0,0,0 },
	{ 0,1,0 },
	{ 1,1,0 },
	{ 1,0,0 },
	// Far
	{ 0,0,1 },
	{ 1,0,1 },
	{ 1,1,1 },
	{ 0,1,1 },
};
#else // CW_CONSTRUCTION
// Construction vectors ordered such that the vertices of each face are
// CCW in a right-handed coordinate system with B-L-N at 0,0,0. 
float Cube::s_vertexIndexToConstructionVector[24][3] = {
	// Bottom
	{ 0,0,0 },
	{ 0,0,1 },
	{ 1,0,1 },
	{ 1,0,0 },
	// Top
	{ 0,1,0 },
	{ 1,1,0 },
	{ 1,1,1 },
	{ 0,1,1 },
	// Right
	{ 1,0,0 },
	{ 1,0,1 },
	{ 1,1,1 },
	{ 1,1,0 },
	// Left
	{ 0,0,0 },
	{ 0,1,0 },
	{ 0,1,1 },
	{ 0,0,1 },
	// Near
	{ 0,0,0 },
	{ 1,0,0 },
	{ 1,1,0 },
	{ 0,1,0 },
	// Far
	{ 0,0,1 },
	{ 0,1,1 },
	{ 1,1,1 },
	{ 1,0,1 },
};
#endif

// Normals for a right-handed coordinate system
float Cube::s_vertexIndexToNormalVector[6][3] = {
	{  0,-1, 0 }, // Bottom
	{  0, 1, 0 }, // Top
	{  1, 0, 0 }, // Right
	{ -1, 0, 0 }, // Left
	{  0, 0,-1 }, // Near
	{  0, 0, 1 }, // Far
};

Renderer::Renderer()
{}

Renderer::~Renderer()
{}

// Simple dispatch between API and SPI
int Renderer::add(
	Primitive *primitive
	)
{
	return vAdd(primitive);
}

void Renderer::remove(
	int id
	)
{
	vRemove(id);
}

void Renderer::render()
{
	vRender();
}

PrimitiveRenderer::PrimitiveRenderer(
	int maxCount
	) :
		_maxCount(maxCount),
		_vertexElementCount(0),
		_maxVertexElementCount(maxCount),
		_triElementCount(0),
		_maxTriElementCount(maxCount),
		_primitiveCount(0),

		_triBufferId(0),
		_vertexBufferId(0),

		_gpuMemoryUsage(0),
		_cpuMemoryUsage(0)

{
	initialize();
}

PrimitiveRenderer::~PrimitiveRenderer()
{
	terminate();
}

void PrimitiveRenderer::initialize()
{
	initializeGL();
	initializeBookkeeping();
}

void PrimitiveRenderer::initializeGL()
{
	glGenBuffers(1, &_triBufferId);
	glGenBuffers(1, &_vertexBufferId);

	// Set up the element array buffer containing the index ids
	int size = _maxCount * sizeof(GLint) * 3;
	_gpuMemoryUsage += size;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _triBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Set up the array buffer in the form of array of structures
	// I chose AOS because it maximizes the amount of data tranferred
	// by a single glBufferSubData call.
	size = _maxCount * sizeof(VertexElement);
	_gpuMemoryUsage += size;

	glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Initialize the first vertex element in the buffer to all zeros, the
	// degenerate case
	_vertexElementCount = 1;
	_triElementCount = 1;

	VertexElement v;
	memset(&v, 0, sizeof(v));

	transferVertexElement(0, &v);
	deconstructTriElement(0);

	GLint err = glGetError();
}

void PrimitiveRenderer::initializeBookkeeping()
{
	_primitiveCount = 1;
	_cpuMemoryUsage = sizeof(PrimitiveRenderer);
}

void PrimitiveRenderer::terminate()
{
	terminateBookkeeping();
	terminateGL();
}

void PrimitiveRenderer::terminateGL()
{
	glDeleteBuffers(1, &_vertexBufferId);
	glDeleteBuffers(1, &_triBufferId);
	GLint err = glGetError();
}

void PrimitiveRenderer::terminateBookkeeping()
{
	// Drain all of the queues, stop updating the counters
	while (_availableVertexElementIndex.remove());
	while (_availableTriElementIndex.remove());
	while (_deconstructTriElementIndex.remove());

	std::map<int, Primitive *>::iterator it  = _indexToPrimitiveMap.begin();
	std::map<int, Primitive *>::iterator end = _indexToPrimitiveMap.end();

	for ( ; it != end; ++it)
	{
		Primitive *primitive = it->second;
		delete primitive;
	}
}

int PrimitiveRenderer::vAdd(
	Primitive *primitive
	)
{
	int index = getAvailablePrimitiveIndex();
	if (index != 0)
	{
		try
		{
			// Take ownership of primitive, including responsibility
			// for destruction
			_indexToPrimitiveMap[index] = primitive;
			constructElements(primitive);

			// No need to keep an extra copy of the vertices
			primitive->releaseVertexElements();
		}
		catch(...)
		{
			// STL failed, recycle the index
			_availablePrimitiveIndex.add(index);
			index = 0;
		}
	}
	return index;
}

void PrimitiveRenderer::vRemove(
	int index
	)
{
	try
	{
		// Locate the primitive by id in the associative map
		std::map<int, Primitive *>::iterator it = _indexToPrimitiveMap.find(index);
		if (it != _indexToPrimitiveMap.end())
		{
			Primitive *primitive = it->second;
			if (primitive)
			{
				_indexToPrimitiveMap[index] = 0;
				deconstructElements(primitive);
				_availablePrimitiveIndex.add(index);
			}
			// Not necessary to remove the item from the associative map, because 
			// the index is going to be re-used, but if you want to... uncomment the following:
			//_indexToPrimitiveMap.erase(it);
		}
	}
	catch(...)
	{
		// STL failed
	}
}

void PrimitiveRenderer::constructElements(
	Primitive *primitive
	)
{
	// Load vertex elements
	VertexElementIndexList & vertexElementIndexList = primitive->vertexElementIndices();
	{
		VertexElementList const & vertices = primitive->vertexElements();
		VertexElementList::const_iterator it  = vertices.begin();
		VertexElementList::const_iterator end = vertices.end();

		for ( ; it != end; ++it ) 
		{
			int index = getAvailableVertexElementIndex();
			if (index != 0)
			{
				vertexElementIndexList.push_back(index);
				VertexElement *vertex = *it;
				transferVertexElement(index, vertex);
			}
		}
	}

	// Load tri elements
	{
		TriElementList & tris = primitive->triElements();
		TriElementList::iterator it  = tris.begin();
		TriElementList::iterator end = tris.end();

		for ( ; it != end; ++it)
		{
			TriElement *tri = *it;
			int index = getAvailableTriElementIndex();
			if (index != 0)
			{
				int k;
				k = tri->indices[0];
				tri->indices[0] = vertexElementIndexList[k];

				k = tri->indices[1];
				tri->indices[1] = vertexElementIndexList[k];

				k = tri->indices[2];
				tri->indices[2] = vertexElementIndexList[k];

				tri->id = index;
				transferTriElement(index, tri->indices);
			}
		}
	}
}

void PrimitiveRenderer::deconstructElements(
	Primitive *primitive
	)
{
	// Schedule the tri elements of the face for deconstruction
	{
		TriElementList & tris = primitive->triElements();
		TriElementList::iterator it  = tris.begin();
		TriElementList::iterator end = tris.end();

		for ( ; it != end; ++it)
		{
			TriElement *tri = *it;

			// Put the tri element index into decon queue
			_deconstructTriElementIndex.add(tri->id);
		}
	}
	// Return the vertex element index to the available queue, it is not necessary
	// to zero the data
	{
		VertexElementIndexList & vertexIndexList = primitive->vertexElementIndices();
		VertexElementIndexList::iterator it  = vertexIndexList.begin();
		VertexElementIndexList::iterator end = vertexIndexList.end();

		for ( ; it != end; ++it)
		{
			int index = *it;

			// Put the vertex element index into the available queue
			_availableVertexElementIndex.add(index);
		}
	}

	// destroy primitive
	delete primitive;
}

int PrimitiveRenderer::getAvailablePrimitiveIndex()
{
	// Check the available primitive index queue first for an available index.
	int index = _availablePrimitiveIndex.remove();
	// Remember that the primitive index 0 is used not used.
	if (index == 0) 
	{
		// There are no primitive indices available from the queue, 
		// make one up
		index = _primitiveCount++;
	}
	return index;
}

int PrimitiveRenderer::getAvailableVertexElementIndex()
{
	// Check the available vertex element queue first for an available index.
	int index = _availableVertexElementIndex.remove();
	// Remember that the vertex element 0 is used for degenerate triangles.
	if (index == 0) 
	{
		// There are no vertex elements available from the queue, 
		// grab one from the end of the list
		if (_vertexElementCount < _maxVertexElementCount)
			index = _vertexElementCount++;
	}
	return index;
}

int PrimitiveRenderer::getAvailableTriElementIndex()
{
	// Check the deconstruct tri element queue first for an available index.
	int index = _deconstructTriElementIndex.remove();
	// Remember that the tri element 0 is used for degenerate triangles.
	if (index == 0) 
	{
		// There are no tri elements in the deconstruct tri element queue that are reusable.
		// Check the available tri element queue.
		index = _availableTriElementIndex.remove();
		if (index == 0)
		{
			// There are no reusable tri elements available from any queue, 
			// grab one from the end of the list
			if (_triElementCount < _maxTriElementCount)
				index = _triElementCount++;
		}
	}
	return index;
}

void PrimitiveRenderer::deconstructTriElement(
	int idx
	)
{
	// Set the element to the degenerate case.
	int degenerate[3] = { 0, 0, 0 };
	transferTriElement(idx, degenerate);

}

void PrimitiveRenderer::transferVertexElement(
	int idx,
	VertexElement *vertex
	)
{
	glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferId);
	glBufferSubData(GL_ARRAY_BUFFER, idx * sizeof(VertexElement), sizeof(VertexElement), vertex);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PrimitiveRenderer::transferTriElement(
	int idx,
	int tri[3]
)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _triBufferId);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, idx * sizeof(GLint) * 3, sizeof(GLint) * 3, tri);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void PrimitiveRenderer::vRender()
{
	// Now would be an appropriate time to set the element array buffer ids
	// scheduled for deconstruction to the degenerate case.
	int id;
	while ((id = _deconstructTriElementIndex.remove())) 
	{
		deconstructTriElement(id);
		_availableTriElementIndex.add(id);
	}

	// The application uses clockwise winding for the definition of front face, but I
	// arbitrarily chose counter-clockwise (that is the gl default) to construct the triangulation
	// so...
	//glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferId);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(VertexElement), 0);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(VertexElement), (GLvoid const *)12);

	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexElement), (GLvoid const *)24);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	GLint err = glGetError();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _triBufferId);
	glDrawElements(GL_TRIANGLES, 3 * _triElementCount, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisable(GL_CULL_FACE);

	// TODO: does the interface ever change the winding order?
	//glFrontFace(GL_CW);
	err = glGetError();
}

