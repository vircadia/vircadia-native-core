//
//  OculusManager.cpp
//  interface/src/devices
//
//  Created by Stephen Birarda on 5/9/13.
//  Refactored by Ben Arnold on 6/30/2014
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include "OculusManager.h"

#include <QOpenGLFramebufferObject>

#include <glm/glm.hpp>
#include <UserActivityLogger.h>

#include "Application.h"

#ifdef HAVE_LIBOVR

using namespace OVR;

ProgramObject OculusManager::_program;
int OculusManager::_textureLocation;
int OculusManager::_eyeToSourceUVScaleLocation;
int OculusManager::_eyeToSourceUVOffsetLocation;
int OculusManager::_eyeRotationStartLocation;
int OculusManager::_eyeRotationEndLocation;
int OculusManager::_positionAttributeLocation;
int OculusManager::_colorAttributeLocation;
int OculusManager::_texCoord0AttributeLocation;
int OculusManager::_texCoord1AttributeLocation;
int OculusManager::_texCoord2AttributeLocation;
bool OculusManager::_isConnected = false;

ovrHmd OculusManager::_ovrHmd;
ovrHmdDesc OculusManager::_ovrHmdDesc;
ovrFovPort OculusManager::_eyeFov[ovrEye_Count];
ovrEyeRenderDesc OculusManager::_eyeRenderDesc[ovrEye_Count];
ovrSizei OculusManager::_renderTargetSize;
ovrVector2f OculusManager::_UVScaleOffset[ovrEye_Count][2];
GLuint OculusManager::_vertices[ovrEye_Count] = { 0, 0 };
GLuint OculusManager::_indices[ovrEye_Count] = { 0, 0 };
GLsizei OculusManager::_meshSize[ovrEye_Count] = { 0, 0 };
ovrFrameTiming OculusManager::_hmdFrameTiming;
ovrRecti OculusManager::_eyeRenderViewport[ovrEye_Count];
unsigned int OculusManager::_frameIndex = 0;
bool OculusManager::_frameTimingActive = false;
bool OculusManager::_programInitialized = false;
Camera* OculusManager::_camera = NULL;
int OculusManager::_activeEyeIndex = -1;

#endif

glm::vec3 OculusManager::_leftEyePosition = glm::vec3();
glm::vec3 OculusManager::_rightEyePosition = glm::vec3();

