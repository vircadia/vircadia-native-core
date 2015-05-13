//
//  Skybox.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 5/4/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Skybox.h"

#include "gpu/Batch.h"
#include "gpu/Context.h"

#include "ViewFrustum.h"
#include "Skybox_vert.h"
#include "Skybox_frag.h"

using namespace model;

Skybox::Skybox() {

/* // PLease create a default engineer skybox
    _cubemap.reset( gpu::Texture::createCube(gpu::Element::COLOR_RGBA_32, 1));
    unsigned char texels[] = {
        255, 0, 0, 255,
        0, 255, 255, 255,
        0, 0, 255, 255,
        255, 255, 0, 255,
        0, 255, 0, 255,
        255, 0, 255, 255,
    };
    _cubemap->assignStoredMip(0, gpu::Element::COLOR_RGBA_32, sizeof(texels), texels);*/
}

void Skybox::setColor(const Color& color) {
    _color = color;
}

void Skybox::setCubemap(const gpu::TexturePointer& cubemap) {
    if (_isSHValid && (cubemap != _cubemap)) {
        _isSHValid = false;
    }
    _cubemap = cubemap;
}

void Skybox::clearCubemap() {
    _cubemap.reset();
}

void Skybox::render(gpu::Batch& batch, const ViewFrustum& viewFrustum, const Skybox& skybox) {

    if (skybox.getCubemap() && skybox.getCubemap()->isDefined()) {

        skybox.getIrradianceSH();

        static gpu::PipelinePointer thePipeline;
        static gpu::BufferPointer theBuffer;
        static gpu::Stream::FormatPointer theFormat;
        static gpu::BufferPointer theConstants;
        int SKYBOX_CONSTANTS_SLOT = 0; // need to be defined by the compilation of the shader
        if (!thePipeline) {
            auto skyVS = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(Skybox_vert)));
            auto skyFS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(Skybox_frag)));
            auto skyShader = gpu::ShaderPointer(gpu::Shader::createProgram(skyVS, skyFS));

            gpu::Shader::BindingSet bindings;
            bindings.insert(gpu::Shader::Binding(std::string("cubeMap"), 0));
            if (!gpu::Shader::makeProgram(*skyShader, bindings)) {

            }

            SKYBOX_CONSTANTS_SLOT = skyShader->getBuffers().findLocation("skyboxBuffer");
            if (SKYBOX_CONSTANTS_SLOT == gpu::Shader::INVALID_LOCATION) {
                SKYBOX_CONSTANTS_SLOT = skyShader->getUniforms().findLocation("skyboxBuffer");
            }
            
            auto skyState = gpu::StatePointer(new gpu::State());

            thePipeline = gpu::PipelinePointer(gpu::Pipeline::create(skyShader, skyState));
        
            const float CLIP = 1.0;
            const glm::vec2 vertices[4] = { {-CLIP, -CLIP}, {CLIP, -CLIP}, {-CLIP, CLIP}, {CLIP, CLIP}};
            theBuffer.reset(new gpu::Buffer(sizeof(vertices), (const gpu::Byte*) vertices));
        
            theFormat.reset(new gpu::Stream::Format());
            theFormat->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::XYZ));
        
            auto color = glm::vec4(1.0f);
            theConstants.reset(new gpu::Buffer(sizeof(color), (const gpu::Byte*) &color));
        }

        glm::mat4 projMat;
        viewFrustum.evalProjectionMatrix(projMat);

        Transform viewTransform;
        viewFrustum.evalViewTransform(viewTransform);

        if (glm::all(glm::equal(skybox.getColor(), glm::vec3(0.0f)))) { 
            auto color = glm::vec4(1.0f);
            theConstants->setSubData(0, sizeof(color), (const gpu::Byte*) &color);
        } else {
            theConstants->setSubData(0, sizeof(Color), (const gpu::Byte*) &skybox.getColor());
        }

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewTransform);
        batch.setModelTransform(Transform()); // only for Mac
        batch.setPipeline(thePipeline);
        batch.setInputBuffer(gpu::Stream::POSITION, theBuffer, 0, 8);
        batch.setUniformBuffer(SKYBOX_CONSTANTS_SLOT, theConstants, 0, theConstants->getSize());
        batch.setInputFormat(theFormat);
        batch.setUniformTexture(0, skybox.getCubemap());
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    } else {
        // skybox has no cubemap, just clear the color buffer
        auto color = skybox.getColor();
        batch.clearFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(skybox.getColor(),1.0f), 0.f, 0); 
    }
}


