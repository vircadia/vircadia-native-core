//
//  svoviewer.cpp
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//


#include "svoviewer.h"
#include "GLCanvas.h"
#include "TextRenderer.h"

#include <cstdio>
#include <QDesktopWidget>
#include <QOpenGLFramebufferObject>
#include <qtimer.h>
#include <QKeyEvent>

#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>


const int IDLE_SIMULATE_MSECS = 1;              // How often should call simulate and other stuff
                                                // in the idle loop?  (60 FPS is default)
												// Empirically, I found 5 to be about 60fps	

static QTimer* idleTimer = NULL;
const ViewFrustumOffset DEFAULT_FRUSTUM_OFFSET = {-135.0f, 0.0f, 0.0f, 25.0f, 0.0f};
SvoViewer * _globalSvoViewerObj; // Hack :: var to store global pointer since this wasn't designed to work with the interface nodelist.

SvoViewer::SvoViewer(int& argc, char** argv, QWidget *parent)
	: QApplication(argc, argv),
	  _window(new QMainWindow(desktop())),
	  _width(1280),
	  _height(720),
	  _pixelCount(1280*720),
      _glWidget(new GLCanvas()),
      _nodeCount(0),
	  _leafCount(0),
      _pitch(0),
      _yaw(0),
      _roll(0),
      _displayOnlyPartition(NO_PARTITION),
      _frameCount(0),
	  _fps(0.0),
	  _lastTimeFpsUpdated(0),
	  _lastTimeFrameUpdated(0),
	  _ptRenderInitialized(false),
	  _voxelRenderInitialized(false),
	  _voxelOptRenderInitialized(false),
	  _voxelOpt2RenderInitialized(false),
	  _vertexShader(0),
	  _pixelShader(0),
	  _geometryShader(0),
	  _pointVertices(NULL),
	  _pointVerticesCount(0),
      _numSegments(0),
      _useBoundingVolumes(true),
      _numElemsDrawn(0),
      _totalPossibleElems(0),
      _viewFrustumOffset(DEFAULT_FRUSTUM_OFFSET),
      _maxVoxels(DEFAULT_MAX_VOXELS_PER_SYSTEM),
      _voxelSizeScale(DEFAULT_OCTREE_SIZE_SCALE),
      _boundaryLevelAdjust(0),
	  //_vboShaderData(NULL),
    _fieldOfView(DEFAULT_FIELD_OF_VIEW_DEGREES)
{
    gettimeofday(&_applicationStartupTime, NULL);
	_appStartTickCount = usecTimestampNow();

	_globalSvoViewerObj = this;
    _mousePressed = false;
    _useVoxelTextures = false;

	//ui.setupUi(this);
	_window->setWindowTitle("SvoViewer");
	_window->resize(1280,720);

	_window->setCentralWidget(_glWidget);

	qDebug("Window initialized\n");

	_window->setVisible(true);
	_glWidget->setFocusPolicy(Qt::StrongFocus);
	_glWidget->setFocus();
	_glWidget->setMouseTracking(true);
	
	QString svoFileToRead;
	QString shaderMode;

    QStringList argumentList = arguments();
    
    // check if this domain server should use no authentication or a custom hostname for authentication
    const QString FILE_NAME = "--file";
    const QString SHADER_MODE = "--mode";
    if (argumentList.indexOf(FILE_NAME) != -1) {
        svoFileToRead = argumentList.value(argumentList.indexOf(FILE_NAME) + 1);
        qDebug() << "file:" << svoFileToRead;
    }
    if (argumentList.indexOf(SHADER_MODE) != -1) {
        shaderMode = argumentList.value(argumentList.indexOf(SHADER_MODE) + 1);
        qDebug() << "shaderMode:" << shaderMode;
    }

    if (shaderMode == "RENDER_OPT_CULLED_POLYS") {
        _currentShaderModel = RENDER_OPT_CULLED_POLYS;
    } else if (shaderMode == "RENDER_OPT_POLYS") {
        _currentShaderModel = RENDER_OPT_POLYS;
    } else if (shaderMode == "RENDER_CLASSIC_POLYS") {
        _currentShaderModel = RENDER_CLASSIC_POLYS;
    } else if (shaderMode == "RENDER_POINTS") {
        _currentShaderModel = RENDER_POINTS;
    } else {
    	_currentShaderModel = RENDER_OPT_CULLED_POLYS;
    }
	memset(&_renderFlags, 0, sizeof(_renderFlags));
	_renderFlags.useShadows = false;

	// Load the scene.

	// We want our corner voxels to be about 1/2 meter high, and our TREE_SCALE is in meters, so...
    float voxelSize = 0.5f / TREE_SCALE;

	qDebug("Reading SVO file\n");

	//H:\highfidelity\hifi-19509\build\interface\resources\voxels1A.svo
	/**
    const int MAX_PATH = 1024;
	char svoFileToRead[MAX_PATH] = "./voxels10.svo"; //"H:\\highfidelity\\hifi-19509\\build\\interface\\resources\\voxels10.svo"
	if (argc > 1) {
	    strcpy(svoFileToRead, argv[1]); // Command line is arg 0 by default.
	    qDebug() << svoFileToRead;
	}
	**/

	//qDebug("Sizeof Octree element is %d\n", sizeof(OctreeElement));

	quint64 readStart = usecTimestampNow();
	bool readSucceeded = _systemTree.readFromSVOFile(qPrintable(svoFileToRead));
	qDebug("Done reading SVO file : %f seconds :  ", (float)(usecTimestampNow() - readStart) / 1000.0f);
	readSucceeded ? qDebug("Succeeded\n") : qDebug("Failed\n");

	// this should exist... we just loaded it...
    if (_systemTree.getVoxelAt(voxelSize, 0, voxelSize, voxelSize)) {
        qDebug("corner point voxelSize, 0, voxelSize exists...\n");
    } else {
        qDebug("corner point voxelSize, 0, voxelSize does not exists...\n");
    }

    _nodeCount = _systemTree.getOctreeElementsCount();
    qDebug("Nodes after loading file: %ld nodes\n", _nodeCount);

		// Initialize the display model we're using.
	switch(_currentShaderModel)
	{
	case RENDER_NONE:
		break;
	case RENDER_POINTS:
		_renderFlags.useVoxelShader = false;
		_renderFlags.usePtShader = true;
		if (!_ptRenderInitialized) InitializePointRenderSystem();
		break;
	case RENDER_CLASSIC_POLYS:
		_renderFlags.useVoxelShader = true;
		_renderFlags.usePtShader = false;
		if (!_voxelRenderInitialized) InitializeVoxelRenderSystem();
		break;
	case RENDER_OPT_POLYS:		
		_renderFlags.useVoxelShader = true;
		_renderFlags.usePtShader = false;
		if (!_voxelOptRenderInitialized) InitializeVoxelOptRenderSystem();
		break;
	case RENDER_OPT_CULLED_POLYS:				
		_renderFlags.useVoxelShader = true;
		_renderFlags.usePtShader = false;
		if (!_voxelOpt2RenderInitialized) InitializeVoxelOpt2RenderSystem();
		break;
	}
}

