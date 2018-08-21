//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestTextures.h"

#include <random>
#include <algorithm>

#include <QtCore/QDir>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlContext>
#include <gpu/Batch.h>
#include <gpu/Context.h>

#include "TestHelpers.h"

std::string vertexShaderSource = R"SHADER(
#line 14
layout(location = 0) out vec2 outTexCoord0;

const vec4 VERTICES[] = vec4[](
    vec4(-1.0, -1.0, 0.0, 1.0), 
    vec4( 1.0, -1.0, 0.0, 1.0),    
    vec4(-1.0,  1.0, 0.0, 1.0),
    vec4( 1.0,  1.0, 0.0, 1.0)
);   

void main() {
    outTexCoord0 = VERTICES[gl_VertexID].xy;
    outTexCoord0 += 1.0;
    outTexCoord0 /= 2.0;
    gl_Position = VERTICES[gl_VertexID];
}
)SHADER";

std::string fragmentShaderSource = R"SHADER(
#line 28

uniform sampler2D tex;

layout(location = 0) in vec2 inTexCoord0;
layout(location = 0) out vec4 outFragColor;

void main() {
    outFragColor = texture(tex, inTexCoord0);
    outFragColor.a = 1.0;
    //outFragColor.rb = inTexCoord0;
}

)SHADER";

#define STAT_UPDATE(name, src) \
    { \
        auto val = src; \
        if (_##name != val) { \
            _##name = val; \
            emit name##Changed(); \
        } \
    }


void TextureTestStats::update(int curIndex, const gpu::TexturePointer& texture) {
    STAT_UPDATE(total, (int)BYTES_TO_MB(gpu::Context::getTextureGPUMemSize()));
    STAT_UPDATE(allocated, (int)gpu::Context::getTextureResourceGPUMemSize());
    STAT_UPDATE(pending, (int)gpu::Context::getTexturePendingGPUTransferMemSize());
    STAT_UPDATE(populated, (int)gpu::Context::getTextureResourcePopulatedGPUMemSize());
    STAT_UPDATE(source, texture->source().c_str());
    STAT_UPDATE(index, curIndex);
    auto dims = texture->getDimensions();
    STAT_UPDATE(rez, QSize(dims.x, dims.y));
}

TexturesTest::TexturesTest() {
    connect(&stats, &TextureTestStats::changeTextures, this, &TexturesTest::onChangeTextures);
    connect(&stats, &TextureTestStats::nextTexture, this, &TexturesTest::onNextTexture);
    connect(&stats, &TextureTestStats::prevTexture, this, &TexturesTest::onPrevTexture);
    connect(&stats, &TextureTestStats::maxTextureMemory, this, &TexturesTest::onMaxTextureMemory);
    {
        auto VS = gpu::Shader::createVertex({ vertexShaderSource, {} });
        auto PS = gpu::Shader::createPixel({ fragmentShaderSource, {} });
        auto program = gpu::Shader::createProgram(VS, PS);
        // If the pipeline did not exist, make it
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_NONE);
        state->setDepthTest({});
        state->setBlendFunction({ false });
        pipeline = gpu::Pipeline::create(program, state);
    }

    onChangeTextures();
}


void TexturesTest::renderTest(size_t testId, const RenderArgs& args) {
    stats.update((int)index, textures[index]);
    gpu::Batch& batch = *(args._batch);
    batch.setPipeline(pipeline);
    batch.setInputFormat(vertexFormat);
    for (const auto& texture : textures) {
        batch.setResourceTexture(0, texture);
        batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
    }
    batch.setResourceTexture(0, textures[index]);
    batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
}

#define LOAD_TEXTURE_COUNT 64

void TexturesTest::onChangeTextures() {
    static const QDir TEST_DIR("D:/ktx_texture_test");
    static std::vector<std::string> ALL_TEXTURE_FILES;
    if (ALL_TEXTURE_FILES.empty()) {
        auto entryList = TEST_DIR.entryList({ "*.ktx" }, QDir::Filter::Files);
        ALL_TEXTURE_FILES.reserve(entryList.size());
        for (auto entry : entryList) {
            auto textureFile = TEST_DIR.absoluteFilePath(entry).toStdString();
            ALL_TEXTURE_FILES.push_back(textureFile);
        }
    }

    oldTextures.clear();
    oldTextures.swap(textures);

#if 0
    static const std::string bad = "D:/ktx_texture_test/b4beed38675dbc7a827ecd576399c1f4.ktx";
    auto texture = gpu::Texture::unserialize(bad);
    auto texelFormat = texture->getTexelFormat();
    qDebug() << texture->getTexelFormat().getSemantic();
    qDebug() << texture->getTexelFormat().getScalarCount();
    textures.push_back(texture);
#else
    std::shuffle(ALL_TEXTURE_FILES.begin(), ALL_TEXTURE_FILES.end(), std::default_random_engine());
    size_t newTextureCount = std::min<size_t>(ALL_TEXTURE_FILES.size(), LOAD_TEXTURE_COUNT);
    for (size_t i = 0; i < newTextureCount; ++i) {
        const auto& textureFile = ALL_TEXTURE_FILES[i];
        auto texture = gpu::Texture::unserialize(textureFile);
        qDebug() << textureFile.c_str();
        qDebug() << texture->getTexelFormat().getSemantic();
        qDebug() << texture->getTexelFormat().getScalarCount();
        textures.push_back(texture);
    }
#endif
    index = 0;
    qDebug() << "Done";
}

void TexturesTest::onNextTexture() {
    index += textures.size() + 1;
    index %= textures.size();
}

void TexturesTest::onPrevTexture() {
    index += textures.size() - 1;
    index %= textures.size();
}

void TexturesTest::onMaxTextureMemory(int maxTextureMemory) {
    gpu::Texture::setAllowedGPUMemoryUsage(MB_TO_BYTES(maxTextureMemory));
}