glm::vec3 sRGBToLinear(glm::vec3& color) {
    const float GAMMA_CORRECTION = 2.2f;
    return glm::pow(color, glm::vec3(GAMMA_CORRECTION));
}

glm::vec3 linearTosRGB(glm::vec3& color) {
    const float GAMMA_CORRECTION_INV = 1.0f / 2.2f;
    return glm::pow(color, glm::vec3(GAMMA_CORRECTION_INV));
}

// Originial code for the Spherical Harmonics taken from "Sun and Black Cat- Igor Dykhta (igor dykhta email) © 2007-2014 "
void sphericalHarmonicsAdd(float * result, int order, const float * inputA, const float * inputB) {
   const int numCoeff = order * order;
   for(int i=0; i < numCoeff; i++) {
      result[i] = inputA[i] + inputB[i];
   }
}

void sphericalHarmonicsScale(float * result, int order, const float * input, float scale) {
   const int numCoeff = order * order;
   for(int i=0; i < numCoeff; i++) {
      result[i] = input[i] * scale;
   }
}

void sphericalHarmonicsEvaluateDirection(float * result, int order,  const glm::vec3 & dir) {
   // calculate coefficients for first 3 bands of spherical harmonics
   double P_0_0 = 0.282094791773878140;
   double P_1_0 = 0.488602511902919920 * dir.z;
   double P_1_1 = -0.488602511902919920;
   double P_2_0 = 0.946174695757560080 * dir.z * dir.z - 0.315391565252520050;
   double P_2_1 = -1.092548430592079200 * dir.z;
   double P_2_2 = 0.546274215296039590;
   result[0] = P_0_0;
   result[1] = P_1_1 * dir.y;
   result[2] = P_1_0;
   result[3] = P_1_1 * dir.x;
   result[4] = P_2_2 * (dir.x * dir.y + dir.y * dir.x);
   result[5] = P_2_1 * dir.y;
   result[6] = P_2_0;
   result[7] = P_2_1 * dir.x;
   result[8] = P_2_2 * (dir.x * dir.x - dir.y * dir.y);
}