void OculusManager::connect() {
#ifdef HAVE_LIBOVR
    ovr_Initialize();

    _ovrHmd = ovrHmd_Create(0);
    if (_ovrHmd) {
        if (!_isConnected) {
            UserActivityLogger::getInstance().connectedDevice("hmd", "oculus");
        }
        _isConnected = true;
#if defined(__APPLE__) || defined(_WIN32)
        _eyeFov[0] = _ovrHmd->DefaultEyeFov[0];
        _eyeFov[1] = _ovrHmd->DefaultEyeFov[1];
#else
        ovrHmd_GetDesc(_ovrHmd, &_ovrHmdDesc);
        _eyeFov[0] = _ovrHmdDesc.DefaultEyeFov[0];
        _eyeFov[1] = _ovrHmdDesc.DefaultEyeFov[1];
#endif
        //Get texture size
        ovrSizei recommendedTex0Size = ovrHmd_GetFovTextureSize(_ovrHmd, ovrEye_Left,
                                                                _eyeFov[0], 1.0f);
        ovrSizei recommendedTex1Size = ovrHmd_GetFovTextureSize(_ovrHmd, ovrEye_Right,
                                                                _eyeFov[1], 1.0f);
        _renderTargetSize.w = recommendedTex0Size.w + recommendedTex1Size.w;
        _renderTargetSize.h = recommendedTex0Size.h;
        if (_renderTargetSize.h < recommendedTex1Size.h) {
            _renderTargetSize.h = recommendedTex1Size.h;
        }

        _eyeRenderDesc[0] = ovrHmd_GetRenderDesc(_ovrHmd, ovrEye_Left, _eyeFov[0]);
        _eyeRenderDesc[1] = ovrHmd_GetRenderDesc(_ovrHmd, ovrEye_Right, _eyeFov[1]);

#if defined(__APPLE__) || defined(_WIN32)
        ovrHmd_SetEnabledCaps(_ovrHmd, ovrHmdCap_LowPersistence);
#else
        ovrHmd_SetEnabledCaps(_ovrHmd, ovrHmdCap_LowPersistence | ovrHmdCap_LatencyTest);
#endif

#if defined(__APPLE__) || defined(_WIN32)
        ovrHmd_ConfigureTracking(_ovrHmd, ovrTrackingCap_Orientation | ovrTrackingCap_Position |
                                 ovrTrackingCap_MagYawCorrection,
                                 ovrTrackingCap_Orientation);
#else
        ovrHmd_StartSensor(_ovrHmd, ovrSensorCap_Orientation | ovrSensorCap_YawCorrection |
                           ovrSensorCap_Position,
                           ovrSensorCap_Orientation);
#endif

        if (!_camera) {
            _camera = new Camera;
        }

        if (!_programInitialized) {
            // Shader program
            _programInitialized = true;
            _program.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/oculus.vert");
            _program.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() + "shaders/oculus.frag");
            _program.link();

            // Uniforms
            _textureLocation = _program.uniformLocation("texture");
            _eyeToSourceUVScaleLocation = _program.uniformLocation("EyeToSourceUVScale");
            _eyeToSourceUVOffsetLocation = _program.uniformLocation("EyeToSourceUVOffset");
            _eyeRotationStartLocation = _program.uniformLocation("EyeRotationStart");
            _eyeRotationEndLocation = _program.uniformLocation("EyeRotationEnd");

            // Attributes
            _positionAttributeLocation = _program.attributeLocation("position");
            _colorAttributeLocation = _program.attributeLocation("color");
            _texCoord0AttributeLocation = _program.attributeLocation("texCoord0");
            _texCoord1AttributeLocation = _program.attributeLocation("texCoord1");
            _texCoord2AttributeLocation = _program.attributeLocation("texCoord2");
        }

        //Generate the distortion VBOs
        generateDistortionMesh();

    } else {
        _isConnected = false;
        
        // we're definitely not in "VR mode" so tell the menu that
        Menu::getInstance()->getActionForOption(MenuOption::EnableVRMode)->setChecked(false);
        
        ovrHmd_Destroy(_ovrHmd);
        ovr_Shutdown();
    }
#endif
}

//Disconnects and deallocates the OR
void OculusManager::disconnect() {
#ifdef HAVE_LIBOVR
    if (_isConnected) {
        _isConnected = false;
        ovrHmd_Destroy(_ovrHmd);
        ovr_Shutdown();

        //Free the distortion mesh data
        for (int i = 0; i < ovrEye_Count; i++) {
            if (_vertices[i] != 0) {
                glDeleteBuffers(1, &(_vertices[i]));
                _vertices[i] = 0;
            }
            if (_indices[i] != 0) {
                glDeleteBuffers(1, &(_indices[i]));
                _indices[i] = 0;
            }
        }
    }
#endif
}