SvoViewer::~SvoViewer()
{
	//glDeleteBuffers(_pointVtxBuffer); // TODO :: add comprehensive cleanup!!
	//glDeleteBuffers(1, _pointVtxBuffer);
	//glDeleteBuffers(1, _pointColorBuffer);

	if (_pointVertices) delete [] _pointVertices;
	if (_pointColors) delete [] _pointColors;
	//if (_vboShaderData) delete [] _vboShaderData;

	StopUsingVoxelRenderSystem();
}

void SvoViewer::init() {
    //_voxelShader.init();
    //_pointShader.init();
    _mouseX = _glWidget->width() / 2;
    _mouseY = _glWidget->height() / 2;
    QCursor::setPos(_mouseX, _mouseY);
    _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
    _myCamera.setPosition(glm::vec3(0.0f, 0.0f, -5.0f));
    _myCamera.setNearClip(0.01f);
    //_myCamera.setFarClip(500.0f * TREE_SCALE);
    _myCamera.setFarClip(TREE_SCALE);
    _myCamera.setFieldOfView(_fieldOfView);
    _myCamera.lookAt(glm::vec3(0.0f,0.0f,0.0f));
    _myCamera.setAspectRatio((float)_width / (float) _height);
    loadViewFrustum(_myCamera, _viewFrustum);
}

