//
//  Stars.cpp
//  interface/src
//
//  Created by Tobias Schwinger on 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Stars.h"

#include <mutex>

#include <QElapsedTimer>
#include <NumericalConstants.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <TextureCache.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>

#include <render-utils/stars_vert.h>
#include <render-utils/stars_frag.h>

#include <render-utils/standardTransformPNTC_vert.h>
#include <render-utils/starsGrid_frag.h>

//static const float TILT = 0.23f;
static const float TILT = 0.0f;
static const unsigned int STARFIELD_NUM_STARS = 50000;
static const unsigned int STARFIELD_SEED = 1;
static const float STAR_COLORIZATION = 0.1f;

static const float TAU = 6.28318530717958f;
//static const float HALF_TAU = TAU / 2.0f;
//static const float QUARTER_TAU = TAU / 4.0f;
//static const float MILKY_WAY_WIDTH = TAU / 30.0f; // width in radians of one half of the Milky Way
//static const float MILKY_WAY_INCLINATION = 0.0f; // angle of Milky Way from horizontal in degrees
//static const float MILKY_WAY_RATIO = 0.4f;
static const char* UNIFORM_TIME_NAME = "iGlobalTime";

// Produce a random float value between 0 and 1
static float frand() {
    return (float)rand() / (float)RAND_MAX;
}

// http://mathworld.wolfram.com/SpherePointPicking.html
static vec2 randPolar() {
    vec2 result(frand(), frand());
    result.x *= TAU;
    result.y = powf(result.y, 2.0) / 2.0f;
    if (frand() > 0.5f) {
        result.y = 0.5f - result.y;
    } else {
        result.y += 0.5f;
    }
    result.y = acos((2.0f * result.y) - 1.0f);
    return result;
}


static vec3 fromPolar(const vec2& polar) {
    float sinTheta = sin(polar.x);
    float cosTheta = cos(polar.x);
    float sinPhi = sin(polar.y);
    float cosPhi = cos(polar.y);
    return vec3(
        cosTheta * sinPhi,
        cosPhi,
        sinTheta * sinPhi);
}


// computeStarColor
// - Generate a star color.
//
// colorization can be a value between 0 and 1 specifying how colorful the resulting star color is.
//
// 0 = completely black & white
// 1 = very colorful
unsigned computeStarColor(float colorization) {
    unsigned char red, green, blue;
    if (randFloat() < 0.3f) {
        // A few stars are colorful
        red = 2 + (rand() % 254);
        green = 2 + round((red * (1 - colorization)) + ((rand() % 254) * colorization));
        blue = 2 + round((red * (1 - colorization)) + ((rand() % 254) * colorization));
    } else {
        // Most stars are dimmer and white
        red = green = blue = 2 + (rand() % 128);
    }
    return red | (green << 8) | (blue << 16);
}

struct StarVertex {
    vec4 position;
    vec4 colorAndSize;
};

static const int STARS_VERTICES_SLOT{ 0 };
static const int STARS_COLOR_SLOT{ 1 };

gpu::PipelinePointer Stars::_gridPipeline{};
gpu::PipelinePointer Stars::_starsPipeline{};
int32_t Stars::_timeSlot{ -1 };

