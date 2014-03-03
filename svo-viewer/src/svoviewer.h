//
//  svoviewer.h
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//



#ifndef SVOVIEWER_H
#define SVOVIEWER_H

#pragma once

#include "svo-viewer-config.h"

#include <QApplication>
#include <QGLWidget>

#include "globals.h"
#include "AABoundingVolume.h"



enum SVOViewerShaderModel
{
	RENDER_NONE,
	RENDER_POINTS,
	RENDER_CLASSIC_POLYS,
	RENDER_OPT_POLYS,
	RENDER_OPT_CULLED_POLYS,
};

enum CubeOrdering
{
	CUBE_TOP,
	CUBE_BOTTOM,
	CUBE_LEFT,
	CUBE_RIGHT,
	CUBE_FRONT,
	CUBE_BACK,
	NUM_CUBE_FACES
};

struct ViewFrustumOffset {
    float yaw;
    float pitch;
    float roll;
    float distance;
    float up;
};

struct RenderFlags
{
	bool ptRenderDirty : 1;
	bool voxelRenderDirty : 1;
	bool voxelOptRenderDirty : 1;
	bool usePtShader : 1;
	bool useVoxelShader : 1;
	bool useVoxelOptShader : 1;
	bool useShadows : 1;
};

struct Vertex
{
	glm::vec3		position;
	unsigned char	color[4];
};

enum VtxAttributes
{
	ATTRIB_POSITION,
	ATTRIB_COLOR,
	ATTRIB_TEXTURE
};

struct VoxelDimIdxSet
{
	GLuint	idxIds[NUM_CUBE_FACES];		// Id for each voxel face
	GLuint  idxCount[NUM_CUBE_FACES];	// count for each voxel face.
	GLuint* idxBuff[NUM_CUBE_FACES];	// actual index buffers for each voxel face
	GLuint  elemCount[NUM_CUBE_FACES];
	AABoundingVolume bounds[NUM_CUBE_FACES];	// Super efficient bounding set here.
	bool	visibleFace[NUM_CUBE_FACES];
};

struct VisibleFacesData
{
	int count;
	glm::vec3 * ptList;
};

struct FindNumLeavesData
{
	int numLeaves;
};

//#define MAX_VOXELS 4000000 
#define MAX_VOXELS 8000000
#define REASONABLY_LARGE_BUFFER 10000

#define MAX_NUM_OCTREE_PARTITIONS 20
#define MAX_NUM_VBO_ALLOWED MAX_NUM_OCTREE_PARTITIONS * NUM_CUBE_FACES
#define NO_PARTITION -1

class VoxelOptRenderer : public OctreeRenderer {
};

class SvoViewer : public QApplication
{
	Q_OBJECT

public:
	static SvoViewer* getInstance() { return static_cast<SvoViewer*>(QCoreApplication::instance()); }

	SvoViewer(int& argc, char** argv, QWidget *parent = 0);
	~SvoViewer();

	
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
	void init();
	void update(float deltaTime);
	void updateMouseRay();
	void updateProjectionMatrix(Camera& camera, bool updateViewFrustum);
	void updateCamera(float deltaTime);
	void loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum);
	ViewFrustum* getViewFrustum() { return &_viewFrustum; }
	void computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal, float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const ;
	void setupWorldLight();
	glm::vec2 getViewportDimensions() const{ return glm::vec2(_width,_height); }

	// User Tweakable LOD Items
    void autoAdjustLOD(float currentFPS);
    void setVoxelSizeScale(float sizeScale);
    float getVoxelSizeScale() const { return _voxelSizeScale; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }

	bool getIsPointShader(){ return _renderFlags.usePtShader; }
	void setIsPointShader(const bool b){ _renderFlags.usePtShader = b; }
	bool getIsVoxelShader(){ return _renderFlags.useVoxelShader; }
	void setIsVoxelShader(const bool b){ _renderFlags.useVoxelShader = b; }
	bool getIsVoxelOptShader(){ return _renderFlags.useVoxelOptShader; }
	void setIsVoxelOptShader(const bool b){ _renderFlags.useVoxelOptShader = b; }
	void setUseVoxelTextures(const bool b){_useVoxelTextures = b; }
	bool getUseVoxelTextures(){ return _useVoxelTextures; }
	void setUseShadows(const bool b){ _renderFlags.useShadows = b; }
	bool getUseShadows(){ return _renderFlags.useShadows; }

	//VoxelShader* getVoxelShader(){ return &_voxelShader; }
	//PointShader* getPointShader(){ return &_pointShader; }

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

	void PrintToScreen(const int width, const int height, const char* szFormat, ...);
	static GLubyte PrintGLErrorCode();

	// Some helper functions.
	GLubyte SetupGlVBO(GLuint * id, int sizeInBytes, GLenum target, GLenum usage, void * dataUp );
	static glm::vec3 computeQuickAndDirtyQuadCenter(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
        glm::vec3 avg = p0 + p1 + p2 + p3;
        avg /= 4.0f;
        return avg;
    }

	bool isVisibleBV(AABoundingVolume * volume, Camera * camera, ViewFrustum * frustum);
	float visibleAngleSubtended(AABoundingVolume * volume, Camera * camera, ViewFrustum * frustum);
	static int ptCompFunc(const void * a, const void * b);
	static int ptCloseEnough(const void * a, const void * b);
	static int binVecSearch(glm::vec3 searchVal, glm::vec3* list, int count, int * found);
	bool parameterizedRayPlaneIntersection(const glm::vec3 origin, const glm::vec3 direction, const glm::vec3 planePt, const glm::vec3 planeNormal, float *t);