void SvoViewer::initializeGL() 
{
    #ifdef WIN32
    int argc = 0;
    glutInit(&argc, 0);
    #endif
    init();
    #ifdef WIN32
        GLenum err = glewInit();
        if (GLEW_OK != err) {
            /* Problem: glewInit failed, something is seriously wrong. */
            qDebug("Error: %s\n", glewGetErrorString(err));
        }
        qDebug("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif
    glViewport(0, 0, _width, _height);
    glGetIntegerv(GL_VIEWPORT, _viewport);
    //glEnable(GL_BLEND);
    //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);
    //glEnable(GL_LIGHTING);
    //glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    //// call our idle function whenever we can
    idleTimer = new QTimer(this);
    connect(idleTimer, SIGNAL(timeout()), SLOT(idle()));
    idleTimer->start(0);
    //_idleLoopStdev.reset();
    _lastTimeFpsUpdated = _lastTimeFpsUpdated = usecTimestampNow();
    float startupTime = (float)(_lastTimeFpsUpdated - _appStartTickCount) / 1000.0;
    qDebug("Startup time: %4.2f seconds.", startupTime);
    //// update before the first render
    updateProjectionMatrix(_myCamera, true);
    update(0.0f);

    UpdateOpt2BVFaceVisibility();
}

void SvoViewer::idle() {
    quint64 tc = usecTimestampNow();
    //  Only run simulation code if more than IDLE_SIMULATE_MSECS have passed since last time we ran
    quint64 elapsed = tc - _lastTimeFrameUpdated;
    //if (elapsed >= IDLE_SIMULATE_MSECS) 
    {
        const float BIGGEST_DELTA_TIME_SECS = 0.25f;
        update(glm::clamp((float)elapsed / 1000.f, 0.f, BIGGEST_DELTA_TIME_SECS));
        _glWidget->updateGL();
        // After finishing all of the above work, restart the idle timer, allowing 2ms to process events.
        idleTimer->start(2);
        _lastTimeFrameUpdated = tc;
    }
}


void SvoViewer::paintGL() 
{
	glClearColor( 0.f, 0.f, .3f, 0.f );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_LINE_SMOOTH);

	glViewport(0, 0, _width, _height);
	glMatrixMode( GL_PROJECTION ); 
	glLoadIdentity();
	glm::vec3 pos = _myCamera.getPosition();
	glm::vec3 target = _myCamera.getTargetPosition();
	updateProjectionMatrix(_myCamera, true);

	// save matrix
    glGetDoublev(GL_PROJECTION_MATRIX, _projectionMatrix);

	glMatrixMode( GL_MODELVIEW ); 
	glLoadIdentity();
	gluLookAt(pos.x, pos.y, pos.z,  target.x, target.y, target.z, 0, 1, 0);

	glGetDoublev(GL_MODELVIEW_MATRIX, _modelviewMatrix);

	//glFrontFace(GL_CW);

	glPointSize(5.0);
	
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_POINTS);
		glVertex3f(0,0,0);
		glVertex3f(1,0,0);
		glVertex3f(-1,0,0);
		glVertex3f(0,1,0);
		glVertex3f(0,-1,0);
		glVertex3f(0,0,1);
		glVertex3f(0,0,-1);
	glEnd();

	switch(_currentShaderModel)
	{
	case RENDER_NONE:
		// Nothing to do!
		break;
	case RENDER_POINTS:
		RenderTreeSystemAsPoints();
		break;
	case RENDER_CLASSIC_POLYS:		
		RenderTreeSystemAsVoxels();
		break;
	case RENDER_OPT_POLYS:		
		RenderTreeSystemAsOptVoxels();
		break;		
	case RENDER_OPT_CULLED_POLYS:		
		RenderTreeSystemAsOpt2Voxels();
		break;		
	}

	_frameCount++;

	// Print out some statistics.

	// Update every x seconds for more stability
	quint64 tc = usecTimestampNow();
	quint64 interval = tc - _lastTimeFpsUpdated;
	const quint64 USECS_PER_SECOND = 1000 * 1000;
    const int FPS_UPDATE_TIME_INTERVAL = 2;
	if (interval > (USECS_PER_SECOND * FPS_UPDATE_TIME_INTERVAL)) {
		int numFrames = _frameCount - _lastTrackedFrameCount;
		float intervalSeconds = (float)((float)interval/(float)USECS_PER_SECOND);
		_fps = (float)numFrames / intervalSeconds;
		_lastTrackedFrameCount = _frameCount;
		_lastTimeFpsUpdated = tc;
	}
	PrintToScreen((_width / 3) * 2, 10, "FPS is : %f", _fps );
	PrintToScreen(10, 10, "Camera Pos : %f %f %f", pos.x, pos.y, pos.z);
	PrintToScreen(10, 30, "Drawing %d of %d (%% %f) total elements", _numElemsDrawn, _totalPossibleElems, ((float)_numElemsDrawn / (float)_totalPossibleElems) * 100.0);
}