#ifdef HAVE_LIBOVR
void OculusManager::generateDistortionMesh() {

    //Check if we already have the distortion mesh
    if (_vertices[0] != 0) {
        printf("WARNING: Tried to generate Oculus distortion mesh twice without freeing the VBOs.");
        return;
    }

    //Viewport for the render target for each eye
    _eyeRenderViewport[0].Pos = Vector2i(0, 0);
    _eyeRenderViewport[0].Size = Sizei(_renderTargetSize.w / 2, _renderTargetSize.h);
    _eyeRenderViewport[1].Pos = Vector2i((_renderTargetSize.w + 1) / 2, 0);
    _eyeRenderViewport[1].Size = _eyeRenderViewport[0].Size;

    for (int eyeNum = 0; eyeNum < ovrEye_Count; eyeNum++) {
        // Allocate and generate distortion mesh vertices
        ovrDistortionMesh meshData;
        ovrHmd_CreateDistortionMesh(_ovrHmd, _eyeRenderDesc[eyeNum].Eye, _eyeRenderDesc[eyeNum].Fov, _ovrHmdDesc.DistortionCaps, &meshData);
        
        ovrHmd_GetRenderScaleAndOffset(_eyeRenderDesc[eyeNum].Fov, _renderTargetSize, _eyeRenderViewport[eyeNum],
                                       _UVScaleOffset[eyeNum]);

        // Parse the vertex data and create a render ready vertex buffer
        DistortionVertex* pVBVerts = (DistortionVertex*)OVR_ALLOC(sizeof(DistortionVertex) * meshData.VertexCount);
        _meshSize[eyeNum] = meshData.IndexCount;

        // Convert the oculus vertex data to the DistortionVertex format.
        DistortionVertex* v = pVBVerts;
        ovrDistortionVertex* ov = meshData.pVertexData;
        for (unsigned int vertNum = 0; vertNum < meshData.VertexCount; vertNum++) {
#if defined(__APPLE__) || defined(_WIN32)
            v->pos.x = ov->ScreenPosNDC.x;
            v->pos.y = ov->ScreenPosNDC.y;
            v->texR.x = ov->TanEyeAnglesR.x;
            v->texR.y = ov->TanEyeAnglesR.y;
            v->texG.x = ov->TanEyeAnglesG.x;
            v->texG.y = ov->TanEyeAnglesG.y;
            v->texB.x = ov->TanEyeAnglesB.x;
            v->texB.y = ov->TanEyeAnglesB.y;
#else
            v->pos.x = ov->Pos.x;
            v->pos.y = ov->Pos.y;
            v->texR.x = ov->TexR.x;
            v->texR.y = ov->TexR.y;
            v->texG.x = ov->TexG.x;
            v->texG.y = ov->TexG.y;
            v->texB.x = ov->TexB.x;
            v->texB.y = ov->TexB.y;
#endif
            v->color.r = v->color.g = v->color.b = (GLubyte)(ov->VignetteFactor * 255.99f);
            v->color.a = (GLubyte)(ov->TimeWarpFactor * 255.99f);
            v++; 
            ov++;
        }

        //vertices
        glGenBuffers(1, &(_vertices[eyeNum]));
        glBindBuffer(GL_ARRAY_BUFFER, _vertices[eyeNum]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(DistortionVertex) * meshData.VertexCount, pVBVerts, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //indices
        glGenBuffers(1, &(_indices[eyeNum]));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indices[eyeNum]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * meshData.IndexCount, meshData.pIndexData, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
        //Now that we have the VBOs we can get rid of the mesh data
        OVR_FREE(pVBVerts);
        ovrHmd_DestroyDistortionMesh(&meshData);
    }

}
#endif

bool OculusManager::isConnected() {
#ifdef HAVE_LIBOVR
    return _isConnected && Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode);
#else
    return false;
#endif
}

//Begins the frame timing for oculus prediction purposes
void OculusManager::beginFrameTiming() {
#ifdef HAVE_LIBOVR

    if (_frameTimingActive) {
        printf("WARNING: Called OculusManager::beginFrameTiming() twice in a row, need to call OculusManager::endFrameTiming().");
    }

   _hmdFrameTiming = ovrHmd_BeginFrameTiming(_ovrHmd, _frameIndex);
   _frameTimingActive = true;
#endif
}

//Ends frame timing
void OculusManager::endFrameTiming() {
#ifdef HAVE_LIBOVR
    ovrHmd_EndFrameTiming(_ovrHmd);
    _frameIndex++;
    _frameTimingActive = false;
#endif
}

