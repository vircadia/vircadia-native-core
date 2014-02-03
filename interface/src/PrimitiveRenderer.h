/** 
 *	@file PrimitiveRenderer.h
 *	A geometric primitive renderer.
 *
 *	@author: Norman Crafts
 *	@copyright 2014, High Fidelity, Inc. All rights reserved.
 */

#ifndef __interface__PrimitiveRenderer__
#define __interface__PrimitiveRenderer__

#include <vector>
#include <Queue.h>

/** Vertex element structure
 *	Using the array of structures approach to specifying
 *	vertex data to GL cuts down on the calls to glBufferSubData
 */
typedef
	struct __VertexElement 
	{
		struct __position {
			float x;
			float y;
			float z;
		} position;
		struct __normal {
			float x;
			float y;
			float z;
		} normal;
		struct __color {
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		} color;
	} VertexElement;

/** Triangle element index structure
 *	Specify the vertex indices of the triangle and its element index.
 */
typedef
struct __TriElement 
{
	int indices[3];
	int id;

} TriElement;

typedef std::vector<VertexElement *, std::allocator<VertexElement *> > VertexElementList;
typedef std::vector<int, std::allocator<int> > VertexElementIndexList;
typedef std::vector<TriElement *, std::allocator<TriElement *> > TriElementList;

/** 
 *	@class Primitive
 *	Primitive Interface class
 *	Abstract class for accessing vertex and tri elements of geometric primitives
 *
 */
class Primitive 
{
public:
	virtual ~Primitive();

	// API methods go here

	/** Vertex element accessor
	 *	@return A list of vertex elements of the primitive
	 */
	VertexElementList const & vertexElements() const;

	/** Vertex element index accessor
	 *	@return A list of vertex element indices of the primitive
	 */
	VertexElementIndexList & vertexElementIndices();

	/** Tri element accessor
	 *	@return A list of tri elements of the primitive
	 */
	TriElementList & triElements();

	/** Release vertex elements
	 */
	void releaseVertexElements();

protected:
	/** Default constructor prohibited to API user, restricted to service implementer
	 */
	Primitive();

private:
	/** Copy constructor prohibited
	 */
	Primitive(
		Primitive const & prim
		);

	// SPI methods are defined here

	/** Vertex element accessor
	 *	Service implementer to provide private override for this method
	 *	in derived class
	 */
	virtual VertexElementList const & vVertexElements() const = 0;

	/** Vertex element index accessor
	 *	Service implementer to provide private override for this method
	 *	in derived class
	 */
	virtual VertexElementIndexList & vVertexElementIndices() = 0;

	/** Tri element accessor
	 *	Service implementer to provide private override for this method
	 *	in derived class
	 */
	virtual TriElementList & vTriElements() = 0;

	/** Release vertex elements
	 */
	virtual void vReleaseVertexElements() = 0;

};


/** 
 *	@class Cube
 *	Class for accessing the vertex and tri elements of a cube
 */
class Cube:
	public Primitive
{
public:
	/** Configuration dependency injection constructor
	 */
	Cube(
		float x,
		float y,
		float z,
		float s,
		unsigned char r,
		unsigned char g,
		unsigned char b,
		unsigned char faces
		);

	~Cube();

private:
	/** Copy constructor prohibited
	 */
	Cube (
		Cube const & cube
		);

	void initialize(
		float x,
		float y,
		float z,
		float s,
		unsigned char r,
		unsigned char g,
		unsigned char b,
		unsigned char faceExclusions
		);

	void terminate();

	void initializeVertices(
		float x,
		float y,
		float z,
		float s,
		unsigned char r,
		unsigned char g,
		unsigned char b,
		unsigned char faceExclusions
		);

	void terminateVertices();

	void initializeTris(
		unsigned char faceExclusions
		);

	void terminateTris();

	// SPI virtual override methods go here

	VertexElementList const & vVertexElements() const;
	VertexElementIndexList & vVertexElementIndices();
	TriElementList & vTriElements();
	void vReleaseVertexElements();


private:
	VertexElementList _vertices;				///< Vertex element list
	VertexElementIndexList _vertexIndices;		///< Vertex element index list
	TriElementList _tris;						///< Tri element list

	static unsigned char s_faceIndexToHalfSpaceMask[6];
	static float s_vertexIndexToConstructionVector[24][3];
	static float s_vertexIndexToNormalVector[6][3];

};


/** 
 *	@class Renderer
 *	GL renderer interface class
 *	Abstract class for rendering geometric primitives in GL
 */