void drawtext(int x, int y, float scale, float rotate, float thick, int mono,
              char const* string, float r, float g, float b) {
    //
    //  Draws text on screen as stroked so it can be resized
    //
    glPushMatrix();
    glTranslatef(static_cast<float>(x), static_cast<float>(y), 0.0f);
    glColor3f(r,g,b);
    glRotated(rotate,0,0,1);
    // glLineWidth(thick);
    glScalef(scale / 0.10, scale / 0.10, 1.0);

    TextRenderer textRenderer(SANS_FONT_FAMILY, 11, 50);
    textRenderer.draw(0, 0, string);

    //textRenderer(mono)->draw(0, 0, string);

    glPopMatrix();
}


#define TEMP_STRING_BUFFER_MAX 1024
#define SHADOW_OFFSET 2
void SvoViewer::PrintToScreen(const int width, const int height, const char* szFormat, ...)
{
	char szBuff[TEMP_STRING_BUFFER_MAX]; 
	assert(strlen(szFormat) < TEMP_STRING_BUFFER_MAX); // > max_path. Use this only for small messages.
    va_list arg;
    va_start(arg, szFormat);
    vsnprintf(szBuff, sizeof(szBuff), szFormat, arg);
    va_end(arg);
	// Passed in via char for convenience - convert to unsigned for glutBitmapString
	unsigned char szUBuff[TEMP_STRING_BUFFER_MAX];
	memset(szUBuff, 0, sizeof(szUBuff));
	int len = strlen(szBuff); 
	for (int i = 0; i < len; i++) szUBuff[i] = (unsigned char)szBuff[i];
    qDebug() << szBuff;


	glEnable(GL_DEPTH_TEST);

	glMatrixMode( GL_PROJECTION ); 
	glPushMatrix();
	glLoadIdentity();
    gluOrtho2D(0, _width, _height, 0);

	glDisable(GL_LIGHTING);
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

    drawtext(width, height, 0.10f, 0, 1, 2, szBuff, 1,1,1);

	glPopMatrix();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );

	glEnable(GL_DEPTH_TEST);
}

void SvoViewer::update(float deltaTime){
}

void SvoViewer::resizeGL(int width, int height) 
{

    glViewport(0, 0, width, height); // shouldn't this account for the menu???
	glGetIntegerv(GL_VIEWPORT, _viewport);
	_pixelCount = width * height;
	_width = width; 
	_height = height;

    updateProjectionMatrix(_myCamera, true);
    glLoadIdentity();
}

void SvoViewer::updateProjectionMatrix(Camera& camera, bool updateViewFrustum) 
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;

    // Tell our viewFrustum about this change, using the application camera
    if (updateViewFrustum) {
        loadViewFrustum(camera, _viewFrustum);
        computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    } else {
        ViewFrustum tempViewFrustum;
        loadViewFrustum(camera, tempViewFrustum);
        tempViewFrustum.computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    }
    glFrustum(left, right, bottom, top, nearVal, farVal);

    // save matrix
    //glGetDoublev(GL_PROJECTION_MATRIX, _projectionMatrix);

    glMatrixMode(GL_MODELVIEW);
}


/////////////////////////////////////////////////////////////////////////////////////
// loadViewFrustum()
//
// Description: this will load the view frustum bounds for EITHER the head
//                 or the "myCamera".
//
void SvoViewer::loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum) 
{
    // We will use these below, from either the camera or head vectors calculated above
    glm::vec3 position(camera.getPosition());
    float fov         = camera.getFieldOfView();
    float nearClip    = camera.getNearClip();
    float farClip     = camera.getFarClip();
    float aspectRatio = camera.getAspectRatio();

    glm::quat rotation = camera.getRotation();

    // Set the viewFrustum up with the correct position and orientation of the camera
    viewFrustum.setPosition(position);
    viewFrustum.setOrientation(rotation);

    // Also make sure it's got the correct lens details from the camera
    viewFrustum.setAspectRatio(aspectRatio);
    viewFrustum.setFieldOfView(fov);
    viewFrustum.setNearClip(nearClip);
    viewFrustum.setFarClip(farClip);
    viewFrustum.setEyeOffsetPosition(camera.getEyeOffsetPosition());
    viewFrustum.setEyeOffsetOrientation(camera.getEyeOffsetOrientation());

    // Ask the ViewFrustum class to calculate our corners
    viewFrustum.calculate();
}