//Sets the camera FoV and aspect ratio
void OculusManager::configureCamera(Camera& camera, int screenWidth, int screenHeight) {
#ifdef HAVE_LIBOVR
    camera.setAspectRatio(_renderTargetSize.w / _renderTargetSize.h);
    camera.setFieldOfView(atan(_eyeFov[0].UpTan) * DEGREES_PER_RADIAN * 2.0f);
#endif    
}

//Displays everything for the oculus, frame timing must be active
void OculusManager::display(const glm::quat &bodyOrientation, const glm::vec3 &position, Camera& whichCamera) {
#ifdef HAVE_LIBOVR
    //beginFrameTiming must be called before display
    if (!_frameTimingActive) {
        printf("WARNING: Called OculusManager::display() without calling OculusManager::beginFrameTiming() first.");
        return;
    }

    ApplicationOverlay& applicationOverlay = Application::getInstance()->getApplicationOverlay();

    // We only need to render the overlays to a texture once, then we just render the texture on the hemisphere
    // PrioVR will only work if renderOverlay is called, calibration is connected to Application::renderingOverlay() 
    applicationOverlay.renderOverlay(true);
   
    //Bind our framebuffer object. If we are rendering the glow effect, we let the glow effect shader take care of it
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableGlowEffect)) {
        Application::getInstance()->getGlowEffect()->prepare();
    } else {
        Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    ovrPosef eyeRenderPose[ovrEye_Count];

    _camera->setTightness(0.0f);  //  In first person, camera follows (untweaked) head exactly without delay
    _camera->setDistance(0.0f);
    _camera->setUpShift(0.0f);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
  
    glm::quat orientation;
    glm::vec3 trackerPosition;
    
#if defined(__APPLE__) || defined(_WIN32)
    ovrTrackingState ts = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    ovrVector3f ovrHeadPosition = ts.HeadPose.ThePose.Position;
    
    trackerPosition = glm::vec3(ovrHeadPosition.x, ovrHeadPosition.y, ovrHeadPosition.z);
    trackerPosition = bodyOrientation * trackerPosition;
#endif
    
    //Render each eye into an fbo
    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++) {
        _activeEyeIndex = eyeIndex;
        
#if defined(__APPLE__) || defined(_WIN32)
        ovrEyeType eye = _ovrHmd->EyeRenderOrder[eyeIndex];
#else
        ovrEyeType eye = _ovrHmdDesc.EyeRenderOrder[eyeIndex];
#endif
        //Set the camera rotation for this eye
        eyeRenderPose[eye] = ovrHmd_GetEyePose(_ovrHmd, eye);
        orientation.x = eyeRenderPose[eye].Orientation.x;
        orientation.y = eyeRenderPose[eye].Orientation.y;
        orientation.z = eyeRenderPose[eye].Orientation.z;
        orientation.w = eyeRenderPose[eye].Orientation.w;
        
        _camera->setTargetRotation(bodyOrientation * orientation);
        _camera->setTargetPosition(position + trackerPosition);
        
        //  Store the latest left and right eye render locations for things that need to know
        glm::vec3 thisEyePosition = position + trackerPosition +
            (bodyOrientation * glm::quat(orientation.x, orientation.y, orientation.z, orientation.w) *
             glm::vec3(_eyeRenderDesc[eye].ViewAdjust.x, _eyeRenderDesc[eye].ViewAdjust.y, _eyeRenderDesc[eye].ViewAdjust.z));
        
        if (eyeIndex == 0) {
            _leftEyePosition = thisEyePosition;
        } else {
            _rightEyePosition = thisEyePosition;
        }

        _camera->update(1.0f / Application::getInstance()->getFps());

        Matrix4f proj = ovrMatrix4f_Projection(_eyeRenderDesc[eye].Fov, whichCamera.getNearClip(), whichCamera.getFarClip(), true);
        proj.Transpose();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glLoadMatrixf((GLfloat *)proj.M);

        glViewport(_eyeRenderViewport[eye].Pos.x, _eyeRenderViewport[eye].Pos.y,
                   _eyeRenderViewport[eye].Size.w, _eyeRenderViewport[eye].Size.h);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(_eyeRenderDesc[eye].ViewAdjust.x, _eyeRenderDesc[eye].ViewAdjust.y, _eyeRenderDesc[eye].ViewAdjust.z);
        
        Application::getInstance()->displaySide(*_camera);

        applicationOverlay.displayOverlayTextureOculus(*_camera);
        _activeEyeIndex = -1;
    }

    //Wait till time-warp to reduce latency
    ovr_WaitTillTime(_hmdFrameTiming.TimewarpPointSeconds);

    glPopMatrix();

    //Full texture viewport for glow effect
    glViewport(0, 0, _renderTargetSize.w, _renderTargetSize.h);
  
    //Bind the output texture from the glow shader. If glow effect is disabled, we just grab the texture
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableGlowEffect)) {
        QOpenGLFramebufferObject* fbo = Application::getInstance()->getGlowEffect()->render(true);
        glBindTexture(GL_TEXTURE_2D, fbo->texture());
    } else {
        Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->release();
        glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->texture());
    }

    // restore our normal viewport
    glViewport(0, 0, Application::getInstance()->getGLWidget()->getDeviceWidth(),
        Application::getInstance()->getGLWidget()->getDeviceHeight());

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    //Renders the distorted mesh onto the screen
    renderDistortionMesh(eyeRenderPose);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Update camera for use by rest of Interface.
    whichCamera.setTargetPosition((_leftEyePosition + _rightEyePosition) / 2.f);
    whichCamera.setTargetRotation(_camera->getTargetRotation());

