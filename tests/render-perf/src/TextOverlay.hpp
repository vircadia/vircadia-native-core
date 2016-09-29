/*
* Text overlay class for displaying debug information
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <GLMHelpers.h>

#include "stb_font_consolas_24_latin1.inl"

// Defines for the STB font used
// STB font files can be found at http://nothings.org/stb/font/
#define STB_FONT_NAME stb_font_consolas_24_latin1
#define STB_FONT_WIDTH STB_FONT_consolas_24_latin1_BITMAP_WIDTH
#define STB_FONT_HEIGHT STB_FONT_consolas_24_latin1_BITMAP_HEIGHT 
#define STB_FIRST_CHAR STB_FONT_consolas_24_latin1_FIRST_CHAR
#define STB_NUM_CHARS STB_FONT_consolas_24_latin1_NUM_CHARS

// Max. number of chars the text overlay buffer can hold
#define MAX_CHAR_COUNT 1024

// Mostly self-contained text overlay class
// todo : comment
class TextOverlay {
private:
    uvec2 _size;
// FIXME port from oglplus
#if 0
    FramebufferPtr _framebuffer;
    TexturePtr _texture;
    BufferPtr _vertexBuffer;
    ProgramPtr _program;
    VertexArrayPtr _vertexArray;
    // Pointer to mapped vertex buffer
    glm::vec4* _mapped { nullptr };

    const char* const VERTEX_SHADER = R"SHADER(
#version 450 core
layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec2 outUV;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main(void)
{
    vec2 pos = inPos;
    pos.y *= -1.0;
    gl_Position = vec4(pos, 0.0, 1.0);
    outUV = inUV;
}
)SHADER";

    const char* const FRAGMENT_SHADER = R"SHADER(
#version 450 core

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerFont;

layout (location = 0) out vec4 outFragColor;

void main(void)
{
    float color = texture(samplerFont, inUV).r;
    outFragColor = vec4(vec3(color), 1.0);
}
)SHADER";

#endif

    stb_fontchar stbFontData[STB_NUM_CHARS];
    uint32_t numLetters;


public:
    enum TextAlign { alignLeft, alignCenter, alignRight };

    bool visible = true;
    bool invalidated = false;


    TextOverlay(const glm::uvec2& size) : _size(size) {
        prepare();
    }

    ~TextOverlay() {
#if 0
        // Free up all Vulkan resources requested by the text overlay
        _program.reset();
        _texture.reset();
        _vertexBuffer.reset();
        _vertexArray.reset();
#endif
    }

    void resize(const uvec2& size) {
        _size = size;
    }

    // Prepare all vulkan resources required to render the font
    // The text overlay uses separate resources for descriptors (pool, sets, layouts), pipelines and command buffers
    void prepare() {
#if 0
        static unsigned char font24pixels[STB_FONT_HEIGHT][STB_FONT_WIDTH];
        STB_FONT_NAME(stbFontData, font24pixels, STB_FONT_HEIGHT);

        // Vertex buffer
        {
            GLuint buffer;
            GLuint bufferSize = MAX_CHAR_COUNT * sizeof(glm::vec4);
            glCreateBuffers(1, &buffer);
            glNamedBufferStorage(buffer, bufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
            using BufferName = oglplus::ObjectName<oglplus::tag::Buffer>;
            BufferName name = BufferName(buffer);
            _vertexBuffer = std::make_shared<oglplus::Buffer>(oglplus::Buffer::FromRawName(name));
            _vertexArray = std::make_shared<oglplus::VertexArray>();
            _vertexArray->Bind();
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)sizeof(glm::vec2));
            glBindVertexArray(0);
        }

        // Font texture
        {
            GLuint texture;
            glCreateTextures(GL_TEXTURE_2D, 1, &texture);
            glTextureStorage2D(texture, 1, GL_R8, STB_FONT_WIDTH, STB_FONT_HEIGHT);
            glTextureSubImage2D(texture, 0, 0, 0, STB_FONT_WIDTH, STB_FONT_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, &font24pixels[0][0]);
            glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            using TextureName = oglplus::ObjectName<oglplus::tag::Texture>;
            TextureName name = TextureName(texture);
            _texture = std::make_shared<oglplus::Texture>(oglplus::Texture::FromRawName(name));
        }

        compileProgram(_program, VERTEX_SHADER, FRAGMENT_SHADER);
#endif
    }


    // Map buffer 
    void beginTextUpdate() {
#if 0
        using namespace oglplus;
        _mapped = (glm::vec4*)glMapNamedBuffer(GetName(*_vertexBuffer), GL_WRITE_ONLY);
        numLetters = 0;
#endif
    }

    // Add text to the current buffer
    // todo : drop shadow? color attribute?
    void addText(std::string text, vec2 pos, TextAlign align) {
        assert(_mapped != nullptr);
        const vec2 fbSize = _size;
        const vec2 charSize = vec2(1.5f) / fbSize;
        pos = (pos / fbSize * 2.0f) - 1.0f;

        // Calculate text width
        float textWidth = 0;
        for (auto letter : text) {
            stb_fontchar *charData = &stbFontData[(uint32_t)letter - STB_FIRST_CHAR];
            textWidth += charData->advance * charSize.x;
        }

        switch (align) {
        case alignRight:
            pos.x -= textWidth;
            break;
        case alignCenter:
            pos.x -= textWidth / 2.0f;
            break;
        case alignLeft:
            break;
        }

        // Generate a uv mapped quad per char in the new text
        for (auto letter : text) {
            stb_fontchar *charData = &stbFontData[(uint32_t)letter - STB_FIRST_CHAR];
            _mapped->x = pos.x + (float)charData->x0 * charSize.x;
            _mapped->y = pos.y + (float)charData->y0 * charSize.y;
            _mapped->z = charData->s0;
            _mapped->w = charData->t0;
            ++_mapped;

            _mapped->x = pos.x + (float)charData->x1 * charSize.x;
            _mapped->y = pos.y + (float)charData->y0 * charSize.y;
            _mapped->z = charData->s1;
            _mapped->w = charData->t0;
            ++_mapped;

            _mapped->x = pos.x + (float)charData->x0 * charSize.x;
            _mapped->y = pos.y + (float)charData->y1 * charSize.y;
            _mapped->z = charData->s0;
            _mapped->w = charData->t1;
            _mapped++;

            _mapped->x = pos.x + (float)charData->x1 * charSize.x;
            _mapped->y = pos.y + (float)charData->y1 * charSize.y;
            _mapped->z = charData->s1;
            _mapped->w = charData->t1;
            _mapped++;
            pos.x += charData->advance * charSize.x;
            numLetters++;
        }
    }

    // Unmap buffer and update command buffers
    void endTextUpdate() {
#if 0
        glUnmapNamedBuffer(GetName(*_vertexBuffer));
        _mapped = nullptr;
#endif
    }

    // Needs to be called by the application
    void render() {
#if 0
        _texture->Bind(oglplus::TextureTarget::_2D);
        _program->Use();
        _vertexArray->Bind();
        for (uint32_t j = 0; j < numLetters; j++) {
            glDrawArrays(GL_TRIANGLE_STRIP, j * 4, 4);
        }
#endif
    }
};