void SvoViewer::computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
	float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const 
{

    _viewFrustum.computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
}

void SvoViewer::updateCamera(float deltaTime) 
{
        /*if (Menu::getInstance()->isOptionChecked(MenuOption::OffAxisProjection)) {
            float xSign = _myCamera.getMode() == CAMERA_MODE_MIRROR ? 1.0f : -1.0f;
            if (_faceshift.isActive()) {
                const float EYE_OFFSET_SCALE = 0.025f;
                glm::vec3 position = _faceshift.getHeadTranslation() * EYE_OFFSET_SCALE;
                _myCamera.setEyeOffsetPosition(glm::vec3(position.x * xSign, position.y, -position.z));
                updateProjectionMatrix();
            }
        }*/
}

void SvoViewer::setupWorldLight() 
{
    //  Setup 3D lights (after the camera transform, so that they are positioned in world space)
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    //glm::vec3 sunDirection = getSunDirection();
	GLfloat light_position0[] = {10.0, 10.0, -20.0, 0.0};//{ sunDirection.x, sunDirection.y, sunDirection.z, 0.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
    GLfloat ambient_color[] = { 0.7f, 0.7f, 0.8f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color);
    GLfloat diffuse_color[] = { 0.8f, 0.7f, 0.7f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color);

    glLightfv(GL_LIGHT0, GL_SPECULAR, WHITE_SPECULAR_COLOR);
    glMaterialfv(GL_FRONT, GL_SPECULAR, WHITE_SPECULAR_COLOR);
    glMateriali(GL_FRONT, GL_SHININESS, 96);
}

inline glm::vec3 MAD(glm::vec3 v1, float mult, glm::vec3 v2)
{
	return glm::vec3(v1.x * mult + v2.x, v1.y * mult + v2.y, v1.z * mult + v2.z);
}

void SvoViewer::keyPressEvent(QKeyEvent* event) 
{
	int keyval = event->key();

	glm::vec3 lookAt = glm::normalize(_myCamera.getTargetPosition() - _myCamera.getPosition());
	glm::vec3 up	= glm::vec3(0.0,1.0f,0.0f);
	glm::vec3 right = glm::cross(lookAt, up);
	glm::vec3 rotY;
	switch (keyval) 
	{
		case Qt::Key_W:
			_myCamera.setPosition( MAD(lookAt, .2, _myCamera.getPosition()) );
			_myCamera.setTargetPosition( MAD(lookAt, .2, _myCamera.getTargetPosition()) );
			break;
		case Qt::Key_S:			
			_myCamera.setPosition( MAD(lookAt, -.2, _myCamera.getPosition()) );
			_myCamera.setTargetPosition( MAD(lookAt, -.2, _myCamera.getTargetPosition()) );
			break;
		case Qt::Key_A:
			_myCamera.setPosition( MAD(right, -.2, _myCamera.getPosition()) );
			_myCamera.setTargetPosition( MAD(right, -.2, _myCamera.getTargetPosition()) );
			break;
		case Qt::Key_D:
			_myCamera.setPosition( MAD(right, .2, _myCamera.getPosition()) );
			_myCamera.setTargetPosition( MAD(right, .2, _myCamera.getTargetPosition()) );
			break;
		case Qt::Key_R:
			_myCamera.setPosition( MAD(up, .2, _myCamera.getPosition()) );
			_myCamera.setTargetPosition( MAD(up, .2, _myCamera.getTargetPosition()) );
			break;
		case Qt::Key_F:
			_myCamera.setPosition( MAD(up, -.2, _myCamera.getPosition()) );
			_myCamera.setTargetPosition( MAD(up, -.2, _myCamera.getTargetPosition()) );
			break;
		case Qt::Key_Q: // rotate left
			_yaw += 10;
			_myCamera.setTargetRotation(glm::quat(glm::radians(glm::vec3(_pitch, _yaw, _roll))));
			break;
		case Qt::Key_E: // rotate right
			_yaw -= 10;
			_myCamera.setTargetRotation(glm::quat(glm::radians(glm::vec3(_pitch, _yaw, _roll))));
			break;
		case Qt::Key_B: // rotate right
			_useBoundingVolumes ^= 1;
			break;
		default:
			if (keyval >= Qt::Key_0 && keyval <= Qt::Key_9)
			{
				int newPartitionToDisplay = keyval - Qt::Key_0;
				if (_displayOnlyPartition == newPartitionToDisplay) _displayOnlyPartition = NO_PARTITION;
				else _displayOnlyPartition = newPartitionToDisplay;
			}


	}
	loadViewFrustum(_myCamera, _viewFrustum);
	UpdateOpt2BVFaceVisibility();
}


