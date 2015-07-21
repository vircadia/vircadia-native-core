//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Oculus_0_5_DisplayPlugin.h"

#if (OVR_MAJOR_VERSION == 5)

#include <memory>

#include <QMainWindow>
#include <QGLWidget>
#include <GLMHelpers.h>
#include <GlWindow.h>
#include <QEvent>
#include <QResizeEvent>
#include <QOpenGLContext>

#include <OVR_CAPI_GL.h>

#include <PerfStat.h>
#include <OglplusHelpers.h>

#include "plugins/PluginContainer.h"
#include "OculusHelpers.h"

using namespace oglplus;


#define RIFT_SDK_DISTORTION 0

const QString Oculus_0_5_DisplayPlugin::NAME("Oculus Rift (0.5)");

const QString & Oculus_0_5_DisplayPlugin::getName() const {
    return NAME;
}

bool Oculus_0_5_DisplayPlugin::isSupported() const {
    if (!ovr_Initialize(nullptr)) {
        return false;
    }
    bool result = false;
    if (ovrHmd_Detect() > 0) {
        result = true;
    }
    ovr_Shutdown();
    return result;
}

static const char* OVR_DISTORTION_VS = R"SHADER(#version 120
#pragma line __LINE__

//uniform vec2 EyeToSourceUVScale;
const vec2 EyeToSourceUVScale = vec2(0.232447237, 0.375487238);
uniform vec2 EyeToSourceUVOffset;
uniform mat4 EyeRotationStart;
uniform mat4 EyeRotationEnd;

attribute vec4 Position;
attribute vec2 TexCoord0;
attribute vec2 TexCoord1;
attribute vec2 TexCoord2;

varying vec2 oTexCoord0;
varying vec2 oTexCoord1;
varying vec2 oTexCoord2;

void main() {
    gl_Position.x = Position.x;
    gl_Position.y = Position.y; 
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
    oTexCoord0 = TexCoord0;
    oTexCoord1 = TexCoord1;
    oTexCoord2 = TexCoord2;

    // Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic aberration and distortion).
    // These are now "real world" vectors in direction (x,y,1) relative to the eye of the HMD.
    vec3 TanEyeAngleR = vec3 ( TexCoord0.x, TexCoord0.y, 1.0 );
    vec3 TanEyeAngleG = vec3 ( TexCoord1.x, TexCoord1.y, 1.0 );
    vec3 TanEyeAngleB = vec3 ( TexCoord2.x, TexCoord2.y, 1.0 );
    // Apply the two 3x3 timewarp rotations to these vectors.
    vec3 TransformedRStart = (EyeRotationStart * vec4(TanEyeAngleR, 0)).xyz;
    vec3 TransformedGStart = (EyeRotationStart * vec4(TanEyeAngleG, 0)).xyz;
    vec3 TransformedBStart = (EyeRotationStart * vec4(TanEyeAngleB, 0)).xyz;
    vec3 TransformedREnd   = (EyeRotationEnd * vec4(TanEyeAngleR, 0)).xyz;
    vec3 TransformedGEnd   = (EyeRotationEnd * vec4(TanEyeAngleG, 0)).xyz;
    vec3 TransformedBEnd   = (EyeRotationEnd * vec4(TanEyeAngleB, 0)).xyz;
    // And blend between them.
    vec3 TransformedR = mix ( TransformedRStart, TransformedREnd, Position.z );
    vec3 TransformedG = mix ( TransformedGStart, TransformedGEnd, Position.z );
    vec3 TransformedB = mix ( TransformedBStart, TransformedBEnd, Position.z );

    // Project them back onto the Z=1 plane of the rendered images.
    float RecipZR = 1.0 / TransformedR.z;
    float RecipZG = 1.0 / TransformedG.z;
    float RecipZB = 1.0 / TransformedB.z;
    vec2 FlattenedR = vec2 ( TransformedR.x * RecipZR, TransformedR.y * RecipZR );
    vec2 FlattenedG = vec2 ( TransformedG.x * RecipZG, TransformedG.y * RecipZG );
    vec2 FlattenedB = vec2 ( TransformedB.x * RecipZB, TransformedB.y * RecipZB );

    // These are now still in TanEyeAngle space.
    // Scale them into the correct [0-1],[0-1] UV lookup space (depending on eye)
    vec2 SrcCoordR = FlattenedR * EyeToSourceUVScale + EyeToSourceUVOffset;
    vec2 SrcCoordG = FlattenedG * EyeToSourceUVScale + EyeToSourceUVOffset;
    vec2 SrcCoordB = FlattenedB * EyeToSourceUVScale + EyeToSourceUVOffset;
//    vec2 SrcCoordR = FlattenedR  + EyeToSourceUVOffset;
//    vec2 SrcCoordG = FlattenedG  + EyeToSourceUVOffset;
//    vec2 SrcCoordB = FlattenedB  + EyeToSourceUVOffset;

    oTexCoord0 = SrcCoordR;
    oTexCoord1 = SrcCoordG;
    oTexCoord2 = SrcCoordB;
}

)SHADER";