class Renderer 
{
public:
	virtual ~Renderer();

	// API methods go here

	/** Add primitive to renderer database
	 */
	int add(
		Primitive *primitive					///< Pointer to primitive
		);

	/** Remove primitive from renderer database
	 */
	void remove(
		int id									///< Primitive id
		);

	/** Render primitive database
	 *	The render method assumes appropriate GL context and state has
	 *	already been provided for
	 */
	void render();

protected:
	/** Default constructor prohibited to API user, restricted to service implementer
	 */
	Renderer();

private:
	/** Copy constructor prohibited
	 */
	Renderer(
		Renderer const & primitive
		);

	// SPI methods are defined here

	/** Add primitive to renderer database
	 *	Service implementer to provide private override for this method
	 *	in derived class
	 */
	virtual int vAdd(
		Primitive *primitive					///< Pointer to primitive
		) = 0;

	/** Remove primitive from renderer database
	 *	Service implementer to provide private override for this method
	 *	in derived class
	 */
	virtual void vRemove(
		int id									///< Primitive id
		) = 0;

	/** Render primitive database
	 *	Service implementer to provide private virtual override for this method
	 *	in derived class
	 */
	virtual void vRender() = 0;
};

/**
 *	@class PrimitiveRenderer
 *	Renderer implementation class for the rendering of geometric primitives
 *	using GL element array and GL array buffers
 */
class PrimitiveRenderer :
	public Renderer
{
public:
	/** Configuration dependency injection constructor
	 */
	PrimitiveRenderer(
		int maxCount
		);

	~PrimitiveRenderer();

private:
	/** Default constructor prohibited
	 */
	PrimitiveRenderer();

	/** Copy constructor prohibited
	 */
	PrimitiveRenderer(
		PrimitiveRenderer const & renderer
		);

	void initialize();
	void terminate();

	/** Allocate and initialize GL buffers
	 */
	void initializeGL();

	/** Terminate and deallocate GL buffers
	 */
	void terminateGL();

	void initializeBookkeeping();
	void terminateBookkeeping();

	/** Construct the elements of the faces of the primitive
	 */
	void constructElements(
		Primitive *primitive
		);

	/** Deconstruct the elements of the faces of the primitive
	 */
	void deconstructElements(
		Primitive *primitive
		);

	/** Deconstruct the triangle element from the GL buffer
	 */
	void deconstructTriElement(
		int idx
		);

	/** Transfer the vertex element to the GL buffer
	 */
	void transferVertexElement(
		int idx,
		VertexElement *vertex
		);

	/** Transfer the triangle element to the GL buffer
	 */
	void transferTriElement(
		int idx,
		int tri[3]
		);

	/** Get available primitive index
	 *	Get an available primitive index from either the recycling
	 *	queue or incrementing the counter
	 */
	int getAvailablePrimitiveIndex();

	/** Get available vertex element index
	 *	Get an available vertex element index from either the recycling
	 *	queue or incrementing the counter
	 */
	int getAvailableVertexElementIndex();

	/** Get available triangle element index
	 *	Get an available triangle element index from either the elements
	 *	scheduled for deconstruction queue, the recycling
	 *	queue or incrementing the counter
	 */
	int getAvailableTriElementIndex();

	// SPI virtual override methods go here

	/** Add primitive to renderer database
	 */
	int vAdd(
		Primitive *primitive
		);

	/** Remove primitive from renderer database
	 */
	void vRemove(
		int id
		);

	/** Render triangle database
	 */
	void vRender();

private:

	int _maxCount;

	// GL related parameters

	GLuint _triBufferId;						///< GL element array buffer id
	GLuint _vertexBufferId;						///< GL vertex array buffer id

	// Book keeping parameters

	int _vertexElementCount;					///< Count of vertices
	int _maxVertexElementCount;					///< Max count of vertices

	int _triElementCount;						///< Count of triangles
	int _maxTriElementCount;					///< Max count of triangles

	std::map<int, Primitive *> _indexToPrimitiveMap;	///< Associative map between index and primitive
	int _primitiveCount;						///< Count of primitives

	Queue<int> _availablePrimitiveIndex;		///< Queue of primitive indices available
	Queue<int> _availableVertexElementIndex;	///< Queue of vertex element indices available
	Queue<int> _availableTriElementIndex;		///< Queue of triangle element indices available
	Queue<int> _deconstructTriElementIndex;		///< Queue of triangle element indices requiring GL update

	// Statistics parameters, not necessary for proper operation

	int _gpuMemoryUsage;
	int _cpuMemoryUsage;

};


#endif
