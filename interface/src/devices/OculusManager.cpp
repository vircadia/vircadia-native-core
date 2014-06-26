//
//  OculusManager.cpp
//  interface/src/devices
//
//  Created by Stephen Birarda on 5/9/13.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include "OculusManager.h"

#include <QOpenGLFramebufferObject>

#include <glm/glm.hpp>

#include "Application.h"

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

#ifdef HAVE_LIBOVR

ovrHmd OculusManager::_ovrHmd;
ovrHmdDesc OculusManager::_ovrHmdDesc;
ovrFovPort OculusManager::_eyeFov[ovrEye_Count];
ovrSizei OculusManager::_renderTargetSize;
ovrVector2f OculusManager::_UVScaleOffset[ovrEye_Count][2];
GLuint  OculusManager::_vertices[ovrEye_Count] = { 0, 0 };
GLuint OculusManager::_indices[ovrEye_Count] = { 0, 0 };
GLsizei OculusManager::_meshSize[ovrEye_Count] = { 0, 0 };
ovrFrameTiming OculusManager::_hmdFrameTiming;
unsigned int OculusManager::_frameIndex = 0;
bool OculusManager::_frameTimingActive = false;
bool OculusManager::_programInitialized = false;

#endif

void OculusManager::connect() {
#ifdef HAVE_LIBOVR
    ovr_Initialize();

    _ovrHmd = ovrHmd_Create(0);
    if (_ovrHmd) {
        _isConnected = true;
      
        ovrHmd_GetDesc(_ovrHmd, &_ovrHmdDesc);

        _eyeFov[0] = _ovrHmdDesc.DefaultEyeFov[0];
        _eyeFov[1] = _ovrHmdDesc.DefaultEyeFov[1];

        //Get texture size
        ovrSizei recommendedTex0Size = ovrHmd_GetFovTextureSize(_ovrHmd, ovrEye_Left,
                                                                _ovrHmdDesc.DefaultEyeFov[0], 1.0f);
        ovrSizei recommendedTex1Size = ovrHmd_GetFovTextureSize(_ovrHmd, ovrEye_Right,
                                                                _ovrHmdDesc.DefaultEyeFov[1], 1.0f);
        _renderTargetSize.w = recommendedTex0Size.w + recommendedTex1Size.w;
        _renderTargetSize.h = recommendedTex0Size.h;
        if (_renderTargetSize.h < recommendedTex1Size.h) {
            _renderTargetSize.h = recommendedTex1Size.h;
        }

        ovrHmd_StartSensor(_ovrHmd, ovrSensorCap_Orientation | ovrSensorCap_YawCorrection |
                           ovrSensorCap_Position,
                           ovrSensorCap_Orientation);


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
        ovrHmd_Destroy(_ovrHmd);
        ovr_Shutdown();
    }
#endif
}

//Disconnects and deallocates the OR
void OculusManager::disconnect() {
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
}

