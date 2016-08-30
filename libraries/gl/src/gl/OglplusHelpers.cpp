//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OglplusHelpers.h"

#include <set>
#include <oglplus/shapes/plane.hpp>
#include <oglplus/shapes/sky_box.hpp>

using namespace oglplus;
using namespace oglplus::shapes;

static const char * SIMPLE_TEXTURED_VS = R"VS(#version 410 core
#pragma line __LINE__

uniform mat4 mvp = mat4(1);

in vec3 Position;
in vec2 TexCoord;

out vec3 vPosition;
out vec2 vTexCoord;

void main() {
  gl_Position = mvp * vec4(Position, 1);
  vTexCoord = TexCoord;
  vPosition = Position;
}

)VS";

static const char * SIMPLE_TEXTURED_FS = R"FS(#version 410 core
#pragma line __LINE__

uniform sampler2D sampler;
uniform float alpha = 1.0;

in vec3 vPosition;
in vec2 vTexCoord;

out vec4 FragColor;

void main() {
    FragColor = texture(sampler, vTexCoord);
    FragColor.a *= alpha;
    if (FragColor.a <= 0.0) {
        discard;
    }
}

)FS";


static const char * SIMPLE_TEXTURED_CUBEMAP_FS = R"FS(#version 410 core
#pragma line __LINE__

uniform samplerCube sampler;
uniform float alpha = 1.0;

in vec3 vPosition;
in vec3 vTexCoord;

out vec4 FragColor;

void main() {

    FragColor = texture(sampler, vPosition);
    FragColor.a *= alpha;
}

)FS";


ProgramPtr loadDefaultShader() {
    ProgramPtr result;
    compileProgram(result, SIMPLE_TEXTURED_VS, SIMPLE_TEXTURED_FS);
    return result;
}

ProgramPtr loadCubemapShader() {
    ProgramPtr result;
    compileProgram(result, SIMPLE_TEXTURED_VS, SIMPLE_TEXTURED_CUBEMAP_FS);
    return result;
}

void compileProgram(ProgramPtr & result, const std::string& vs, const std::string& gs, const std::string& fs) {
    using namespace oglplus;
    try {
        result = std::make_shared<Program>();
        // attach the shaders to the program
        result->AttachShader(
            VertexShader()
            .Source(GLSLSource(vs))
            .Compile()
            );
        result->AttachShader(
            GeometryShader()
            .Source(GLSLSource(gs))
            .Compile()
            );
        result->AttachShader(
            FragmentShader()
            .Source(GLSLSource(fs))
            .Compile()
            );
        result->Link();
    } catch (ProgramBuildError& err) {
        Q_UNUSED(err);
        qWarning() << err.Log().c_str();
        Q_ASSERT_X(false, "compileProgram", "Failed to build shader program");
        qFatal("%s", (const char*)err.Message);
        result.reset();
    }
}

void compileProgram(ProgramPtr & result, const std::string& vs, const std::string& fs) {
    using namespace oglplus;
    try {
        result = std::make_shared<Program>();
        // attach the shaders to the program
        result->AttachShader(
            VertexShader()
            .Source(GLSLSource(vs))
            .Compile()
            );
        result->AttachShader(
            FragmentShader()
            .Source(GLSLSource(fs))
            .Compile()
            );
        result->Link();
    } catch (ProgramBuildError& err) {
        Q_UNUSED(err);
        qWarning() << err.Log().c_str();
        Q_ASSERT_X(false, "compileProgram", "Failed to build shader program");
        qFatal("%s", (const char*) err.Message);
        result.reset();
    }
}


ShapeWrapperPtr loadPlane(ProgramPtr program, float aspect) {
    using namespace oglplus;
    Vec3f a(1, 0, 0);
    Vec3f b(0, 1, 0);
    if (aspect > 1) {
        b[1] /= aspect;
    } else {
        a[0] *= aspect;
    }
    return ShapeWrapperPtr(
        new shapes::ShapeWrapper({ "Position", "TexCoord" }, shapes::Plane(a, b), *program)
    );
}

ShapeWrapperPtr loadSkybox(ProgramPtr program) {
    return ShapeWrapperPtr(new shapes::ShapeWrapper(std::initializer_list<std::string>{ "Position" }, shapes::SkyBox(), *program));
}

// Return a point's cartesian coordinates on a sphere from pitch and yaw
static glm::vec3 getPoint(float yaw, float pitch) {
    return glm::vec3(glm::cos(-pitch) * (-glm::sin(yaw)),
        glm::sin(-pitch),
        glm::cos(-pitch) * (-glm::cos(yaw)));
}