static const char* OVR_DISTORTION_FS = R"SHADER(#version 120
#pragma line __LINE__
uniform sampler2D Texture0;
#extension GL_ARB_shader_texture_lod : enable
#extension GL_ARB_draw_buffers : enable
#extension GL_EXT_gpu_shader4 : enable

varying vec2 oTexCoord0;
varying vec2 oTexCoord1;
varying vec2 oTexCoord2;

void main() {
   gl_FragColor = vec4(oTexCoord0, 0, 1);
   gl_FragColor = texture2D(Texture0, oTexCoord0, 0.0);
   //gl_FragColor.a = 1.0;
}
)SHADER";

void Oculus_0_5_DisplayPlugin::activate(PluginContainer * container) {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        Q_ASSERT(false);
        qFatal("Failed to Initialize SDK");
    }
    _hmd = ovrHmd_Create(0);
    if (!_hmd) {
        qFatal("Failed to acquire HMD");
    }

    OculusBaseDisplayPlugin::activate(container);

    _window->makeCurrent();
    _hmdWindow = new GlWindow(QOpenGLContext::currentContext());

    QSurfaceFormat format;
    initSurfaceFormat(format);
    _hmdWindow->setFlags(Qt::FramelessWindowHint);
    _hmdWindow->setFormat(format);
    _hmdWindow->create();
    _hmdWindow->setGeometry(_hmd->WindowsPos.x, _hmd->WindowsPos.y, _hmd->Resolution.w, _hmd->Resolution.h);
    //_hmdWindow->setGeometry(0, -1080, _hmd->Resolution.w, _hmd->Resolution.h);
    _hmdWindow->show();
    _hmdWindow->installEventFilter(this);
    _hmdWindow->makeCurrent();
    ovrRenderAPIConfig config; memset(&config, 0, sizeof(ovrRenderAPIConfig));
    config.Header.API = ovrRenderAPI_OpenGL;
    config.Header.BackBufferSize = _hmd->Resolution;
    config.Header.Multisample = 1;
    int distortionCaps = 0
        | ovrDistortionCap_Vignette
        | ovrDistortionCap_Overdrive
        | ovrDistortionCap_TimeWarp
        ;

    memset(_eyeTextures, 0, sizeof(ovrTexture) * 2);
    ovr_for_each_eye([&](ovrEyeType eye) {
        auto& header = _eyeTextures[eye].Header;
        header.API = ovrRenderAPI_OpenGL;
        header.TextureSize = { (int)_desiredFramebufferSize.x, (int)_desiredFramebufferSize.y };
        header.RenderViewport.Size = header.TextureSize;
        header.RenderViewport.Size.w /= 2;
        if (eye == ovrEye_Right) {
            header.RenderViewport.Pos.x = header.RenderViewport.Size.w;
        }
    });
