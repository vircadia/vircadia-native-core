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

#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <NumericalConstants.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <TextureCache.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>

static const unsigned int STARFIELD_NUM_STARS = 50000;
static const unsigned int STARFIELD_SEED = 1;
static const float STAR_COLORIZATION = 0.1f;

static const float TAU = 6.28318530717958f;
static const float HALF_TAU = TAU / 2.0f;
static const float QUARTER_TAU = TAU / 4.0f;
static const float MILKY_WAY_WIDTH = TAU / 30.0f; // width in radians of one half of the Milky Way
static const float MILKY_WAY_INCLINATION = 0.0f; // angle of Milky Way from horizontal in degrees
static const float MILKY_WAY_RATIO = 0.4f;

#include "../../libraries/render-utils/standardTransformPNTC_vert.h"

const char stars_frag[] = R"SCRIBE(#version 430 compatibility
void main(void) {
    gl_FragColor = vec4( 1.0 ); 
}
)SCRIBE";

Stars::Stars() {
}

Stars::~Stars() {
}

// Produce a random float value between 0 and 1
static float frand() {
    return (float)rand() / (float)RAND_MAX;
}

// Produce a random radian value between 0 and 2 PI (TAU)
static float rrand() {
    return frand() * TAU;
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

// FIXME star colors
void Stars::render(RenderArgs* renderArgs, float alpha) {
    static gpu::BufferPointer vertexBuffer;
    static gpu::Stream::FormatPointer streamFormat;
    static gpu::Element positionElement, colorElement;

    const int VERTICES_SLOT = 0;
    const int COLOR_SLOT = 2;

    static std::once_flag once;

    std::call_once(once, [&] {
        QElapsedTimer startTime;
        startTime.start();

        vertexBuffer.reset(new gpu::Buffer);

        srand(STARFIELD_SEED);
        unsigned limit = STARFIELD_NUM_STARS;
        std::vector<vec3> points;
        points.reserve(limit);
        for (size_t star = 0; star < limit; ++star) {
            points[star] = fromPolar(randPolar());
            //auto color = computeStarColor(STAR_COLORIZATION);
            //vertexBuffer->append(sizeof(color), (const gpu::Byte*)&color);
        }
        vertexBuffer->append(sizeof(vec3) * limit, (const gpu::Byte*)&points[0]);
        streamFormat.reset(new gpu::Stream::Format()); // 1 for everyone
        streamFormat->setAttribute(gpu::Stream::POSITION, VERTICES_SLOT, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
        positionElement = streamFormat->getAttributes().at(gpu::Stream::POSITION)._element;
        double timeDiff = (double)startTime.nsecsElapsed() / 1000000.0; // ns to ms
        qDebug() << "Total time to generate stars: " << timeDiff << " msec";
    });

    gpu::Batch batch;
    batch.setInputFormat(streamFormat);
    batch.setInputBuffer(VERTICES_SLOT, gpu::BufferView(vertexBuffer, positionElement));
    batch.setViewTransform(Transform());
    batch.setProjectionTransform(renderArgs->_viewFrustum->getProjection());
    batch.setModelTransform(Transform().setRotation(glm::inverse(renderArgs->_viewFrustum->getOrientation())));
    auto geometryCache = DependencyManager::get<GeometryCache>();
    auto textureCache = DependencyManager::get<TextureCache>();
    geometryCache->useSimpleDrawPipeline(batch);
    batch.setResourceTexture(0, textureCache->getWhiteTexture());
    batch.draw(gpu::Primitive::POINTS, STARFIELD_NUM_STARS);
    renderArgs->_context->render(batch);
}