#endif
}

#ifdef HAVE_LIBOVR
void OculusManager::renderDistortionMesh(ovrPosef eyeRenderPose[ovrEye_Count]) {

    glLoadIdentity();
    gluOrtho2D(0, Application::getInstance()->getGLWidget()->getDeviceWidth(), 0,
        Application::getInstance()->getGLWidget()->getDeviceHeight());

    glDisable(GL_DEPTH_TEST);

    glDisable(GL_BLEND);
    _program.bind();
    _program.setUniformValue(_textureLocation, 0);

    _program.enableAttributeArray(_positionAttributeLocation);
    _program.enableAttributeArray(_colorAttributeLocation);
    _program.enableAttributeArray(_texCoord0AttributeLocation);
    _program.enableAttributeArray(_texCoord1AttributeLocation);
    _program.enableAttributeArray(_texCoord2AttributeLocation);

    //Render the distortion meshes for each eye
    for (int eyeNum = 0; eyeNum < ovrEye_Count; eyeNum++) {
        GLfloat uvScale[2] = { _UVScaleOffset[eyeNum][0].x, _UVScaleOffset[eyeNum][0].y };
        _program.setUniformValueArray(_eyeToSourceUVScaleLocation, uvScale, 1, 2);
        GLfloat uvOffset[2] = { _UVScaleOffset[eyeNum][1].x, _UVScaleOffset[eyeNum][1].y };
        _program.setUniformValueArray(_eyeToSourceUVOffsetLocation, uvOffset, 1, 2);

        ovrMatrix4f timeWarpMatrices[2];
        Matrix4f transposeMatrices[2];
        //Grabs the timewarp matrices to be used in the shader
        ovrHmd_GetEyeTimewarpMatrices(_ovrHmd, (ovrEyeType)eyeNum, eyeRenderPose[eyeNum], timeWarpMatrices);
        transposeMatrices[0] = Matrix4f(timeWarpMatrices[0]);
        transposeMatrices[1] = Matrix4f(timeWarpMatrices[1]);

        //Have to transpose the matrices before using them
        transposeMatrices[0].Transpose();
        transposeMatrices[1].Transpose();

        glUniformMatrix4fv(_eyeRotationStartLocation, 1, GL_FALSE, (GLfloat *)transposeMatrices[0].M);
        glUniformMatrix4fv(_eyeRotationEndLocation, 1, GL_FALSE, (GLfloat *)transposeMatrices[1].M);

        glBindBuffer(GL_ARRAY_BUFFER, _vertices[eyeNum]);

        //Set vertex attribute pointers
        glVertexAttribPointer(_positionAttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(DistortionVertex), (void *)0);
        glVertexAttribPointer(_texCoord0AttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(DistortionVertex), (void *)8);
        glVertexAttribPointer(_texCoord1AttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(DistortionVertex), (void *)16);
        glVertexAttribPointer(_texCoord2AttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(DistortionVertex), (void *)24);
        glVertexAttribPointer(_colorAttributeLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DistortionVertex), (void *)32);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indices[eyeNum]);
        glDrawElements(GL_TRIANGLES, _meshSize[eyeNum], GL_UNSIGNED_SHORT, 0);
    }

    _program.disableAttributeArray(_positionAttributeLocation);
    _program.disableAttributeArray(_colorAttributeLocation);
    _program.disableAttributeArray(_texCoord0AttributeLocation);
    _program.disableAttributeArray(_texCoord1AttributeLocation);
    _program.disableAttributeArray(_texCoord2AttributeLocation);

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    _program.release();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
#endif