void Stars::init() {
    if (!_gridPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(standardTransformPNTC_vert));
        auto ps = gpu::Shader::createPixel(std::string(starsGrid_frag));
        auto program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram((*program));
        _timeSlot = program->getBuffers().findLocation(UNIFORM_TIME_NAME);
        if (_timeSlot == gpu::Shader::INVALID_LOCATION) {
            _timeSlot = program->getUniforms().findLocation(UNIFORM_TIME_NAME);
        }
        auto state = gpu::StatePointer(new gpu::State());
        // enable decal blend
        state->setDepthTest(gpu::State::DepthTest(false));
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        _gridPipeline = gpu::Pipeline::create(program, state);
    }

    if (!_starsPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(stars_vert));
        auto ps = gpu::Shader::createPixel(std::string(stars_frag));
        auto program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram((*program));
        auto state = gpu::StatePointer(new gpu::State());
        // enable decal blend
        state->setDepthTest(gpu::State::DepthTest(false));
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));
        state->setAntialiasedLineEnable(true); // line smoothing also smooth points
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        _starsPipeline = gpu::Pipeline::create(program, state);
    }

    unsigned limit = STARFIELD_NUM_STARS;
    std::vector<StarVertex> points;
    points.resize(limit);

    { // generate stars
        QElapsedTimer startTime;
        startTime.start();

        vertexBuffer.reset(new gpu::Buffer);

        srand(STARFIELD_SEED);
        for (size_t star = 0; star < limit; ++star) {
            points[star].position = vec4(fromPolar(randPolar()), 1);
            float size = frand() * 2.5f + 0.5f;
            if (frand() < STAR_COLORIZATION) {
                vec3 color(frand() / 2.0f + 0.5f, frand() / 2.0f + 0.5f, frand() / 2.0f + 0.5f);
                points[star].colorAndSize = vec4(color, size);
            } else {
                vec3 color(frand() / 2.0f + 0.5f);
                points[star].colorAndSize = vec4(color, size);
            }
        }

        double timeDiff = (double)startTime.nsecsElapsed() / 1000000.0; // ns to ms
        qDebug() << "Total time to generate stars: " << timeDiff << " msec";
    }

    gpu::Element positionElement, colorElement;
    const size_t VERTEX_STRIDE = sizeof(StarVertex);

    vertexBuffer->append(VERTEX_STRIDE * limit, (const gpu::Byte*)&points[0]);
    streamFormat.reset(new gpu::Stream::Format()); // 1 for everyone
    streamFormat->setAttribute(gpu::Stream::POSITION, STARS_VERTICES_SLOT, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::XYZW), 0);
    streamFormat->setAttribute(gpu::Stream::COLOR, STARS_COLOR_SLOT, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::RGBA));
    positionElement = streamFormat->getAttributes().at(gpu::Stream::POSITION)._element;
    colorElement = streamFormat->getAttributes().at(gpu::Stream::COLOR)._element;

    size_t offset = offsetof(StarVertex, position);
    positionView = gpu::BufferView(vertexBuffer, offset, vertexBuffer->getSize(), VERTEX_STRIDE, positionElement);

    offset = offsetof(StarVertex, colorAndSize);
    colorView = gpu::BufferView(vertexBuffer, offset, vertexBuffer->getSize(), VERTEX_STRIDE, colorElement);
}

// FIXME star colors
void Stars::render(RenderArgs* renderArgs, float alpha) {
    std::call_once(once, [&]{ init(); });


    auto modelCache = DependencyManager::get<ModelCache>();
    auto textureCache = DependencyManager::get<TextureCache>();
    auto geometryCache = DependencyManager::get<GeometryCache>();


    gpu::Batch& batch = *renderArgs->_batch;
    batch.setViewTransform(Transform());
    batch.setProjectionTransform(renderArgs->getViewFrustum().getProjection());
    batch.setModelTransform(Transform().setRotation(glm::inverse(renderArgs->getViewFrustum().getOrientation()) *
        quat(vec3(TILT, 0, 0))));
    batch.setResourceTexture(0, textureCache->getWhiteTexture());

    // Render the world lines
    batch.setPipeline(_gridPipeline);
    static auto start = usecTimestampNow();
    float msecs = (float)(usecTimestampNow() - start) / (float)USECS_PER_MSEC;
    float secs = msecs / (float)MSECS_PER_SECOND;
    batch._glUniform1f(_timeSlot, secs);
    geometryCache->renderCube(batch);

    // Render the stars
    batch.setPipeline(_starsPipeline);
    batch.setInputFormat(streamFormat);
    batch.setInputBuffer(STARS_VERTICES_SLOT, positionView);
    batch.setInputBuffer(STARS_COLOR_SLOT, colorView);
    batch.draw(gpu::Primitive::POINTS, STARFIELD_NUM_STARS);
}
