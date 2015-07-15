//
//  AmbientOcclusionEffect.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 7/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include <gpu/GPUConfig.h>

#include <gpu/GLBackend.h>

#include <glm/gtc/random.hpp>

#include <PathUtils.h>
#include <SharedUtil.h>

#include "AbstractViewStateInterface.h"
#include "AmbientOcclusionEffect.h"
#include "GlowEffect.h"
#include "ProgramObject.h"
#include "RenderUtil.h"
#include "TextureCache.h"
#include "DependencyManager.h"
#include "ViewFrustum.h"

#include "ambient_occlusion_vert.h"
#include "ambient_occlusion_frag.h"

const int ROTATION_WIDTH = 4;
const int ROTATION_HEIGHT = 4;
    
void AmbientOcclusionEffect::init(AbstractViewStateInterface* viewState) {
    /*_viewState = viewState; // we will use this for view state services
    
    _occlusionProgram = new ProgramObject();
    _occlusionProgram->addShaderFromSourceFile(QGLShader::Vertex, PathUtils::resourcesPath()
                                               + "shaders/ambient_occlusion.vert");
    _occlusionProgram->addShaderFromSourceFile(QGLShader::Fragment, PathUtils::resourcesPath()
                                               + "shaders/ambient_occlusion.frag");
    _occlusionProgram->link();
    
    // create the sample kernel: an array of hemispherically distributed offset vectors
    const int SAMPLE_KERNEL_SIZE = 16;
    QVector3D sampleKernel[SAMPLE_KERNEL_SIZE];
    for (int i = 0; i < SAMPLE_KERNEL_SIZE; i++) {
        // square the length in order to increase density towards the center
        glm::vec3 vector = glm::sphericalRand(1.0f);
        float scale = randFloat();
        const float MIN_VECTOR_LENGTH = 0.01f;
        const float MAX_VECTOR_LENGTH = 1.0f;
        vector *= glm::mix(MIN_VECTOR_LENGTH, MAX_VECTOR_LENGTH, scale * scale);
        sampleKernel[i] = QVector3D(vector.x, vector.y, vector.z);
    }
    
    _occlusionProgram->bind();
    _occlusionProgram->setUniformValue("depthTexture", 0);
    _occlusionProgram->setUniformValue("rotationTexture", 1);
    _occlusionProgram->setUniformValueArray("sampleKernel", sampleKernel, SAMPLE_KERNEL_SIZE);
    _occlusionProgram->setUniformValue("radius", 0.1f);
    _occlusionProgram->release();
    
    _nearLocation = _occlusionProgram->uniformLocation("near");
    _farLocation = _occlusionProgram->uniformLocation("far");
    _leftBottomLocation = _occlusionProgram->uniformLocation("leftBottom");
    _rightTopLocation = _occlusionProgram->uniformLocation("rightTop");
    _noiseScaleLocation = _occlusionProgram->uniformLocation("noiseScale");
    _texCoordOffsetLocation = _occlusionProgram->uniformLocation("texCoordOffset");
    _texCoordScaleLocation = _occlusionProgram->uniformLocation("texCoordScale");
    
    // generate the random rotation texture
    glGenTextures(1, &_rotationTextureID);
    glBindTexture(GL_TEXTURE_2D, _rotationTextureID);
    const int ELEMENTS_PER_PIXEL = 3;
    unsigned char rotationData[ROTATION_WIDTH * ROTATION_HEIGHT * ELEMENTS_PER_PIXEL];
    unsigned char* rotation = rotationData;
    for (int i = 0; i < ROTATION_WIDTH * ROTATION_HEIGHT; i++) {
        glm::vec3 vector = glm::sphericalRand(1.0f);
        *rotation++ = ((vector.x + 1.0f) / 2.0f) * 255.0f;
        *rotation++ = ((vector.y + 1.0f) / 2.0f) * 255.0f;
        *rotation++ = ((vector.z + 1.0f) / 2.0f) * 255.0f;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ROTATION_WIDTH, ROTATION_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, rotationData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    _blurProgram = new ProgramObject();
    _blurProgram->addShaderFromSourceFile(QGLShader::Vertex, PathUtils::resourcesPath() + "shaders/ambient_occlusion.vert");
    _blurProgram->addShaderFromSourceFile(QGLShader::Fragment, PathUtils::resourcesPath() + "shaders/occlusion_blur.frag");
    _blurProgram->link();
    
    _blurProgram->bind();
    _blurProgram->setUniformValue("originalTexture", 0);
    _blurProgram->release();
    
    _blurScaleLocation = _blurProgram->uniformLocation("blurScale");*/
}