protected:
	void InitializePointRenderSystem();
	void setupFaceIndices(GLuint& faceVBOID, GLubyte faceIdentityIndices[]);
	void InitializeVoxelRenderSystem();
	void InitializeVoxelOptRenderSystem();
	void InitializeVoxelOpt2RenderSystem();
	void StopUsingPointRenderSystem();
	void StopUsingVoxelRenderSystem();
	void StopUsingVoxelOptRenderSystem();
	void StopUsingVoxelOpt2RenderSystem();
	void UpdateOpt2BVFaceVisibility();


	void RenderTreeSystemAsPoints();
	void RenderTreeSystemAsVoxels();
	void RenderTreeSystemAsOptVoxels();
	void RenderTreeSystemAsOpt2Voxels();

	// Tree traversal functions.
	static bool PointRenderAssemblePerVoxel(OctreeElement* node, void* extraData);
	static bool VoxelRenderAssemblePerVoxel(OctreeElement* node, void* extraData);	
	static bool VoxelOptRenderAssemblePerVoxel(OctreeElement* node, void* extraData);		
	static bool VoxelOpt2RenderAssemblePerVoxel(OctreeElement* node, void* extraData);	
	static bool FindNumLeaves(OctreeElement* node, void* extraData);
	static bool TrackVisibleFaces(OctreeElement* node, void* extraData);


private slots:
	void idle();

private:
	//Ui::SvoViewerClass ui;
	
	QMainWindow*	_window;
	int				_width;
	int				_height;
	int				_pixelCount;
    QGLWidget*		_glWidget;

	//VoxelSystem		_voxels;
	VoxelTree		_systemTree;
	unsigned long   _nodeCount;
	unsigned int	_leafCount;

	ViewFrustum		_viewFrustum;
	Camera			_myCamera;                  // My view onto the world
	float			_pitch, _yaw, _roll;

	int				_mouseX;
    int				_mouseY;
    int				_mouseDragStartedX;
    int				_mouseDragStartedY;
	bool			_mousePressed;
    quint64			_lastMouseMove;
    bool			_mouseHidden;
    bool			_seenMouseMove;

    glm::vec3		_mouseRayOrigin;
    glm::vec3		_mouseRayDirection;

	glm::mat4		_untranslatedViewMatrix;
    glm::vec3		_viewMatrixTranslation;
	GLdouble		_projectionMatrix[16];
	GLdouble		_modelviewMatrix[16];
	GLint			_viewport[4];

	SVOViewerShaderModel	_currentShaderModel;
	//VoxelShader				_voxelShader;
    //PointShader				_pointShader;

	// Display options 
	int				_displayOnlyPartition;

	// Frame Rate Measurement
    int				_frameCount;
	int				_lastTrackedFrameCount;
    float			_fps;
    timeval			_applicationStartupTime;
	quint64			_appStartTickCount;
    quint64			_lastTimeFpsUpdated;
	quint64			_lastTimeFrameUpdated;

	// Render variables.
	bool			_ptRenderInitialized;
	bool			_voxelRenderInitialized;
	bool			_voxelOptRenderInitialized;
	bool			_voxelOpt2RenderInitialized;

	GLuint			_vertexShader;
	GLuint			_pixelShader;
	GLuint			_geometryShader;
	GLuint			_linkProgram;

	// Vars for RENDER_POINTS
	GLuint			_pointVtxBuffer;
	GLuint			_pointColorBuffer;
	glm::vec3*		_pointVertices;
	unsigned char*	_pointColors;
	int				_pointVerticesCount;

	// Vars for RENDER_CLASSIC_POLYS
	GLuint			_vboVerticesID;
    GLuint			_vboColorsID;
	GLuint			_vboIndicesIds[NUM_CUBE_FACES];
	GLuint*			_vboIndices[NUM_CUBE_FACES];
	//VoxelShaderVBOData* _vboShaderData; // may not need.
	glm::vec3*		_readVerticesArray;
    unsigned char*	_readColorsArray;
	GLuint*			_readIndicesArray;


	// Vars for RENDER_OPT_POLYS
	// Allow for primitive first level sorting at the moment.
	GLuint			_vboOVerticesIds[MAX_NUM_VBO_ALLOWED];
	glm::vec3*		_vboOVertices[MAX_NUM_VBO_ALLOWED];
	GLuint			_vboOIndicesIds[MAX_NUM_VBO_ALLOWED];
	GLuint*			_vboOIndices[MAX_NUM_VBO_ALLOWED];
	GLuint			_numChildNodeLeaves[MAX_NUM_VBO_ALLOWED];
	OctreeElement * _segmentNodeReferences[MAX_NUM_OCTREE_PARTITIONS];
	AABoundingVolume	_segmentBoundingVolumes[MAX_NUM_OCTREE_PARTITIONS];
	int				_segmentElemCount[MAX_NUM_OCTREE_PARTITIONS];
	unsigned int	_numSegments;
	Vertex *		_readVertexStructs;

	// Vars for RENDER_OPT_CULLED_POLYS
	// Use vtx vars from opt_polys version.
	VoxelDimIdxSet	_segmentIdxBuffers[MAX_NUM_OCTREE_PARTITIONS];
	bool			_useBoundingVolumes;


	int				_numElemsDrawn;
	int				_totalPossibleElems;

	RenderFlags		_renderFlags;

	ViewFrustumOffset _viewFrustumOffset;
	int				_maxVoxels;
    float			_voxelSizeScale;
    int				_boundaryLevelAdjust;
	float			_fieldOfView; /// in Degrees
	bool			_useVoxelTextures;

};

//Extern hack since this wasn't built with global linking to old project in mind.
extern SvoViewer * _globalSvoViewerObj;


#endif // SVOVIEWER_H