void SvoViewer::keyReleaseEvent(QKeyEvent* event) {}

void SvoViewer::mouseMoveEvent(QMouseEvent* event) 
{
    _mouseX = event->x();
    _mouseY = event->y();
	loadViewFrustum(_myCamera, _viewFrustum);
}

void SvoViewer::mousePressEvent(QMouseEvent* event) 
{
	if (event->button() == Qt::LeftButton) 
	{
            _mouseX = event->x();
            _mouseY = event->y();
            _mouseDragStartedX = _mouseX;
            _mouseDragStartedY = _mouseY;
            _mousePressed = true;
	}
   
}

void SvoViewer::mouseReleaseEvent(QMouseEvent* event) {
    
}


void SvoViewer::updateMouseRay() 
{
    // if the mouse pointer isn't visible, act like it's at the center of the screen
    float x = 0.5f, y = 0.5f;
    if (!_mouseHidden) {
        x = _mouseX / (float)_glWidget->width();
        y = _mouseY / (float)_glWidget->height();
    }
    _viewFrustum.computePickRay(x, y, _mouseRayOrigin, _mouseRayDirection);

    // adjust for mirroring
    if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        glm::vec3 mouseRayOffset = _mouseRayOrigin - _viewFrustum.getPosition();
        _mouseRayOrigin -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), mouseRayOffset) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), mouseRayOffset));
        _mouseRayDirection -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), _mouseRayDirection) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), _mouseRayDirection));
    }
}


// Stub for the full function.
bool SvoViewer::isVisibleBV(AABoundingVolume * volume, Camera * camera, ViewFrustum * frustum)
{
	//if (pos.z >= volume->getBound(2,AABF_HIGH)) return false;
	// Project all the points into screen space.
	AA2DBoundingVolume twoDBounds;
	//float xvals[2] = {9999.0, -1.0};
	//float yvals[2] = {9999.0, -1.0};
	//project all bv points into screen space.
	GLdouble scr[3];
	for (int i = 0; i < 8; i++)
	{
		glm::vec3 pt = volume->getCorner((BoxVertex)i);
		gluProject((GLdouble)pt.x, (GLdouble)pt.y, (GLdouble)pt.z, _modelviewMatrix, _projectionMatrix, _viewport, &scr[0], &scr[1], &scr[2]);		
		if (scr[2] > 0 && scr[2] < 1)
		{
			float tPt[2] = {(float)scr[0], (float)scr[1]};
			twoDBounds.AddToSet(tPt);
		}
	}
	bool inVisibleSpace = twoDBounds.clipToRegion(0, 0, _width, _height);
	return inVisibleSpace;
}

float SvoViewer::visibleAngleSubtended(AABoundingVolume * volume, Camera * camera, ViewFrustum * frustum)
{
	AA2DBoundingVolume twoDBounds;
	//float xvals[2] = {9999.0, -1.0};
	//float yvals[2] = {9999.0, -1.0};
	//project all bv points into screen space.
	GLdouble scr[3];
	for (int i = 0; i < 8; i++)
	{
		glm::vec3 pt = volume->getCorner((BoxVertex)i);
		gluProject((GLdouble)pt.x, (GLdouble)pt.y, (GLdouble)pt.z, _modelviewMatrix, _projectionMatrix, _viewport, &scr[0], &scr[1], &scr[2]);		
		if (scr[2] > 0 && scr[2] < 1)
		{
			float tPt[2] = {(float)scr[0], (float)scr[1]};
			twoDBounds.AddToSet(tPt);
		}
	}
	twoDBounds.clipToRegion(0, 0, _width, _height);
	float area = twoDBounds.getWidth() * twoDBounds.getHeight();
	if (area < 1) return 0.0;
	return area / (float)_pixelCount;
}

GLubyte SvoViewer::PrintGLErrorCode()
{
	GLubyte  err = glGetError();
	if( err != GL_NO_ERROR ) //qDebug("GL Error! : %x\n", err);
		qDebug("Error! : %u, %s\n", (unsigned int)err, gluErrorString(err));
	return err;
}