class SphereSection : public DrawingInstructionWriter, public DrawMode {
public:
    using IndexArray = std::vector<GLuint>;
    using PosArray = std::vector<float>;
    using TexArray = std::vector<float>;
    /// The type of the index container returned by Indices()
    // vertex positions
    PosArray _pos_data;
    // vertex tex coords
    TexArray _tex_data;
    IndexArray _idx_data;
    unsigned int _prim_count{ 0 };

public:
    SphereSection(
        const float fov,
        const float aspectRatio,
        const int slices_,
        const int stacks_) {
        //UV mapping source: http://www.mvps.org/directx/articles/spheremap.htm
        if (fov >= PI) {
            qDebug() << "TexturedHemisphere::buildVBO(): FOV greater or equal than Pi will create issues";
        }

        int gridSize = std::max(slices_, stacks_);
        int gridSizeLog2 = 1;
        while (1 << gridSizeLog2 < gridSize) {
            ++gridSizeLog2;
        }
        gridSize = (1 << gridSizeLog2) + 1;
        // Compute number of vertices needed
        int vertices = gridSize * gridSize;
        _pos_data.resize(vertices * 3);
        _tex_data.resize(vertices * 2);

        // Compute vertices positions and texture UV coordinate
        for (int y = 0; y <= gridSize; ++y) {
            for (int x = 0; x <= gridSize; ++x) {

            }
        }
        for (int i = 0; i < gridSize; i++) {
            float stacksRatio = (float)i / (float)(gridSize - 1); // First stack is 0.0f, last stack is 1.0f
            // abs(theta) <= fov / 2.0f
            float pitch = -fov * (stacksRatio - 0.5f);
            for (int j = 0; j < gridSize; j++) {
                float slicesRatio = (float)j / (float)(gridSize - 1); // First slice is 0.0f, last slice is 1.0f
                // abs(phi) <= fov * aspectRatio / 2.0f
                float yaw = -fov * aspectRatio * (slicesRatio - 0.5f);
                int vertex = i * gridSize + j;
                int posOffset = vertex * 3;
                int texOffset = vertex * 2;
                vec3 pos = getPoint(yaw, pitch);
                _pos_data[posOffset] = pos.x;
                _pos_data[posOffset + 1] = pos.y;
                _pos_data[posOffset + 2] = pos.z;
                _tex_data[texOffset] = slicesRatio;
                _tex_data[texOffset + 1] = stacksRatio;
            }
        } // done with vertices

        int rowLen = gridSize;

        // gridsize now refers to the triangles, not the vertices, so reduce by one
        // or die by fencepost error http://en.wikipedia.org/wiki/Off-by-one_error
        --gridSize;
        int quads = gridSize * gridSize;
        for (int t = 0; t < quads; ++t) {
            int x = 
                ((t & 0x0001) >> 0) |
                ((t & 0x0004) >> 1) |
                ((t & 0x0010) >> 2) |
                ((t & 0x0040) >> 3) |
                ((t & 0x0100) >> 4) |
                ((t & 0x0400) >> 5) |
                ((t & 0x1000) >> 6) |
                ((t & 0x4000) >> 7);
            int y = 
                ((t & 0x0002) >> 1) |
                ((t & 0x0008) >> 2) |
                ((t & 0x0020) >> 3) |
                ((t & 0x0080) >> 4) |
                ((t & 0x0200) >> 5) |
                ((t & 0x0800) >> 6) |
                ((t & 0x2000) >> 7) |
                ((t & 0x8000) >> 8);
            int i = x * (rowLen) + y;

            _idx_data.push_back(i);
            _idx_data.push_back(i + 1);
            _idx_data.push_back(i + rowLen + 1);

            _idx_data.push_back(i + rowLen + 1);
            _idx_data.push_back(i + rowLen);
            _idx_data.push_back(i);
        }
        _prim_count = quads * 2;
    }

    /// Returns the winding direction of faces
    FaceOrientation FaceWinding(void) const {
        return FaceOrientation::CCW;
    }

    typedef GLuint(SphereSection::*VertexAttribFunc)(std::vector<GLfloat>&) const;

    /// Makes the vertex positions and returns the number of values per vertex
    template <typename T>
    GLuint Positions(std::vector<T>& dest) const {
        dest.clear();
        dest.insert(dest.begin(), _pos_data.begin(), _pos_data.end());
        return 3;
    }