void OculusManager::generateDistortionMesh() {
#ifdef HAVE_LIBOVR
    //Check if we already have the distortion mesh
    if (_vertices[0] != 0) {
        printf("WARNING: Tried to generate Oculus distortion mesh twice without freeing the VBOs.");
        return;
    }

    ovrEyeRenderDesc eyeDesc[ovrEye_Count];
    eyeDesc[0] = ovrHmd_GetRenderDesc(_ovrHmd, ovrEye_Left, _eyeFov[0]);
    eyeDesc[1] = ovrHmd_GetRenderDesc(_ovrHmd, ovrEye_Right, _eyeFov[1]);

    ovrRecti eyeRenderViewport[ovrEye_Count];
    eyeRenderViewport[0].Pos = Vector2i(0, 0);
    eyeRenderViewport[0].Size = Sizei(_renderTargetSize.w / 2, _renderTargetSize.h);
    eyeRenderViewport[1].Pos = Vector2i((_renderTargetSize.w + 1) / 2, 0);
    eyeRenderViewport[1].Size = eyeRenderViewport[0].Size;

    for (int eyeNum = 0; eyeNum < ovrEye_Count; eyeNum++) {
        // Allocate and generate distortion mesh vertices
        ovrDistortionMesh meshData;
        ovrHmd_CreateDistortionMesh(_ovrHmd, eyeDesc[eyeNum].Eye, eyeDesc[eyeNum].Fov, _ovrHmdDesc.DistortionCaps, &meshData);
        
        ovrHmd_GetRenderScaleAndOffset(eyeDesc[eyeNum].Fov, _renderTargetSize, eyeRenderViewport[eyeNum],
                                       _UVScaleOffset[eyeNum]);

        // Parse the vertex data and create a render ready vertex buffer
        DistortionVertex* pVBVerts = (DistortionVertex*)OVR_ALLOC(sizeof(DistortionVertex) * meshData.VertexCount);
        _meshSize[eyeNum] = meshData.IndexCount;

        // Convert the oculus vertex data to the DistortionVertex format.
        DistortionVertex* v = pVBVerts;
        ovrDistortionVertex* ov = meshData.pVertexData;
        for (unsigned int vertNum = 0; vertNum < meshData.VertexCount; vertNum++) {
            v->pos.x = ov->Pos.x;
            v->pos.y = ov->Pos.y;
            v->texR.x = ov->TexR.x;
            v->texR.y = ov->TexR.y;
            v->texG.x = ov->TexG.x;
            v->texG.y = ov->TexG.y;
            v->texB.x = ov->TexB.x;
            v->texB.y = ov->TexB.y;
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
#endif
}

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
    ovrSizei recommendedTex0Size = ovrHmd_GetFovTextureSize(_ovrHmd, ovrEye_Left,
                                                         _ovrHmdDesc.DefaultEyeFov[0], 1.0f);
    ovrSizei recommendedTex1Size = ovrHmd_GetFovTextureSize(_ovrHmd, ovrEye_Right,
                                                         _ovrHmdDesc.DefaultEyeFov[1], 1.0f);
    
    float width = recommendedTex0Size.w + recommendedTex1Size.w;
    float height = recommendedTex0Size.h;
    if (height < recommendedTex1Size.h) {
        height = recommendedTex1Size.h;
    }
    
    camera.setAspectRatio(width / height);
    camera.setFieldOfView(atan(_eyeFov[0].UpTan) * DEGREES_PER_RADIAN * 2.0f);
#endif    
}

//Displays everything for the oculus, frame timing must be active
void OculusManager::display(Camera& whichCamera) {
#ifdef HAVE_LIBOVR
    //beginFrameTiming must be called before display
    if (!_frameTimingActive) {
        printf("WARNING: Called OculusManager::display() without calling OculusManager::beginFrameTiming() first.");
        return;
    }

    ovrEyeRenderDesc eyeDesc[ovrEye_Count];
    eyeDesc[0] = ovrHmd_GetRenderDesc(_ovrHmd, ovrEye_Left, _eyeFov[0]);
    eyeDesc[1] = ovrHmd_GetRenderDesc(_ovrHmd, ovrEye_Right, _eyeFov[1]);
    ApplicationOverlay& applicationOverlay = Application::getInstance()->getApplicationOverlay();

    // We only need to render the overlays to a texture once, then we just render the texture as a quad
    // PrioVR will only work if renderOverlay is called, calibration is connected to Application::renderingOverlay() 
    applicationOverlay.renderOverlay(true);
    const bool displayOverlays = Menu::getInstance()->isOptionChecked(MenuOption::DisplayOculusOverlays);
    
    Application::getInstance()->getGlowEffect()->prepare(); 

    ovrPosef eyeRenderPose[ovrEye_Count];

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    eyeRenderPose[0] = ovrHmd_GetEyePose(_ovrHmd, ovrEye_Left);
    eyeRenderPose[1] = ovrHmd_GetEyePose(_ovrHmd, ovrEye_Right);

    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++) {

        ovrEyeType eye = _ovrHmdDesc.EyeRenderOrder[eyeIndex];

        Matrix4f proj = ovrMatrix4f_Projection(eyeDesc[eye].Fov, whichCamera.getNearClip(), whichCamera.getFarClip(), true);
        proj.Transpose();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glLoadMatrixf((GLfloat *)proj.M);

        glViewport(eyeDesc[eye].DistortedViewport.Pos.x, eyeDesc[eye].DistortedViewport.Pos.y,
                   eyeDesc[eye].DistortedViewport.Size.w, eyeDesc[eye].DistortedViewport.Size.h);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(eyeDesc[eye].ViewAdjust.x, eyeDesc[eye].ViewAdjust.y, eyeDesc[eye].ViewAdjust.z);

        Application::getInstance()->displaySide(whichCamera);

        if (displayOverlays) {
            applicationOverlay.displayOverlayTextureOculus(whichCamera);
        }
    }

    //Wait till time-warp to reduce latency
    ovr_WaitTillTime(_hmdFrameTiming.TimewarpPointSeconds);

    glPopMatrix();

    // restore our normal viewport
    glViewport(0, 0, Application::getInstance()->getGLWidget()->width(), Application::getInstance()->getGLWidget()->height());

    //Bind the output texture from the glow shader
    QOpenGLFramebufferObject* fbo = Application::getInstance()->getGlowEffect()->render(true);
    glBindTexture(GL_TEXTURE_2D, fbo->texture());

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glLoadIdentity();
    gluOrtho2D(0, Application::getInstance()->getGLWidget()->width(), 0, Application::getInstance()->getGLWidget()->height());
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
        ovrHmd_GetEyeTimewarpMatrices(_ovrHmd, (ovrEyeType)eyeNum, eyeRenderPose[eyeNum], timeWarpMatrices);
        transposeMatrices[0] = Matrix4f(timeWarpMatrices[0]);
        transposeMatrices[1] = Matrix4f(timeWarpMatrices[1]);

        transposeMatrices[0].Transpose();
        transposeMatrices[1].Transpose();
    
        glUniformMatrix4fv(_eyeRotationStartLocation, 1, GL_FALSE, (GLfloat *)transposeMatrices[0].M);
      
        glUniformMatrix4fv(_eyeRotationEndLocation, 1, GL_FALSE, (GLfloat *)transposeMatrices[1].M);
     
        glBindBuffer(GL_ARRAY_BUFFER, _vertices[eyeNum]);
        
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    _program.release();
#endif
}

//Tries to reconnect to the sensors
void OculusManager::reset() {
#ifdef HAVE_LIBOVR
    disconnect();
    connect();
#endif
}

//Gets the current predicted angles from the oculus sensors
void OculusManager::getEulerAngles(float& yaw, float& pitch, float& roll) {
#ifdef HAVE_LIBOVR
    ovrSensorState ss = ovrHmd_GetSensorState(_ovrHmd, _hmdFrameTiming.ScanoutMidpointSeconds);

    if (ss.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
        ovrPosef pose = ss.Predicted.Pose;
        Quatf orientation = Quatf(pose.Orientation);
        orientation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z, Rotate_CCW, Handed_R>(&yaw, &pitch, &roll);
    }
#endif
}