//Tries to reconnect to the sensors
void OculusManager::reset() {
#ifdef HAVE_LIBOVR
    if (_isConnected) {
        ovrHmd_RecenterPose(_ovrHmd);
    }
#endif
}

//Gets the current predicted angles from the oculus sensors
void OculusManager::getEulerAngles(float& yaw, float& pitch, float& roll) {
#ifdef HAVE_LIBOVR
#if defined(__APPLE__) || defined(_WIN32)
    ovrTrackingState ts = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
#else
    ovrSensorState ss = ovrHmd_GetSensorState(_ovrHmd, _hmdFrameTiming.ScanoutMidpointSeconds);
#endif
#if defined(__APPLE__) || defined(_WIN32) 
    if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
#else
    if (ss.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
#endif
        
#if defined(__APPLE__) || defined(_WIN32)
        ovrPosef headPose = ts.HeadPose.ThePose;
#else
        ovrPosef headPose = ss.Predicted.Pose;
#endif
        Quatf orientation = Quatf(headPose.Orientation);
        orientation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z, Rotate_CCW, Handed_R>(&yaw, &pitch, &roll);
    } else {
        yaw = 0.0f;
        pitch = 0.0f;
        roll = 0.0f;
    }
#else
    yaw = 0.0f;
    pitch = 0.0f;
    roll = 0.0f;
#endif
}
    
glm::vec3 OculusManager::getRelativePosition() {
#if (defined(__APPLE__) || defined(_WIN32)) && HAVE_LIBOVR
    ovrTrackingState trackingState = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    ovrVector3f headPosition = trackingState.HeadPose.ThePose.Position;
    
    return glm::vec3(headPosition.x, headPosition.y, headPosition.z);
#else
    // no positional tracking in Linux yet
    return glm::vec3(0.0f, 0.0f, 0.0f);
#endif
}

//Used to set the size of the glow framebuffers
QSize OculusManager::getRenderTargetSize() {
#ifdef HAVE_LIBOVR
    QSize rv;
    rv.setWidth(_renderTargetSize.w);
    rv.setHeight(_renderTargetSize.h);
    return rv;
#else
    return QSize(100, 100);
#endif
}

void OculusManager::overrideOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
        float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) {
#ifdef HAVE_LIBOVR
    if (_activeEyeIndex != -1) {
        const ovrFovPort& port = _eyeFov[_activeEyeIndex];
        right = nearVal * port.RightTan;
        left = -nearVal * port.LeftTan;
        top = nearVal * port.UpTan;
        bottom = -nearVal * port.DownTan;
    }
#endif
}