    /// Makes the vertex normals and returns the number of values per vertex
    template <typename T>
    GLuint Normals(std::vector<T>& dest) const {
        dest.clear();
        return 3;
    }

    /// Makes the vertex tangents and returns the number of values per vertex
    template <typename T>
    GLuint Tangents(std::vector<T>& dest) const {
        dest.clear();
        return 3;
    }

    /// Makes the vertex bi-tangents and returns the number of values per vertex
    template <typename T>
    GLuint Bitangents(std::vector<T>& dest) const {
        dest.clear();
        return 3;
    }

    /// Makes the texture coordinates returns the number of values per vertex
    template <typename T>
    GLuint TexCoordinates(std::vector<T>& dest) const {
        dest.clear();
        dest.insert(dest.begin(), _tex_data.begin(), _tex_data.end());
        return 2;
    }

    typedef VertexAttribsInfo<
        SphereSection,
        std::tuple<
        VertexPositionsTag,
        VertexNormalsTag,
        VertexTangentsTag,
        VertexBitangentsTag,
        VertexTexCoordinatesTag
        >
    > VertexAttribs;

    Spheref MakeBoundingSphere(void) const {
        GLfloat min_x = _pos_data[3], max_x = _pos_data[3];
        GLfloat min_y = _pos_data[4], max_y = _pos_data[4];
        GLfloat min_z = _pos_data[5], max_z = _pos_data[5];
        for (std::size_t v = 0, vn = _pos_data.size() / 3; v != vn; ++v) {
            GLfloat x = _pos_data[v * 3 + 0];
            GLfloat y = _pos_data[v * 3 + 1];
            GLfloat z = _pos_data[v * 3 + 2];

            if (min_x > x) min_x = x;
            if (min_y > y) min_y = y;
            if (min_z > z) min_z = z;
            if (max_x < x) max_x = x;
            if (max_y < y) max_y = y;
            if (max_z < z) max_z = z;
        }

        Vec3f c(
            (min_x + max_x) * 0.5f,
            (min_y + max_y) * 0.5f,
            (min_z + max_z) * 0.5f
            );

        return Spheref(
            c.x(), c.y(), c.z(),
            Distance(c, Vec3f(min_x, min_y, min_z))
            );
    }

    /// Queries the bounding sphere coordinates and dimensions
    template <typename T>
    void BoundingSphere(oglplus::Sphere<T>& bounding_sphere) const {
        bounding_sphere = oglplus::Sphere<T>(MakeBoundingSphere());
    }


    /// Returns element indices that are used with the drawing instructions
    const IndexArray & Indices(Default = Default()) const {
        return _idx_data;
    }

    /// Returns the instructions for rendering of faces
    DrawingInstructions Instructions(PrimitiveType primitive) const {
        DrawingInstructions instr = MakeInstructions();
        DrawOperation operation;
        operation.method = DrawOperation::Method::DrawElements;
        operation.mode = primitive;
        operation.first = 0;
        operation.count = _prim_count * 3;
        operation.restart_index = DrawOperation::NoRestartIndex();
        operation.phase = 0;
        AddInstruction(instr, operation);
        return instr;
    }

    /// Returns the instructions for rendering of faces
    DrawingInstructions Instructions(Default = Default()) const {
        return Instructions(PrimitiveType::Triangles);
    }
};

ShapeWrapperPtr loadSphereSection(ProgramPtr program, float fov, float aspect, int slices, int stacks) {
    using namespace oglplus;
    return ShapeWrapperPtr(
        new shapes::ShapeWrapper({ "Position", "TexCoord" }, SphereSection(fov, aspect, slices, stacks), *program)
    );
}

namespace oglplus {
    namespace shapes {

        class Laser : public DrawingInstructionWriter, public DrawMode {
        public:
            using IndexArray = std::vector<GLuint>;
            using PosArray = std::vector<float>;
            /// The type of the index container returned by Indices()
            // vertex positions
            PosArray _pos_data;
            IndexArray _idx_data;
            unsigned int _prim_count { 0 };

        public:
            Laser() {
                int vertices = 2;
                _pos_data.resize(vertices * 3);
                _pos_data[0] = 0;
                _pos_data[1] = 0;
                _pos_data[2] = 0;

                _pos_data[3] = 0;
                _pos_data[4] = 0;
                _pos_data[5] = -1;

                _idx_data.push_back(0);
                _idx_data.push_back(1);
                _prim_count = 1;
            }

            /// Returns the winding direction of faces
            FaceOrientation FaceWinding(void) const {
                return FaceOrientation::CCW;
            }