void AmbientOcclusionEffect::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext){
    /*glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glBindTexture(GL_TEXTURE_2D, DependencyManager::get<TextureCache>()->getPrimaryDepthTextureID());

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _rotationTextureID);

    // render with the occlusion shader to the secondary/tertiary buffer
    auto freeFramebuffer = DependencyManager::get<GlowEffect>()->getFreeFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(freeFramebuffer));

    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;
    AbstractViewStateInterface::instance()->computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int VIEWPORT_X_INDEX = 0;
    const int VIEWPORT_WIDTH_INDEX = 2;

    auto framebufferSize = DependencyManager::get<TextureCache>()->getFrameBufferSize();
    float sMin = viewport[VIEWPORT_X_INDEX] / (float)framebufferSize.width();
    float sWidth = viewport[VIEWPORT_WIDTH_INDEX] / (float)framebufferSize.width();

    _occlusionProgram->bind();
    _occlusionProgram->setUniformValue(_nearLocation, nearVal);
    _occlusionProgram->setUniformValue(_farLocation, farVal);
    _occlusionProgram->setUniformValue(_leftBottomLocation, left, bottom);
    _occlusionProgram->setUniformValue(_rightTopLocation, right, top);
    _occlusionProgram->setUniformValue(_noiseScaleLocation, viewport[VIEWPORT_WIDTH_INDEX] / (float)ROTATION_WIDTH,
        framebufferSize.height() / (float)ROTATION_HEIGHT);
    _occlusionProgram->setUniformValue(_texCoordOffsetLocation, sMin, 0.0f);
    _occlusionProgram->setUniformValue(_texCoordScaleLocation, sWidth, 1.0f);

    renderFullscreenQuad();

    _occlusionProgram->release();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);

    // now render secondary to primary with 4x4 blur
    auto primaryFramebuffer = DependencyManager::get<TextureCache>()->getPrimaryFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(primaryFramebuffer));

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);

    auto freeFramebufferTexture = freeFramebuffer->getRenderBuffer(0);
    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(freeFramebufferTexture));

    _blurProgram->bind();
    _blurProgram->setUniformValue(_blurScaleLocation, 1.0f / framebufferSize.width(), 1.0f / framebufferSize.height());

    renderFullscreenQuad(sMin, sMin + sWidth);

    _blurProgram->release();

    glBindTexture(GL_TEXTURE_2D, 0);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);*/
}


AmbientOcclusion::AmbientOcclusion() {
}

const gpu::PipelinePointer& AmbientOcclusion::getAOPipeline() {
    if (!_AOPipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(ambient_occlusion_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(ambient_occlusion_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        //_drawItemBoundPosLoc = program->getUniforms().findLocation("inBoundPos");
        //_drawItemBoundDimLoc = program->getUniforms().findLocation("inBoundDim");

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Good to go add the brand new pipeline
        _AOPipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _AOPipeline;
}

void AmbientOcclusion::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {

    // create a simple pipeline that does:

    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    RenderArgs* args = renderContext->args;
    auto& scene = sceneContext->_scene;

    // Allright, something to render let's do it
    gpu::Batch batch;

    glm::mat4 projMat;
    Transform viewMat;
    args->_viewFrustum->evalProjectionMatrix(projMat);
    args->_viewFrustum->evalViewTransform(viewMat);
    if (args->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
        viewMat.postScale(glm::vec3(-1.0f, 1.0f, 1.0f));
    }
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewMat);
    batch.setModelTransform(Transform());

    // bind the one gpu::Pipeline we need
    batch.setPipeline(getAOPipeline());

    //renderFullscreenQuad();

    args->_context->syncCache();
    renderContext->args->_context->syncCache();
    args->_context->render((batch));

    // need to fetch forom the z buffer and render something in a new render target a result that combine the z and produce a fake AO result



}