#if RIFT_SDK_DISTORTION
    ovrBool result = ovrHmd_ConfigureRendering(_hmd, &config, 0, _eyeFovs, nullptr);
    Q_ASSERT(result);
#else
    compileProgram(_distortProgram, OVR_DISTORTION_VS, OVR_DISTORTION_FS);

    auto uniforms = _distortProgram->ActiveUniforms();
    for (size_t i = 0; i < uniforms.Size(); ++i) {
        auto uniform = uniforms.At(i);
        qDebug() << uniform.Name().c_str() << " @ " << uniform.Index();
        if (uniform.Name() == String("EyeToSourceUVScale")) {
            _uniformScale = uniform.Index();
        } else if (uniform.Name() == String("EyeToSourceUVOffset")) {
            _uniformOffset = uniform.Index();
        } else if (uniform.Name() == String("EyeRotationStart")) {
            _uniformEyeRotStart = uniform.Index();
        } else if (uniform.Name() == String("EyeRotationEnd")) {
            _uniformEyeRotEnd = uniform.Index();
        } 
    }
    
    auto attribs = _distortProgram->ActiveAttribs();
    for (size_t i = 0; i < attribs.Size(); ++i) {
        auto attrib = attribs.At(i);
        qDebug() << attrib.Name().c_str() << " @ " << attrib.Index();
        if (attrib.Name() == String("Position")) {
            _attrPosition = attrib.Index();
        } else if (attrib.Name() == String("TexCoord0")) {
            _attrTexCoord0 = attrib.Index();
        } else if (attrib.Name() == String("TexCoord1")) {
            _attrTexCoord1 = attrib.Index();
        } else if (attrib.Name() == String("TexCoord2")) {
            _attrTexCoord2 = attrib.Index();
        }
    }

    ovr_for_each_eye([&](ovrEyeType eye) {
        ovrDistortionMesh meshData;
        ovrHmd_CreateDistortionMesh(_hmd, eye, _eyeFovs[eye], distortionCaps, &meshData);
        {
            auto& buffer = _eyeVertexBuffers[eye];
            buffer.reset(new oglplus::Buffer());
            buffer->Bind(Buffer::Target::Array);
            buffer->Data(Buffer::Target::Array, BufferData(meshData.VertexCount, meshData.pVertexData));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        {
            auto& buffer = _eyeIndexBuffers[eye];
            buffer.reset(new oglplus::Buffer());
            buffer->Bind(Buffer::Target::ElementArray);
            buffer->Data(Buffer::Target::ElementArray, BufferData(meshData.IndexCount, meshData.pIndexData));
            _indexCount[eye] = meshData.IndexCount;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
        ovrHmd_DestroyDistortionMesh(&meshData);
        const auto& header = _eyeTextures[eye].Header;
        ovrHmd_GetRenderScaleAndOffset(_eyeFovs[eye], header.TextureSize, header.RenderViewport, _scaleAndOffset[eye]);
    });

#endif



}

void Oculus_0_5_DisplayPlugin::deactivate(PluginContainer* container) {
    _hmdWindow->deleteLater();
    _hmdWindow = nullptr;

    OculusBaseDisplayPlugin::deactivate(container);

    ovrHmd_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
}

static ovrFrameTiming timing;

void Oculus_0_5_DisplayPlugin::preRender() {
#if RIFT_SDK_DISTORTION
    ovrHmd_BeginFrame(_hmd, _frameIndex);
#else
    timing = ovrHmd_BeginFrameTiming(_hmd, _frameIndex);
#endif
}

void Oculus_0_5_DisplayPlugin::preDisplay() {
    _hmdWindow->makeCurrent();
}

void Oculus_0_5_DisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
    ++_frameIndex;

#if RIFT_SDK_DISTORTION
    ovr_for_each_eye([&](ovrEyeType eye) {
        reinterpret_cast<ovrGLTexture&>(_eyeTextures[eye]).OGL.TexId = finalTexture;
    });
    ovrHmd_EndFrame(_hmd, _eyePoses, _eyeTextures);
#else
    ovr_WaitTillTime(timing.TimewarpPointSeconds);
    glViewport(0, 0, _hmd->Resolution.w, _hmd->Resolution.h);

    //_distortProgram->Bind();
    glClearColor(1, 0, 1, 1);
    Context::Clear().ColorBuffer();
    Context::Disable(Capability::CullFace);
    _distortProgram->Bind();
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    glViewport(0, 0, _hmd->Resolution.w, _hmd->Resolution.h);
    ovr_for_each_eye([&](ovrEyeType eye){
//        ovrMatrix4f timeWarpMatrices[2];
//        ovrHmd_GetEyeTimewarpMatrices(_hmd, eye, _eyePoses[eye], timeWarpMatrices);
//        glUniformMatrix4fv(_uniformEyeRotStart, 1, GL_TRUE, &(timeWarpMatrices[0].M[0][0]));
//        glUniformMatrix4fv(_uniformEyeRotEnd, 1, GL_TRUE, &(timeWarpMatrices[1].M[0][0]));
        mat4 identity;
        glUniformMatrix4fv(_uniformEyeRotStart, 1, GL_FALSE, &identity[0][0]);
        glUniformMatrix4fv(_uniformEyeRotEnd, 1, GL_FALSE, &identity[0][0]);

        vec2 scale(1);
//        glUniform2fv(_uniformScale, 1, &_scaleAndOffset[eye][0].x);
        glUniform2fv(_uniformScale, 1, &scale.x);

        glUniform2fv(_uniformOffset, 1, &_scaleAndOffset[eye][1].x);

        _eyeVertexBuffers[eye]->Bind(Buffer::Target::Array);
        glEnableVertexAttribArray(_attrPosition);
        size_t offset;
        offset = offsetof(ovrDistortionVertex, ScreenPosNDC);
        glVertexAttribPointer(_attrPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex), (void*)offset);
        glEnableVertexAttribArray(_attrTexCoord0);
        offset = offsetof(ovrDistortionVertex, TanEyeAnglesR);
        glVertexAttribPointer(_attrTexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex), (void*)offset);
        glEnableVertexAttribArray(_attrTexCoord1);
        offset = offsetof(ovrDistortionVertex, TanEyeAnglesG);
        glVertexAttribPointer(_attrTexCoord1, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex), (void*)offset);
        glEnableVertexAttribArray(_attrTexCoord2);
        offset = offsetof(ovrDistortionVertex, TanEyeAnglesB);
        glVertexAttribPointer(_attrTexCoord2, 2, GL_FLOAT, GL_FALSE, sizeof(ovrDistortionVertex), (void*)offset);
        _eyeIndexBuffers[eye]->Bind(Buffer::Target::ElementArray);
        glDrawElements(GL_TRIANGLES, _indexCount[eye], GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(_attrPosition);
        glDisableVertexAttribArray(_attrTexCoord0);
        glDisableVertexAttribArray(_attrTexCoord1);
        glDisableVertexAttribArray(_attrTexCoord2);
    });
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif

}

// Pass input events on to the application
bool Oculus_0_5_DisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    return OculusBaseDisplayPlugin::eventFilter(receiver, event);
}

/*
    The swapbuffer call here is only required if we want to mirror the content to the screen.
    However, it should only be done if we can reliably disable v-sync on the mirror surface,
    otherwise the swapbuffer delay will interefere with the framerate of the headset
    */
void Oculus_0_5_DisplayPlugin::finishFrame() {
    _hmdWindow->swapBuffers();
    _hmdWindow->doneCurrent();
};

#endif