void sphericalHarmonicsFromTexture(const gpu::Texture& cubeTexture, std::vector<glm::vec3> & output, const uint order) {
    const uint sqOrder = order*order;

    // allocate memory for calculations
    output.resize(sqOrder);
    std::vector<float> resultR(sqOrder);
    std::vector<float> resultG(sqOrder);
    std::vector<float> resultB(sqOrder);

    int width, height;

    // initialize values
    float fWt = 0.0f;
    for(uint i=0; i < sqOrder; i++) {
        output[i] = glm::vec3(0.0f);
        resultR[i] = 0.0f;
        resultG[i] = 0;
        resultB[i] = 0;
    }
    std::vector<float> shBuff(sqOrder);
    std::vector<float> shBuffB(sqOrder);
    // get width and height
    width = height = cubeTexture.getWidth();
    if(width != height) {
        return;
    }

    const float UCHAR_TO_FLOAT = 1.0f / float(std::numeric_limits<unsigned char>::max());

    // for each face of cube texture
    for(int face=0; face < gpu::Texture::NUM_CUBE_FACES; face++) {

        auto numComponents = cubeTexture.accessStoredMipFace(0,face)->_format.getDimensionCount();
        auto data = cubeTexture.accessStoredMipFace(0,face)->_sysmem.readData();
        if (data == nullptr) {
            continue;
        }

        // step between two texels for range [0, 1]
        float invWidth = 1.0f / float(width);
        // initial negative bound for range [-1, 1]
        float negativeBound = -1.0f + invWidth;
        // step between two texels for range [-1, 1]
        float invWidthBy2 = 2.0f / float(width);

        for(int y=0; y < width; y++) {
            // texture coordinate V in range [-1 to 1]
            const float fV = negativeBound + float(y) * invWidthBy2;

            for(int x=0; x < width; x++) {
                // texture coordinate U in range [-1 to 1]
                const float fU = negativeBound + float(x) * invWidthBy2;

                // determine direction from center of cube texture to current texel
                glm::vec3 dir;
                switch(face) {
                case gpu::Texture::CUBE_FACE_RIGHT_POS_X: {
                    dir.x = 1.0f;
                    dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir.z = 1.0f - (invWidthBy2 * float(x) + invWidth);
                    dir = -dir;
                    break;
                }
                case gpu::Texture::CUBE_FACE_LEFT_NEG_X: {
                    dir.x = -1.0f;
                    dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir.z = -1.0f + (invWidthBy2 * float(x) + invWidth);
                    dir = -dir;
                    break;
                }
                case gpu::Texture::CUBE_FACE_TOP_POS_Y: {
                    dir.x = - 1.0f + (invWidthBy2 * float(x) + invWidth);
                    dir.y = 1.0f;
                    dir.z = - 1.0f + (invWidthBy2 * float(y) + invWidth);
                    dir = -dir;
                    break;
                }
                case gpu::Texture::CUBE_FACE_BOTTOM_NEG_Y: {
                    dir.x = - 1.0f + (invWidthBy2 * float(x) + invWidth);
                    dir.y = - 1.0f;
                    dir.z = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir = -dir;
                    break;
                }
                case gpu::Texture::CUBE_FACE_BACK_POS_Z: {
                    dir.x = - 1.0f + (invWidthBy2 * float(x) + invWidth);
                    dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir.z = 1.0f;
                    break;
                }
                case gpu::Texture::CUBE_FACE_FRONT_NEG_Z: {
                    dir.x = 1.0f - (invWidthBy2 * float(x) + invWidth);
                    dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir.z = - 1.0f;
                    break;
                }
                default:
                    return;
                }

                // normalize direction
                dir = glm::normalize(dir);

                // scale factor depending on distance from center of the face
                const float fDiffSolid = 4.0f / ((1.0f + fU*fU + fV*fV) *
                                            sqrtf(1.0f + fU*fU + fV*fV));
                fWt += fDiffSolid;

                // calculate coefficients of spherical harmonics for current direction
                sphericalHarmonicsEvaluateDirection(shBuff.data(), order, dir);

                // index of texel in texture
                uint pixOffsetIndex = (x + y * width) * numComponents;

                // get color from texture and map to range [0, 1]
                glm::vec3 clr(float(data[pixOffsetIndex]) * UCHAR_TO_FLOAT,
                            float(data[pixOffsetIndex+1]) * UCHAR_TO_FLOAT,
                            float(data[pixOffsetIndex+2]) * UCHAR_TO_FLOAT);

                // Gamma correct
                clr = sRGBToLinear(clr);

                // scale color and add to previously accumulated coefficients
                sphericalHarmonicsScale(shBuffB.data(), order,
                        shBuff.data(), clr.r * fDiffSolid);
                sphericalHarmonicsAdd(resultR.data(), order,
                        resultR.data(), shBuffB.data());
                sphericalHarmonicsScale(shBuffB.data(), order,
                        shBuff.data(), clr.g * fDiffSolid);
                sphericalHarmonicsAdd(resultG.data(), order,
                        resultG.data(), shBuffB.data());
                sphericalHarmonicsScale(shBuffB.data(), order,
                        shBuff.data(), clr.b * fDiffSolid);
                sphericalHarmonicsAdd(resultB.data(), order,
                        resultB.data(), shBuffB.data());
            }
        }
    }

    // final scale for coefficients
    const float fNormProj = (4.0f * glm::pi<float>()) / fWt;
    sphericalHarmonicsScale(resultR.data(), order, resultR.data(), fNormProj);
    sphericalHarmonicsScale(resultG.data(), order, resultG.data(), fNormProj);
    sphericalHarmonicsScale(resultB.data(), order, resultB.data(), fNormProj);

    // save result
    for(uint i=0; i < sqOrder; i++) {
        // gamma Correct
        // output[i] = linearTosRGB(glm::vec3(resultR[i], resultG[i], resultB[i]));
        output[i] = glm::vec3(resultR[i], resultG[i], resultB[i]);
    }
}

const SphericalHarmonics& Skybox::getIrradianceSH() const {
    if (!_isSHValid) {
        if (_cubemap && _cubemap->isDefined()) {
            std::vector< glm::vec3 > coefs;
            sphericalHarmonicsFromTexture(*_cubemap, coefs, 3);

            _irradianceSH.L00 = coefs[0];
            _irradianceSH.L1m1 = coefs[1];
            _irradianceSH.L10  = coefs[2];
            _irradianceSH.L11  = coefs[3];
            _irradianceSH.L2m2 = coefs[4];
            _irradianceSH.L2m1 = coefs[5];
            _irradianceSH.L20 = coefs[6];
            _irradianceSH.L21 = coefs[7];
            _irradianceSH.L22 = coefs[8];

            _isSHValid = true; 
        }
    }
    return _irradianceSH;
}