            /// Queries the bounding sphere coordinates and dimensions
            template <typename T>
            void BoundingSphere(Sphere<T>& bounding_sphere) const {
                bounding_sphere = Sphere<T>(0, 0, -0.5, 0.5);
            }

            typedef GLuint(Laser::*VertexAttribFunc)(std::vector<GLfloat>&) const;

            /// Makes the vertex positions and returns the number of values per vertex
            template <typename T>
            GLuint Positions(std::vector<T>& dest) const {
                dest.clear();
                dest.insert(dest.begin(), _pos_data.begin(), _pos_data.end());
                return 3;
            }

            typedef VertexAttribsInfo<
                Laser,
                std::tuple<VertexPositionsTag>
            > VertexAttribs;


            /// Returns element indices that are used with the drawing instructions
            const IndexArray & Indices(Default = Default()) const {
                return _idx_data;
            }

            /// Returns the instructions for rendering of faces
            DrawingInstructions Instructions(PrimitiveType primitive) const {
                DrawingInstructions instr = MakeInstructions();
                DrawOperation operation;
                operation.method = DrawOperation::Method::DrawElements;
                operation.mode = primitive;
                operation.first = 0;
                operation.count = _prim_count * 3;
                operation.restart_index = DrawOperation::NoRestartIndex();
                operation.phase = 0;
                AddInstruction(instr, operation);
                return instr;
            }

            /// Returns the instructions for rendering of faces
            DrawingInstructions Instructions(Default = Default()) const {
                return Instructions(PrimitiveType::Lines);
            }
        };
    }
}

ShapeWrapperPtr loadLaser(const ProgramPtr& program) {
    return std::make_shared<shapes::ShapeWrapper>(shapes::ShapeWrapper("Position", shapes::Laser(), *program));
}

void TextureRecycler::setSize(const uvec2& size) {
    if (size == _size) {
        return;
    }
    _size = size;
    while (!_readyTextures.empty()) {
        _readyTextures.pop();
    }
    std::set<Map::key_type> toDelete;
    std::for_each(_allTextures.begin(), _allTextures.end(), [&](Map::const_reference item) {
        if (!item.second._active && item.second._size != _size) {
            toDelete.insert(item.first);
        }
    });
    std::for_each(toDelete.begin(), toDelete.end(), [&](Map::key_type key) {
        _allTextures.erase(key);
    });
}

void TextureRecycler::clear() {
    while (!_readyTextures.empty()) {
        _readyTextures.pop();
    }
    _allTextures.clear();
}

TexturePtr TextureRecycler::getNextTexture() {
    using namespace oglplus;
    if (_readyTextures.empty()) {
        TexturePtr newTexture(new Texture());

        if (_useMipmaps) {
            Context::Bound(oglplus::Texture::Target::_2D, *newTexture)
                .MinFilter(TextureMinFilter::LinearMipmapLinear)
                .MagFilter(TextureMagFilter::Linear)
                .WrapS(TextureWrap::ClampToEdge)
                .WrapT(TextureWrap::ClampToEdge)
                .Anisotropy(8.0f)
                .LODBias(-0.2f)
                .Image2D(0, PixelDataInternalFormat::RGBA8,
                         _size.x, _size.y,
                         0, PixelDataFormat::RGB, PixelDataType::UnsignedByte, nullptr);
        } else {
            Context::Bound(oglplus::Texture::Target::_2D, *newTexture)
                .MinFilter(TextureMinFilter::Linear)
                .MagFilter(TextureMagFilter::Linear)
                .WrapS(TextureWrap::ClampToEdge)
                .WrapT(TextureWrap::ClampToEdge)
                .Image2D(0, PixelDataInternalFormat::RGBA8,
                         _size.x, _size.y,
                         0, PixelDataFormat::RGB, PixelDataType::UnsignedByte, nullptr);
        }
        GLuint texId = GetName(*newTexture);
        _allTextures[texId] = TexInfo{ newTexture, _size };
        _readyTextures.push(newTexture);
    }

    TexturePtr result = _readyTextures.front();
    _readyTextures.pop();

    GLuint texId = GetName(*result);
    auto& item = _allTextures[texId];
    item._active = true;

    return result;
}

void TextureRecycler::recycleTexture(GLuint texture) {
    Q_ASSERT(_allTextures.count(texture));
    auto& item = _allTextures[texture];
    Q_ASSERT(item._active);
    item._active = false;
    if (item._size != _size) {
        // Buh-bye
        _allTextures.erase(texture);
        return;
    }

    _readyTextures.push(item._tex);
}

